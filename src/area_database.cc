#include "adr/area_database.h"

#include "cista/containers/nvec.h"

#include "boost/geometry/algorithms/within.hpp"
#include "boost/geometry/core/cs.hpp"
#include "boost/geometry/geometries/register/multi_polygon.hpp"
#include "boost/geometry/geometries/register/point.hpp"
#include "boost/geometry/geometries/register/ring.hpp"

#include "adr/typeahead.h"
#include "utl/enumerate.h"
#include "utl/equal_ranges_linear.h"
#include "utl/verify.h"

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

  void add_area(area_idx_t const area_idx, osm::Area const& area) {
    utl::verify(outers_.size() == to_idx(area_idx),
                "area_db: n_outers={}, area_idx={}", outers_.size(), area_idx);
    utl::verify(inners_.size() == to_idx(area_idx),
                "area_db: n_inners={}, area_idx={}", inners_.size(), area_idx);

    outer_tmp_.clear();
    inner_tmp_.clear();

    outer_tmp_.resize(area.outer_rings().size());
    inner_tmp_.resize(area.outer_rings().size());

    for (auto const& [outer_idx, outer_ring] :
         utl::enumerate(area.outer_rings())) {
      for (auto const& p : outer_ring) {
        outer_tmp_[outer_idx].emplace_back(coordinates{p.x(), p.y()});
      }

      inner_tmp_[outer_idx].resize(area.inner_rings(outer_ring).size());
      for (auto const& [inner_idx, inner_ring] :
           utl::enumerate(area.inner_rings(outer_ring))) {
        for (auto const& p : inner_ring) {
          inner_tmp_[outer_idx][inner_idx].emplace_back(
              coordinates{p.x(), p.y()});
        }
      }
    }

    outers_.emplace_back(outer_tmp_);
    inners_.emplace_back(inner_tmp_);

    utl::verify(outers_.size() == inners_.size(),
                "size mismatch:  area_idx={}, n_outers={}, n_inners={}",
                area_idx, outers_.size(), inners_.size());
    utl::verify(outers_.back().size() == inners_.back().size(),
                "size mismatch: area_idx={}, outers.size[{}/{}]={}, "
                "inners[{}/{}].size={}",
                area_idx, outers_.size() - 1U, outers_.size(),
                outers_.back().size(), inners_.size() - 1U, inners_.size(),
                inners_.back().size());
    utl::verify(outers_.size() == to_idx(area_idx) + 1U,
                "area_db: n_outers={}, area_idx={}", outers_.size(), area_idx);
    utl::verify(inners_.size() == to_idx(area_idx) + 1U,
                "area_db: n_inners={}, area_idx={}", inners_.size(), area_idx);
  }

  multi_polygon get(area_idx_t const area) const {
    auto mp = multi_polygon{};
    utl::verify(outers_[area].size() == inners_[area].size(),
                "size mismatch: n_outers={}, n_inners={}", outers_[area].size(),
                inners_[area].size());
    auto const size = outers_[area].size();
    for (auto i = 0U; i != size; ++i) {
      auto p = polygon{outers_[area][i], inners_[area][i]};
      mp.emplace_back(p);
    }
    return mp;
  }

  std::vector<ring_t> outer_tmp_;
  std::vector<std::vector<ring_t>> inner_tmp_;

  data::nvec<area_idx_t, coordinates, 2U> outers_;
  data::nvec<area_idx_t, coordinates, 3U> inners_;
};

area_database::area_database()
    : impl_{std::make_unique<area_database::impl>()} {}

area_database::~area_database() = default;

void area_database::add_area(area_idx_t const area_idx, osm::Area const& area) {
  impl_->add_area(area_idx, area);
}

bool area_database::is_within(coordinates const c, area_idx_t area) const {
  return boost::geometry::within(c, impl_->get(area));
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