#include <cista/mmap.h>

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/box.hpp"
#include "boost/geometry/geometries/point.hpp"
#include "boost/iterator/function_output_iterator.hpp"

#include "osmium/area/assembler.hpp"
#include "osmium/area/multipolygon_manager.hpp"
#include "osmium/handler/node_locations_for_ways.hpp"
#include "osmium/index/map/flex_mem.hpp"
#include "osmium/io/pbf_input.hpp"

#include "utl/enumerate.h"
#include "utl/helpers/algorithm.h"
#include "utl/progress_tracker.h"
#include "utl/timing.h"
#include "utl/zip.h"

#include "tiles/osm/hybrid_node_idx.h"
#include "tiles/osm/tmp_file.h"
#include "tiles/util_parallel.h"

#include "adr/typeahead.h"

namespace osm = osmium;
namespace osm_io = osmium::io;
namespace osm_rel = osmium::relations;
namespace osm_eb = osmium::osm_entity_bits;
namespace osm_area = osmium::area;
namespace osm_mem = osmium::memory;
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace adr {

using point = bg::model::point<std::int32_t, 2, bg::cs::cartesian>;
using location_index_t =
    osm::index::map::FlexMem<osm::unsigned_object_id_type, osm::Location>;
using location_handler_t = osm::handler::NodeLocationsForWays<location_index_t>;

using box = bg::model::box<point>;
using area_rtree_value = std::pair<box, adr::area_idx_t>;
using rtree = bgi::rtree<area_rtree_value, bgi::rstar<16>>;

template <typename Fn>
struct area_handler : public osmium::handler::Handler {
  explicit area_handler(Fn&& fn) : fn_(std::move(fn)) {}
  void area(osmium::Area const& a) { fn_(a); }
  Fn fn_;
};

template <typename Fn>
area_handler(Fn&&) -> area_handler<std::decay_t<Fn>>;

struct feature_handler : public osmium::handler::Handler {
  feature_handler(typeahead& t,
                  import_context& ctx,
                  std::vector<area_rtree_value>& areas)
      : t_{t}, ctx_{ctx}, areas_{areas} {}

  void way(osmium::Way const& w) {
    if (!w.nodes().empty()) {
      auto const& tags = w.tags();
      if (!tags.has_key("public_transport") &&
          !tags.has_tag("amenity", "toilets") &&
          !tags.has_tag("building", "industrial") &&
          !tags.has_tag("leisure", "playground") &&
          !tags.has_tag("access", "false") &&
          !tags.has_tag("amenity", "taxi")) {
        tags.has_key("highway")
            ? t_.add_street(ctx_, tags, w.nodes().front().location())
            : t_.add_place(ctx_, w.id(), true, tags,
                           w.nodes().front().location());
        t_.add_address(ctx_, tags, w.nodes().front().location());
      }
    }
  }

  void node(osmium::Node const& n) {
    auto const& tags = n.tags();
    if (!tags.has_tag("emergency", "fire_hydrant") &&
        !tags.has_key("public_transport") && /* stops from timetable */
        !tags.has_key("highway") && /* named motorway_junction */
        !tags.has_key("traffic_sign") &&
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
    if (!env.valid()) {
      return;
    }

    auto const admin_area_idx = t_.add_admin_area(ctx_, a.tags());
    auto const postal_code_area_idx = t_.add_postal_code_area(ctx_, a.tags());

    auto const bbox = box{{env.bottom_left().x(), env.bottom_left().y()},
                          {env.top_right().x(), env.top_right().y()}};
    auto env_box = box{};
    bg::envelope(bbox, env_box);

    if (admin_area_idx != area_idx_t::invalid()) {
      areas_.emplace_back(bbox, admin_area_idx);
    }

    if (postal_code_area_idx != area_idx_t::invalid()) {
      areas_.emplace_back(bbox, postal_code_area_idx);
    }
  }

  typeahead& t_;
  import_context& ctx_;
  std::vector<area_rtree_value>& areas_;
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

  auto const node_idx_file = tiles::tmp_file{
      (boost::filesystem::path{tmp_dname} / "idx.bin").generic_string()};
  auto const node_dat_file = tiles::tmp_file{
      (boost::filesystem::path{tmp_dname} / "dat.bin").generic_string()};
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
  auto areas = std::vector<area_rtree_value>{};
  auto mp_queue = tiles::in_order_queue<osm_mem::Buffer>{};
  {  // Extract streets, places, and areas.
    pt->status("Load OSM / Pass 2");
    auto const thread_count =
        std::max(2, static_cast<int>(std::thread::hardware_concurrency()));

    // poor mans thread local (we dont know the threads themselves)
    std::atomic_size_t next_handlers_slot{0};
    auto handlers = std::vector<std::pair<std::thread::id, feature_handler>>{};
    handlers.reserve(thread_count);
    for (auto i = 0; i < thread_count; ++i) {
      handlers.emplace_back(std::thread::id{}, feature_handler{t, ctx, areas});
    }
    auto const get_handler = [&]() -> feature_handler& {
      auto const thread_id = std::this_thread::get_id();
      if (auto it = std::find_if(
              begin(handlers), end(handlers),
              [&](auto const& pair) { return pair.first == thread_id; });
          it != end(handlers)) {
        return it->second;
      }
      auto slot = next_handlers_slot.fetch_add(1);
      utl::verify(slot < handlers.size(), "more threads than expected");
      handlers[slot].first = thread_id;
      return handlers[slot].second;
    };

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
            osm::apply(buf, get_handler());

            mp_queue.process_in_order(idx, std::move(buf), [&](auto buf2) {
              osm::apply(buf2, mp_manager.handler([&](auto&& mp_buffer) {
                auto p = std::make_shared<osm_mem::Buffer>(
                    std::forward<decltype(mp_buffer)>(mp_buffer));
                pool.submit(
                    [p, &get_handler] { osm::apply(*p, get_handler()); });
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
    UTL_STOP_TIMING(copy_data);
    std::cout << "copy data timing: " << UTL_TIMING_MS(copy_data) << "\n";
  }

  {  // Assign place/street/housenumber coordinates to areas.
    UTL_START_TIMING(coordinate_areas);

    t.house_coordinates_.resize(t.street_names_.size());

    auto rtree_results = std::basic_string<area_idx_t>{};
    auto area_bbox_rtree = rtree{areas};

    auto const geo_lookup =
        [&](coordinates const c) -> std::basic_string<area_idx_t> const& {
      rtree_results.clear();
      area_bbox_rtree.query(
          bgi::covers(point{c.lat_, c.lng_}),
          boost::make_function_output_iterator(
              [&](auto&& entry) { rtree_results.push_back(entry.second); }));
      utl::sort(rtree_results);
      return rtree_results;
    };

    for (auto const& c : t.place_coordinates_) {
      t.place_areas_.emplace_back(t.get_or_create_area_set(ctx, geo_lookup(c)));
    }

    for (auto const& [street_idx, coordinates] :
         utl::enumerate(t.street_pos_)) {
      t.street_areas_.add_back_sized(0);
      for (auto const& c : coordinates) {
        t.street_areas_[street_idx_t{street_idx}].push_back(
            t.get_or_create_area_set(ctx, geo_lookup(c)));
      }

      assert(t.house_areas_.size() == street_idx);
      t.house_areas_.add_back_sized(0);
      for (auto const& c : t.house_coordinates_[street_idx_t{street_idx}]) {
        t.house_areas_[street_idx_t{street_idx}].push_back(
            t.get_or_create_area_set(ctx, geo_lookup(c)));
      }
    }

    UTL_STOP_TIMING(coordinate_areas);
    std::cout << "coordinate area timing: " << UTL_TIMING_MS(coordinate_areas)
              << "\n";
  }

  {  // Finalize.
    UTL_START_TIMING(trigram_index);

    t.house_numbers_[street_idx_t{t.street_names_.size() - 1}];
    t.house_coordinates_[street_idx_t{t.street_names_.size() - 1}];

    ctx.string_lookup_ = {};
    ctx.street_lookup_ = {};
    ctx.street_names_ = {};

    t.build_trigram_index();

    UTL_STOP_TIMING(trigram_index);
    std::cout << "trigram index timing: " << UTL_TIMING_MS(trigram_index)
              << "\n";
  }

  {  // Write to disk.
    auto mmap = cista::buf{
        cista::mmap{out_path.string().c_str(), cista::mmap::protection::WRITE}};
    cista::serialize<cista::mode::WITH_STATIC_VERSION>(mmap, t);
  }
}

}  // namespace adr