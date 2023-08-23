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

#include "utl/progress_tracker.h"

#include "adr/typeahead.h"

namespace osm = osmium;
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

  auto invalid = 0U, valid = 0U;
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
      node_way_handler{[&](osm::Node const& n) {
                         auto const& tags = n.tags();
                         if (!tags.has_tag("emergency", "fire_hydrant")) {
                           t.add_address(tags, n.location());
                           t.add_place(tags, n.location());
                         }
                       },
                       [&](osm::Way const& w) {
                         if (!w.nodes().empty()) {
                           auto const& tags = w.tags();
                           t.add_address(tags, w.nodes().front().location());
                           t.add_place(tags, w.nodes().front().location());
                         }
                       }});

  std::cout << "invalid=" << invalid << ", valid=" << valid << "\n";

  auto rtree_results = std::vector<area_idx_t>{};
  auto area_bbox_rtree = rtree{areas};

  for (auto const& c : t.place_coordinates_) {
    rtree_results.clear();
    area_bbox_rtree.query(
        bgi::covers(point{c.lat_, c.lng_}),
        boost::make_function_output_iterator(
            [&](auto&& entry) { rtree_results.emplace_back(entry.second); }));
    t.place_areas_.emplace_back(rtree_results);
  }

  for (auto const& c : t.street_coordinates_) {
    rtree_results.clear();
    area_bbox_rtree.query(
        bgi::covers(point{c.lat_, c.lng_}),
        boost::make_function_output_iterator(
            [&](auto&& entry) { rtree_results.emplace_back(entry.second); }));
    t.street_areas_.emplace_back(rtree_results);
  }
}

}  // namespace adr