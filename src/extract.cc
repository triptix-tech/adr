#include "adr/adr.h"

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

#include "adr/typeahead.h"
#include "utl/zip.h"

namespace osm = osmium;
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
using namespace std::string_view_literals;

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

template <typename NodeFn, typename WayFn>
struct node_way_handler : public osmium::handler::Handler {
  explicit node_way_handler(NodeFn&& node_fn, WayFn&& way_fn)
      : node_fn_{std::move(node_fn)}, way_fn_{std::move(way_fn)} {}
  void way(osmium::Way const& w) { way_fn_(w); }
  void node(osmium::Node const& n) { node_fn_(n); }
  NodeFn node_fn_;
  WayFn way_fn_;
};

template <typename NodeFn, typename WayFn>
node_way_handler(NodeFn&&, WayFn&&)
    -> node_way_handler<std::decay_t<NodeFn>, std::decay_t<WayFn>>;

void extract(std::filesystem::path const& in_path,
             std::filesystem::path const& out_path) {
  auto pt = utl::get_active_progress_tracker();

  auto const filter = osm::TagsFilter{}
                          .add_rule(true, "boundary", "postal_code")
                          .add_rule(true, "boundary", "administrative")
                          .add_rule(true, "admin_level");

  auto const in = osm::io::File{in_path.string()};

  auto polygon_mgr =
      osm::area::MultipolygonManager<osm::area::Assembler>{{}, filter};
  osm::relations::read_relations(in, polygon_mgr);

  auto reader = osm::io::Reader{
      in, osm::osm_entity_bits::node | osm::osm_entity_bits::way,
      osm::io::read_meta::no};
  auto index = location_index_t{};
  auto location_handler = location_handler_t{index};

  auto t = typeahead{};
  auto areas = std::vector<area_rtree_value>{};
  osm::apply(
      reader, [&](auto&&) { pt->update(reader.offset()); }, location_handler,
      polygon_mgr.handler([&](auto&& buffer) {
        osm::apply(buffer, area_handler{[&](osmium::Area const& a) {
                     auto const env = a.envelope();
                     if (!env.valid()) {
                       return;
                     }

                     auto const admin_area_idx = t.add_admin_area(a.tags());
                     auto const postal_code_area_idx =
                         t.add_postal_code_area(a.tags());

                     auto const bbox =
                         box{{env.bottom_left().x(), env.bottom_left().y()},
                             {env.top_right().x(), env.top_right().y()}};
                     auto env_box = box{};
                     bg::envelope(bbox, env_box);

                     if (admin_area_idx != area_idx_t::invalid()) {
                       areas.emplace_back(bbox, admin_area_idx);
                     }

                     if (postal_code_area_idx != area_idx_t::invalid()) {
                       areas.emplace_back(bbox, postal_code_area_idx);
                     }
                   }});
      }),
      node_way_handler{
          [&](osm::Node const& n) {
            auto const& tags = n.tags();
            if (!tags.has_tag("emergency", "fire_hydrant") &&
                !tags.has_key("public_transport") && /* stops from timetable */
                !tags.has_key("highway") && /* named motorway_junction */
                !tags.has_key("traffic_sign")) {
              t.add_address(tags, n.location());
              t.add_place(n.id(), tags, n.location());
            }
          },
          [&](osm::Way const& w) {
            if (!w.nodes().empty()) {
              auto const& tags = w.tags();
              if (!tags.has_key("public_transport") &&
                  !tags.has_tag("amenity", "toilets")) {
                t.add_address(tags, w.nodes().front().location());
                t.add_place(std::numeric_limits<std::uint64_t>::max(), tags,
                            w.nodes().front().location());
                t.add_street(tags, w.nodes().front().location());
              }
            }
          }});

  auto rtree_results = std::basic_string<area_idx_t>{};
  auto area_bbox_rtree = rtree{areas};

  for (auto const& c : t.place_coordinates_) {
    rtree_results.clear();
    area_bbox_rtree.query(
        bgi::covers(point{c.lat_, c.lng_}),
        boost::make_function_output_iterator(
            [&](auto&& entry) { rtree_results.push_back(entry.second); }));
    utl::sort(rtree_results);
    t.place_areas_.emplace_back(t.get_or_create_area_set(rtree_results));
  }

  for (auto const& [street_idx, coordinates] :
       utl::enumerate(t.street_coordinates_)) {
    t.street_areas_.add_back_sized(0);
    for (auto const& c : coordinates) {
      rtree_results.clear();
      area_bbox_rtree.query(
          bgi::covers(point{c.lat_, c.lng_}),
          boost::make_function_output_iterator(
              [&](auto&& entry) { rtree_results.push_back(entry.second); }));
      utl::sort(rtree_results);
      t.street_areas_[street_idx_t{street_idx}].push_back(
          t.get_or_create_area_set(rtree_results));
    }
  }
  t.house_numbers_[street_idx_t{t.street_names_.size() - 1}];
  t.house_coordinates_[street_idx_t{t.street_names_.size() - 1}];

  t.area_set_lookup_ = {};
  t.string_lookup_ = {};
  t.street_lookup_ = {};

  constexpr auto const admin_str =
      std::array<std::string_view, 12>{"0",
                                       "1",
                                       "2",
                                       "region"sv,
                                       "state"sv,
                                       "district",
                                       "county",
                                       "municipality",
                                       "town",
                                       "subtownship",
                                       "neighbourhood",
                                       "zip"};

  auto const print_area = [&](area_idx_t const area) {
    auto const admin_lvl = t.area_admin_level_[area];
    std::cout << "      name=" << t.strings_[t.area_names_[area]].view()
              << ", admin_lvl="
              << (admin_lvl >= admin_str.size()
                      ? fmt::to_string(to_idx(admin_lvl))
                      : admin_str[to_idx(admin_lvl)])
              << "\n";
  };

  auto const print_coordinate_and_areas = [&](coordinates const& c,
                                              area_set_idx_t const area_set) {
    std::cout << "    " << osmium::Location{c.lat_, c.lng_} << "\n";

    for (auto const& area : t.area_sets_[area_set]) {
      auto const admin_lvl = t.area_admin_level_[area];
      std::cout << "      name=" << t.strings_[t.area_names_[area]].view()
                << ", admin_lvl="
                << (admin_lvl >= admin_str.size()
                        ? fmt::to_string(to_idx(admin_lvl))
                        : admin_str[to_idx(admin_lvl)])
                << "\n";
    }
  };

  auto const print_street = [&](street_idx_t const street_idx) {
    auto const name_idx = t.street_names_[street_idx];
    auto const area_sets = t.street_areas_[street_idx];
    auto const coordinates = t.street_coordinates_[street_idx];
    auto const house_numbers = t.house_numbers_[street_idx];
    auto const house_coordinates = t.house_coordinates_[street_idx];

    std::cout << t.strings_[name_idx].view() << ":";

    std::cout << "  HOUSE NUMBERS:\n";
    for (auto const [house_number, house_coordinates] :
         utl::zip(house_numbers, house_coordinates)) {
      std::cout << "    " << t.strings_[house_number].view() << ", "
                << osmium::Location{house_coordinates.lat_,
                                    house_coordinates.lng_}
                << "\n";
    }

    std::cout << "  COORDINATES:\n";
    for (auto const& [c, area_set] : utl::zip(coordinates, area_sets)) {
      print_coordinate_and_areas(c, area_set);
    }
  };

  //  for (auto const& [name_idx, area_sets, coordinates, house_numbers,
  //                    house_coordinates] :
  //       utl::zip(t.street_names_, t.street_areas_, t.street_coordinates_,
  //                t.house_numbers_, t.house_coordinates_)) {
  //    std::cout << t.strings_[name_idx].view() << ": ";
  //
  //    std::cout << "  locations: " << coordinates.size() << ", "
  //              << area_sets.size() << "\n";
  //    for (auto const& [c, area_set] : utl::zip(coordinates, area_sets)) {
  //      std::cout << "    " << osmium::Location{c.lat_, c.lng_} << "\n";
  //
  //      for (auto const& area : t.area_sets_[area_set]) {
  //        print_area(area);
  //      }
  //    }
  //  }

  for (auto const [str, locations] :
       utl::zip(t.strings_, t.string_to_location_)) {
    std::cout << "STR=" << str.view() << "\n";
    for (auto const& [l, type] : locations) {
      switch (type) {
        case location_type_t::kStreet:
          //          std::cout << "  STREET\n";
          //          print_street(street_idx_t{l});
          //          break;

        case location_type_t::kPlace: {
          std::cout << "  PLACE " << l << " ["
                    << t.place_osm_ids_[place_idx_t{l}] << "] name=\""
                    << t.strings_[t.place_names_[place_idx_t{l}]].view()
                    << "\"\n";
          print_coordinate_and_areas(t.place_coordinates_[place_idx_t{l}],
                                     t.place_areas_[place_idx_t{l}]);
          break;
        }

        case location_type_t::kHouseNumber: break;

        case location_type_t::kArea:
          //          std::cout << "  AREA\n";
          //          print_area(area_idx_t{l});
          //          break;
      }
    }
  }
}

}  // namespace adr