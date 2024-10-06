#include "adr/area_database.h"

#include <ranges>

#include "cista/containers/nvec.h"

#include "utl/concat.h"
#include "utl/enumerate.h"
#include "utl/equal_ranges_linear.h"
#include "utl/erase_if.h"
#include "utl/helpers/algorithm.h"
#include "utl/parallel_for.h"
#include "utl/timer.h"
#include "utl/verify.h"

#include "geo/box.h"

#include "rtree.h"

#include "tg.h"

#include "adr/typeahead.h"

namespace osm = osmium;
namespace fs = std::filesystem;
namespace v = std::ranges::views;

namespace adr {

auto geojson = []() {
  auto str = std::string{};
  str.resize(512 * 1024 * 1024);
  return str;
}();

using inner_rings_t = mm_nvec<area_idx_t, coordinates, 3U>;
using outer_rings_t = mm_nvec<area_idx_t, coordinates, 2U>;

tg_ring* convert_ring(std::vector<tg_point>& ring_tmp, auto&& osm_ring) {
  ring_tmp.clear();
  for (auto const& p : osm_ring) {
    ring_tmp.emplace_back(tg_point{p.lon(), p.lat()});
  }
  return tg_ring_new(ring_tmp.data(), ring_tmp.size());
}

struct area_database::impl {
  impl(fs::path p, cista::mmap::protection const mode)
      : p_{std::move(p)},
        mode_{mode},
        rtree_{rtree_new()},
        outer_rings_{{mm_vec<std::uint64_t>{mm("outer_rings_idx_0.bin")},
                      mm_vec<std::uint64_t>{mm("outer_rings_idx_1.bin")}},
                     mm_vec<coordinates>{mm("outer_rings_data.bin")}},
        inner_rings_{{mm_vec<std::uint64_t>{mm("inner_rings_idx_0.bin")},
                      mm_vec<std::uint64_t>{mm("inner_rings_idx_1.bin")},
                      mm_vec<std::uint64_t>{mm("inner_rings_idx_2.bin")}},
                     mm_vec<coordinates>{mm("inner_rings_data.bin")}} {
    if (mode == cista::mmap::protection::READ) {
      struct tmp {
        std::vector<tg_point> ring_tmp_;
        std::vector<tg_ring*> inner_tmp_;
        std::vector<tg_poly*> polys_tmp_;
        std::basic_string<area_idx_t> areas_;
      };

      auto mutex = std::mutex{};

      idx_.resize(outer_rings_.size());
      utl::parallel_for_run_threadlocal<tmp>(
          outer_rings_.size(), [&](tmp& tmp, std::size_t const i) {
            tmp.polys_tmp_.clear();

            auto const area_idx = area_idx_t{i};
            auto const& outer_rings = outer_rings_[area_idx];

            auto box = geo::box{};
            for (auto const& [outer_idx, outer_ring] :
                 utl::enumerate(outer_rings)) {
              tmp.inner_tmp_.clear();
              for (auto const& inner_ring : inner_rings_[area_idx][outer_idx]) {
                tmp.inner_tmp_.emplace_back(
                    convert_ring(tmp.ring_tmp_, inner_ring));
              }

              for (auto const& c : outer_ring) {
                box.extend(c);
              }

              auto const outer = convert_ring(tmp.ring_tmp_, outer_ring);
              auto const poly = tg_poly_new(outer, tmp.inner_tmp_.data(),
                                            tmp.inner_tmp_.size());
              tmp.polys_tmp_.emplace_back(poly);
            }

            auto const min_corner = box.min_.lnglat();
            auto const max_corner = box.max_.lnglat();

            idx_[i] = tg_geom_new_multipolygon(tmp.polys_tmp_.data(),
                                               tmp.polys_tmp_.size());

            {
              auto const lock = std::scoped_lock{mutex};
              utl::concat(polys_to_free_, tmp.polys_tmp_);
              rtree_insert(rtree_, min_corner.data(), max_corner.data(),
                           reinterpret_cast<void*>(
                               static_cast<std::uintptr_t>(to_idx(area_idx))));
            }

            for (auto const& x : tmp.inner_tmp_) {
              tg_ring_free(x);
            }
          });
    }
  }

  ~impl() {
    for (auto const& p : polys_to_free_) {
      tg_poly_free(p);
    }
    for (auto const& mp : idx_) {
      tg_geom_free(mp);
    }
    rtree_free(rtree_);
  }

