#include <cista/mmap.h>

#include "osmium/area/assembler.hpp"
#include "osmium/area/multipolygon_manager.hpp"
#include "osmium/handler/node_locations_for_ways.hpp"
#include "osmium/index/map/flex_mem.hpp"
#include "osmium/io/pbf_input.hpp"

#include "utl/enumerate.h"
#include "utl/helpers/algorithm.h"
#include "utl/parallel_for.h"
#include "utl/progress_tracker.h"
#include "utl/timer.h"
#include "utl/timing.h"
#include "utl/to_vec.h"
#include "utl/zip.h"

#include "tiles/osm/hybrid_node_idx.h"
#include "tiles/osm/tmp_file.h"
#include "tiles/util_parallel.h"

#include "rtree.h"

#include "adr/area_database.h"
#include "adr/import_context.h"
#include "adr/reverse.h"
#include "adr/typeahead.h"

namespace osm = osmium;
namespace osm_io = osmium::io;
namespace osm_rel = osmium::relations;
namespace osm_eb = osmium::osm_entity_bits;
namespace osm_area = osmium::area;
namespace osm_mem = osmium::memory;

namespace adr {

struct feature_handler : public osmium::handler::Handler {
  feature_handler(area_database& area_db,
                  typeahead& t,
                  reverse& r,
                  import_context& ctx,
                  rtree* areas,
                  std::mutex& areas_mutex)
      : area_db_{area_db},
        t_{t},
        r_{r},
        ctx_{ctx},
        areas_{areas},
        areas_mutex_{areas_mutex} {}

  void way(osmium::Way const& w) {
    if (!w.nodes().empty()) {
      auto const& tags = w.tags();
      if (!tags.has_key("public_transport") && !tags.has_key("electrified") &&
          !tags.has_key("railway") && !tags.has_key("waterway") &&
          !tags.has_tag("information", "board") && !tags.has_key("tunnel") &&
          !tags.has_tag("amenity", "toilets") &&
          !tags.has_tag("natural", "wood") &&
          !tags.has_tag("building", "industrial") &&
          !tags.has_tag("leisure", "playground") &&
          !tags.has_tag("access", "false") &&
          !tags.has_tag("amenity", "taxi")) {
        if (tags.has_key("highway")) {
          auto const street =
              t_.add_street(ctx_, tags, w.nodes().front().location());
          if (street != street_idx_t::invalid()) {
            r_.add_street(ctx_, street, w);
          }
        } else {
          t_.add_place(ctx_, w.id(), true, tags, w.nodes().front().location());
        }
        t_.add_address(ctx_, tags, w.nodes().front().location());
      }
    }
  }

  void node(osmium::Node const& n) {
    auto const& tags = n.tags();
    if (!tags.has_tag("emergency", "fire_hydrant") &&
        !tags.has_key("public_transport") && /* stops from timetable */
        !tags.has_key("highway") && /* named motorway_junction */
        !tags.has_key("electrified") && !tags.has_key("railway") &&
        !tags.has_tag("information", "board") && !tags.has_key("waterway") &&
        !tags.has_key("tunnel") && !tags.has_tag("amenity", "toilets") &&
        !tags.has_tag("natural", "wood") && !tags.has_key("traffic_sign") &&
        !tags.has_tag("building", "industrial") &&
        !tags.has_tag("amenity", "bicycle_rental") &&
        !tags.has_tag("leisure", "playground") &&
        !tags.has_tag("access", "false") && !tags.has_tag("amenity", "taxi")) {
      t_.add_address(ctx_, tags, n.location());
      t_.add_place(ctx_, n.id(), false, tags, n.location());
    }
  }

