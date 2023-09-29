#include "adr/area_database.h"

#include "boost/geometry/algorithms/within.hpp"
#include "boost/geometry/core/cs.hpp"
#include "boost/geometry/geometries/register/multi_polygon.hpp"
#include "boost/geometry/geometries/register/point.hpp"
#include "boost/geometry/geometries/register/ring.hpp"

#include "cista/containers/nvec.h"

#include "utl/enumerate.h"
#include "utl/equal_ranges_linear.h"
#include "utl/verify.h"

#include "tg.h"

#include "adr/typeahead.h"

namespace osm = osmium;

namespace adr {

using inner_rings_t = data::nvec<area_idx_t, coordinates, 3U>;
using outer_rings_t = data::nvec<area_idx_t, coordinates, 2U>;
using ring_t = cista::const_bucket<inner_rings_t::data_vec_t,
                                   inner_rings_t::index_vec_t,
                                   inner_rings_t::size_type>;

struct polygon {
  using point_type = coordinates;
  using ring_type = ring_t;
  using inner_container_type =
      cista::const_meta_bucket<1U,
                               inner_rings_t::data_vec_t,
                               inner_rings_t::index_vec_t,
                               inner_rings_t::size_type>;

  ring_type outer() const { return outer_; }
  inner_container_type inners() const { return inners_; }

  ring_type outer_;
  inner_container_type inners_;
};

using multi_polygon = std::vector<polygon>;

}  // namespace adr

BOOST_GEOMETRY_REGISTER_POINT_2D(
    adr::coordinates, std::int32_t, boost::geometry::cs::cartesian, lat_, lng_);
BOOST_GEOMETRY_REGISTER_RING(adr::ring_t)
BOOST_GEOMETRY_REGISTER_MULTI_POLYGON(adr::multi_polygon)

namespace boost::geometry::traits {

template <>
struct tag<adr::polygon> {
  using type = polygon_tag;
};

template <>
struct ring_const_type<adr::polygon> {
  using type = adr::ring_t;
};

template <>
struct ring_mutable_type<adr::polygon> {
  using type = adr::ring_t;
};

template <>
struct interior_const_type<adr::polygon> {
  using type = adr::polygon::inner_container_type;
};

template <>
struct interior_mutable_type<adr::polygon> {
  using type = adr::polygon::inner_container_type;
};

template <>
struct exterior_ring<adr::polygon> {
  static adr::polygon::ring_type get(adr::polygon const& p) {
    return p.outer();
  }
};

template <>
struct interior_rings<adr::polygon> {
  static adr::polygon::inner_container_type get(adr::polygon const& p) {
    return p.inners();
  }
};

template <>
struct point_order<adr::ring_t> {
  static const order_selector value = counterclockwise;
};

template <>
struct closure<adr::ring_t> {
  static const closure_selector value = closed;
};

}  // namespace boost::geometry::traits

namespace adr {

struct area_database::impl {
  using ring_t = std::vector<coordinates>;

  ~impl() {
    for (auto const& mp : idx_) {
      tg_geom_free(mp);
    }
  }

  void add_area(area_idx_t const area_idx, osm::Area const& area) {
    auto const convert_ring = [&](auto&& osm_ring) {
      ring_tmp_.clear();
      for (auto const& p : osm_ring) {
        ring_tmp_.emplace_back(tg_point{.x = p.lat(), .y = p.lon()});
      }
      return tg_ring_new(ring_tmp_.data(), ring_tmp_.size());
    };

    polys_tmp_.clear();

    for (auto const& [outer_idx, outer_ring] :
         utl::enumerate(area.outer_rings())) {
      inner_tmp_.clear();
      for (auto const& inner_ring : area.inner_rings(outer_ring)) {
        inner_tmp_.emplace_back(convert_ring(inner_ring));
      }
      polys_tmp_.emplace_back(tg_poly_new(
          convert_ring(outer_ring), inner_tmp_.data(), inner_tmp_.size()));
    }

    idx_.emplace_back(
        tg_geom_new_multipolygon(polys_tmp_.data(), polys_tmp_.size()));
  }

  bool is_within(coordinates const c, area_idx_t const area) {
    auto const l = osmium::Location{c.lat_, c.lng_};
    auto const point = tg_geom_new_point(tg_point{l.lat(), l.lon()});
    auto const result = tg_geom_within(point, idx_[to_idx(area)]);
    tg_geom_free(point);
    return result;
  }

  std::vector<tg_point> ring_tmp_;
  std::vector<tg_ring*> inner_tmp_;
  std::vector<tg_poly*> polys_tmp_;

  std::vector<tg_geom*> idx_;
};

area_database::area_database()
    : impl_{std::make_unique<area_database::impl>()} {}

area_database::~area_database() = default;

void area_database::add_area(area_idx_t const area_idx, osm::Area const& area) {
  impl_->add_area(area_idx, area);
}

bool area_database::is_within(coordinates const c, area_idx_t area) const {
  return impl_->is_within(c, area);
}

void area_database::eliminate_duplicates(typeahead const& t,
                                         coordinates const c,
                                         std::basic_string<area_idx_t>& areas) {
  std::sort(begin(areas), end(areas),
            [&](area_idx_t const a, area_idx_t const b) {
              return t.area_admin_level_[a] < t.area_admin_level_[b];
            });

  auto const adm = [&](auto&& i) { return t.area_admin_level_[areas[i]]; };

  auto i = 0U;
  auto out_i = 0U;
  auto last_written = admin_level_t::invalid();
  while (i < areas.size()) {
    auto const is_duplicate =
        (i != areas.size() - 1U && adm(i) == adm(i + 1U)) ||
        (last_written == adm(i));

    if (is_duplicate && !is_within(c, areas[i])) {
      ++i;
      continue;
    }

    last_written = adm(i);
    areas[out_i] = areas[i];
    ++i;
    ++out_i;
  }

  areas.resize(out_i);
}

}  // namespace adr