  cista::mmap mm(char const* file) {
    return cista::mmap{(p_ / file).generic_string().c_str(), mode_};
  }

  void lookup(typeahead const& t,
              coordinates const c,
              std::basic_string<area_idx_t>& rtree_results) const {
    auto const min = c.as_latlng().lnglat();
    rtree_results.clear();
    rtree_search(
        rtree_, min.data(), nullptr,
        [](double const*, double const*, void const* item, void* udata) {
          auto const area = area_idx_t{static_cast<area_idx_t::value_t>(
              reinterpret_cast<std::intptr_t>(item))};
          auto const rtree_results_ptr =
              reinterpret_cast<std::basic_string<area_idx_t>*>(udata);
          rtree_results_ptr->push_back(area);
          return true;
        },
        &rtree_results);
    utl::erase_if(rtree_results,
                  [&](area_idx_t const a) { return !is_within(c, a); });
    utl::sort(rtree_results, [&](area_idx_t const a, area_idx_t const b) {
      return t.area_admin_level_[a] < t.area_admin_level_[b];
    });
  }

  void add_area(area_idx_t const area_idx, osm::Area const& area) {
    polys_tmp_.clear();

    auto const nodes_to_coordinates = [](auto&& n) {
      return coordinates::from_location(n.location());
    };
    auto const ring_to_coordinates = [&](auto&& r) {
      return r | v::transform(nodes_to_coordinates);
    };

    assert(area_idx == outer_rings_.size());
    assert(area_idx == inner_rings_.size());
    outer_rings_.emplace_back(area.outer_rings() |
                              v::transform(ring_to_coordinates));
    inner_rings_.emplace_back(area.outer_rings() | v::transform([&](auto&& r) {
                                return area.inner_rings(r) |
                                       v::transform(ring_to_coordinates);
                              }));

    for (auto const& [outer_idx, outer_ring] :
         utl::enumerate(area.outer_rings())) {
      inner_tmp_.clear();
      for (auto const& inner_ring : area.inner_rings(outer_ring)) {
        inner_tmp_.emplace_back(convert_ring(ring_tmp_, inner_ring));
      }
      polys_tmp_.emplace_back(tg_poly_new(convert_ring(ring_tmp_, outer_ring),
                                          inner_tmp_.data(),
                                          inner_tmp_.size()));
    }

    idx_.emplace_back(
        tg_geom_new_multipolygon(polys_tmp_.data(), polys_tmp_.size()));

    for (auto const& outer : area.outer_rings()) {
      auto const outer_env = outer.envelope();
      auto const min_corner = std::array<double, 2U>{
          outer_env.bottom_left().lon(), outer_env.bottom_left().lat()};
      auto const max_corner = std::array<double, 2U>{
          outer_env.top_right().lon(), outer_env.top_right().lat()};

      rtree_insert(
          rtree_, min_corner.data(), max_corner.data(),
          reinterpret_cast<void*>(static_cast<std::size_t>(to_idx(area_idx))));
    }
  }

  bool is_within(coordinates const c, area_idx_t const area) const {
    auto const point = tg_geom_new_point(tg_point{c.lon(), c.lat()});
    auto const result = tg_geom_within(point, idx_[to_idx(area)]);
    tg_geom_free(point);
    return result;
  }

  std::filesystem::path p_;
  cista::mmap::protection mode_;

  rtree* rtree_;

  outer_rings_t outer_rings_;
  inner_rings_t inner_rings_;

  std::vector<tg_point> ring_tmp_;
  std::vector<tg_ring*> inner_tmp_;
  std::vector<tg_poly*> polys_tmp_;

  std::vector<tg_geom*> idx_;
  std::vector<tg_poly*> polys_to_free_;
};

area_database::area_database(std::filesystem::path p,
                             cista::mmap::protection const mode)
    : impl_{std::make_unique<area_database::impl>(std::move(p), mode)} {}

area_database::~area_database() = default;

void area_database::lookup(typeahead const& t,
                           coordinates const c,
                           std::basic_string<area_idx_t>& rtree_results) const {
  impl_->lookup(t, c, rtree_results);
}

void area_database::add_area(area_idx_t const area_idx, osm::Area const& area) {
  impl_->add_area(area_idx, area);
}

bool area_database::is_within(coordinates const c, area_idx_t area) const {
  return impl_->is_within(c, area);
}

}  // namespace adr