  void area(osmium::Area const& a) {
    auto const env = a.envelope();
    auto const& tags = a.tags();
    if (!env.valid() ||
        ((!tags.has_key("admin_level") || !tags.has_key("name")) &&
         !tags.has_key("postal_code"))) {
      return;
    }

    auto const lock = std::scoped_lock{areas_mutex_};

    auto const admin_area_idx = t_.add_admin_area(ctx_, a.tags());
    auto const postal_code_area_idx = t_.add_postal_code_area(ctx_, a.tags());

    for (auto const& outer : a.outer_rings()) {
      auto const outer_env = outer.envelope();
      auto const min_corner = std::array<double, 2U>{
          outer_env.bottom_left().lon(), outer_env.bottom_left().lat()};
      auto const max_corner = std::array<double, 2U>{
          outer_env.top_right().lon(), outer_env.top_right().lat()};

      if (admin_area_idx != area_idx_t::invalid()) {
        rtree_insert(areas_, min_corner.data(), max_corner.data(),
                     reinterpret_cast<void*>(to_idx(admin_area_idx)));
      }

      if (postal_code_area_idx != area_idx_t::invalid()) {
        rtree_insert(areas_, min_corner.data(), max_corner.data(),
                     reinterpret_cast<void*>(to_idx(postal_code_area_idx)));
      }
    }

    if (admin_area_idx != area_idx_t::invalid()) {
      area_db_.add_area(admin_area_idx, a);
    }

    if (postal_code_area_idx != area_idx_t::invalid()) {
      area_db_.add_area(postal_code_area_idx, a);
    }
  }

  area_database& area_db_;
  typeahead& t_;
  reverse& r_;
  import_context& ctx_;
  rtree* areas_;
  std::mutex& areas_mutex_;
};

void extract(std::filesystem::path const& in_path,
             std::filesystem::path const& out_path,
             std::filesystem::path const& tmp_dname) {
  auto input_file = osm_io::File{};
  auto file_size = std::size_t{0U};
  try {
    input_file = osm_io::File{in_path};
    file_size =
        osm_io::Reader{input_file, osmium::io::read_meta::no}.file_size();
  } catch (...) {
    fmt::print("load_osm failed [file={}]\n", in_path);
    throw;
  }

  std::cout << "size=" << file_size << "\n";

  auto pt = utl::get_active_progress_tracker_or_activate("import");
  pt->status("Load OSM").out_mod(3.F).in_high(2 * file_size);

  auto const filter = osm::TagsFilter{}
                          .add_rule(true, "boundary", "postal_code")
                          .add_rule(true, "boundary", "administrative")
                          .add_rule(true, "admin_level");

  auto area_db = area_database{};
  auto const node_idx_file =
      tiles::tmp_file{(tmp_dname / "idx.bin").generic_string()};
  auto const node_dat_file =
      tiles::tmp_file{(tmp_dname / "dat.bin").generic_string()};
  auto node_idx =
      tiles::hybrid_node_idx{node_idx_file.fileno(), node_dat_file.fileno()};
  auto mp_manager = osm_area::MultipolygonManager<osm_area::Assembler>{
      osm_area::Assembler::config_type{}, filter};

  {  // Collect node coordinates.
    pt->status("Load OSM / Pass 1");
    auto node_idx_builder = tiles::hybrid_node_idx_builder{node_idx};

    auto reader = osm_io::Reader{input_file, osm_eb::node | osm_eb::relation,
                                 osmium::io::read_meta::no};
    while (auto buffer = reader.read()) {
      pt->update(reader.offset());
      osm::apply(buffer, node_idx_builder, mp_manager);
    }
    reader.close();

    mp_manager.prepare_for_lookup();
    std::clog << "Multipolygon Manager Memory:\n";
    osm_rel::print_used_memory(std::clog, mp_manager.used_memory());

    node_idx_builder.finish();
    std::clog << "Hybrid Node Index Statistics:\n";
    node_idx_builder.dump_stats();
  }

  auto ctx = import_context{};
  auto t = typeahead{};
  auto r = reverse{};
  t.lang_names_.emplace_back("default");
  auto areas = rtree_new();
  auto areas_mutex = std::mutex{};
  auto mp_queue = tiles::in_order_queue<osm_mem::Buffer>{};
  {  // Extract streets, places, and areas.
    pt->status("Load OSM / Pass 2");
    auto const thread_count =
        std::max(2, static_cast<int>(std::thread::hardware_concurrency()));

    // pool must be destructed before handlers!
    auto pool = osmium::thread::Pool{thread_count,
                                     static_cast<size_t>(thread_count * 8)};

    auto reader = osm_io::Reader{input_file, pool, osmium::io::read_meta::no};
    auto seq_reader = tiles::sequential_until_finish<osm_mem::Buffer>{[&] {
      pt->update(reader.file_size() + reader.offset());
      return reader.read();
    }};

    std::atomic_bool has_exception{false};
    std::vector<std::future<void>> workers;
    auto handler = feature_handler{area_db, t, r, ctx, areas, areas_mutex};
    workers.reserve(thread_count / 2);
    for (auto i = 0; i < thread_count / 2; ++i) {
      workers.emplace_back(pool.submit([&] {
        try {
          while (true) {
            auto opt = seq_reader.process();
            if (!opt.has_value()) {
              break;
            }

            auto& [idx, buf] = *opt;
            tiles::update_locations(node_idx, buf);
            osm::apply(buf, handler);

            mp_queue.process_in_order(idx, std::move(buf), [&](auto buf2) {
              osm::apply(buf2, mp_manager.handler([&](auto&& mp_buffer) {
                auto p = std::make_shared<osm_mem::Buffer>(
                    std::forward<decltype(mp_buffer)>(mp_buffer));
                pool.submit([&, p] { osm::apply(*p, handler); });
              }));
            });
          }
        } catch (std::exception const& e) {
          fmt::print(std::clog, "EXCEPTION CAUGHT: {} {}\n",
                     std::this_thread::get_id(), e.what());
          has_exception = true;
        } catch (...) {
          fmt::print(std::clog, "UNKNOWN EXCEPTION CAUGHT: {} \n",
                     std::this_thread::get_id());
          has_exception = true;
        }
      }));
    }

    utl::verify(!workers.empty(), "have no workers");
    for (auto& worker : workers) {
      worker.wait();
    }

    utl::verify(!has_exception, "load_osm: exception caught!");
    utl::verify(mp_queue.queue_.empty(), "mp_queue not empty!");

    reader.close();
    pt->update(pt->in_high_);

    std::cout << "Multipolygon Manager Memory:\n";
    osm_rel::print_used_memory(std::clog, mp_manager.used_memory());
  }

  //  auto stats = utl::to_vec(ctx.place_stats_, [](auto&& x) { return x; });
  //  utl::sort(stats, [](auto&& a, auto&& b) { return a.second > b.second; });
  //  for (auto const& [k, v] : stats) {
  //    std::cout << k << ": " << v << "\n";
  //  }

  {  // Copy data from context to typeahead (not possible before, because vecvec
     // requires to build indices 0, ..., N in order).
    UTL_START_TIMING(copy_data);
    for (auto const b : ctx.street_pos_) {
      t.street_pos_.emplace_back(b);
    }
    for (auto const b : ctx.house_numbers_) {
      t.house_numbers_.emplace_back(b);
    }
    for (auto const b : ctx.house_coordinates_) {
      t.house_coordinates_.emplace_back(b);
    }
    for (auto const locations : ctx.string_to_location_) {
      auto idxs = t.string_to_location_.add_back_sized(locations.size());
      auto types = t.string_to_type_.add_back_sized(locations.size());
      for (auto i = 0U; i != locations.size(); ++i) {
        idxs[i] = locations[i].first;
        types[i] = locations[i].second;
      }
    }
    t.string_to_location_.resize(t.strings_.size());
    t.string_to_type_.resize(t.strings_.size());
    r.write(ctx);
    UTL_STOP_TIMING(copy_data);
    std::cout << "copy data timing: " << UTL_TIMING_MS(copy_data) << "\n";
  }

  {  // Assign place/street/housenumber coordinates to areas.
    auto timer = utl::scoped_timer{"coordinate to area mapping"};

    t.house_coordinates_.resize(t.street_names_.size());

    auto const geo_lookup = [&](std::basic_string<area_idx_t>& rtree_results,
                                coordinates const c) {
      rtree_results.clear();
      auto const l = osm::Location{c.lat_, c.lng_};
      auto const min = std::array<double, 2U>{l.lon(), l.lat()};
      rtree_search(
          areas, min.data(), nullptr,
          [](double const*, double const*, void const* item, void* udata) {
            auto const area = area_idx_t{static_cast<area_idx_t::value_t>(
                reinterpret_cast<std::intptr_t>(item))};
            auto const rtree_results_ptr =
                reinterpret_cast<std::basic_string<area_idx_t>*>(udata);
            rtree_results_ptr->push_back(area);
            return true;
          },
          &rtree_results);
      utl::sort(rtree_results);
      return rtree_results;
    };

    {
      auto place_areas = std::vector<std::basic_string<area_idx_t>>{};
      place_areas.resize(t.place_coordinates_.size());
      utl::parallel_for_run_threadlocal<std::basic_string<area_idx_t>>(
          t.place_coordinates_.size(),
          [&](std::basic_string<area_idx_t>& areas, std::size_t const i) {
            auto const c = t.place_coordinates_[place_idx_t{i}];
            geo_lookup(areas, c);
            area_db.eliminate_duplicates(t, c, areas);
            place_areas[i] = std::move(areas);
          });
      for (auto const& x : place_areas) {
        t.place_areas_.emplace_back(t.get_or_create_area_set(ctx, x));
      }
    }

    {
      auto street_areas =
          std::vector<cista::raw::vecvec<std::uint32_t, area_idx_t>>{};
      street_areas.resize(t.street_pos_.size());

      utl::parallel_for_run_threadlocal<std::basic_string<area_idx_t>>(
          t.street_pos_.size(),
          [&](std::basic_string<area_idx_t>& areas, std::size_t const i) {
            auto const street_idx = street_idx_t{i};
            auto const& coordinates = t.street_pos_[street_idx];

            for (auto const& c : coordinates) {
              geo_lookup(areas, c);
              area_db.eliminate_duplicates(t, c, areas);
              street_areas[i].emplace_back(areas);
            }
          });

      for (auto const& [i, x] : utl::enumerate(street_areas)) {
        t.street_areas_.add_back_sized(0);
        for (auto const& a : x) {
          t.street_areas_[street_idx_t{i}].push_back(t.get_or_create_area_set(
              ctx, std::basic_string_view<area_idx_t>{begin(a), end(a)}));
        }
      }
    }

    {
      auto house_areas =
          std::vector<cista::raw::vecvec<std::uint32_t, area_idx_t>>{};
      house_areas.resize(t.house_coordinates_.size());

      utl::parallel_for_run_threadlocal<std::basic_string<area_idx_t>>(
          t.house_coordinates_.size(),
          [&](std::basic_string<area_idx_t>& areas, std::size_t const i) {
            auto const street_idx = street_idx_t{i};
            auto const& coordinates = t.street_pos_[street_idx];

            for (auto const& c :
                 t.house_coordinates_[street_idx_t{street_idx}]) {
              geo_lookup(areas, c);
              area_db.eliminate_duplicates(t, c, areas);
              house_areas[i].emplace_back(areas);
            }
          });

      for (auto const& [i, x] : utl::enumerate(house_areas)) {
        t.house_areas_.add_back_sized(0);
        for (auto const& a : x) {
          t.house_areas_[street_idx_t{i}].push_back(t.get_or_create_area_set(
              ctx, std::basic_string_view<area_idx_t>{begin(a), end(a)}));
        }
      }
    }
  }

  {  // Finalize.
    UTL_START_TIMING(trigram_index);

    t.house_numbers_[street_idx_t{t.street_names_.size() - 1}];
    t.house_coordinates_[street_idx_t{t.street_names_.size() - 1}];

    ctx.string_lookup_ = {};
    ctx.street_lookup_ = {};
    ctx.street_names_ = {};

    t.build_ngram_index();

    UTL_STOP_TIMING(trigram_index);
    std::cout << "trigram index timing: " << UTL_TIMING_MS(trigram_index)
              << "\n";
  }

  {  // Write to disk.
    auto const timer = utl::scoped_timer{"write typeahead"};
    auto mmap =
        cista::buf{cista::mmap{(out_path.generic_string() + ".t.adr").c_str(),
                               cista::mmap::protection::WRITE}};
    cista::serialize<cista::mode::WITH_STATIC_VERSION>(mmap, t);
  }

  {  // Write reverse index to disk.
    auto const timer = utl::scoped_timer{"write reverse index"};
    auto mmap =
        cista::buf{cista::mmap{(out_path.generic_string() + ".r.adr").c_str(),
                               cista::mmap::protection::WRITE}};
    cista::serialize<cista::mode::WITH_STATIC_VERSION>(mmap, r);
  }
}

}  // namespace adr