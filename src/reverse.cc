#include "adr/reverse.h"

#include <iostream>

#include "rtree.h"

#include "utl/enumerate.h"

#include "adr/import_context.h"
#include "adr/typeahead.h"

namespace adr {

void reverse::add_street(import_context& ctx,
                         street_idx_t const street,
                         osmium::Way const& way) {
  auto const lock = std::lock_guard{ctx.reverse_mutex_};

  ctx.street_segments_.resize(
      std::max(ctx.street_segments_.size(),
               static_cast<std::size_t>(to_idx(street) + 1U)));
  auto bucket =
      ctx.street_segments_[to_idx(street)].add_back_sized(way.nodes().size());
  for (auto const [i, n] : utl::enumerate(way.nodes())) {
    bucket[i] = coordinates{n.x(), n.y()};
  }
}

void reverse::write(import_context& ctx) {
  for (auto const& street_segments : ctx.street_segments_) {
    street_segments_.emplace_back(street_segments);
  }
  ctx.street_segments_.clear();
}

rtree* reverse::build_rtree(typeahead const& t) const {
  auto rt = rtree_new();

  // Add street segment bounding boxes.
  for (auto const [street, segments] : utl::enumerate(street_segments_)) {
    for (auto const [segment_idx, segment] : utl::enumerate(segments)) {
      auto b = osmium::Box{};
      for (auto const& c : segment) {
        b.extend(osmium::Location{c.lat_, c.lng_});
      }

      auto const min_corner =
          std::array<double, 2U>{b.bottom_left().lon(), b.bottom_left().lat()};
      auto const max_corner =
          std::array<double, 2U>{b.top_right().lon(), b.top_right().lat()};

      rtree_insert(
          rt, min_corner.data(), max_corner.data(),
          rtree_entity{
              .street_segment_ = {.type_ = entity_type::kStreet,
                                  .segment_ =
                                      static_cast<std::uint16_t>(segment_idx),
                                  .street_ = street_idx_t{street}}}
              .to_data());
    }
  }

  // Add place coordinates.
  for (auto const [place, c] : utl::enumerate(t.place_coordinates_)) {
    auto const min = osmium::Location{c.lat_, c.lng_};
    auto const min_corner = std::array<double, 2U>{min.lon(), min.lat()};
    rtree_insert(rt, min_corner.data(), nullptr,
                 rtree_entity{.place_ = {.type_ = entity_type::kPlace,
                                         .place_ = place_idx_t{place}}}
                     .to_data());
  }

  // Add house numbers.
  for (auto const [street, house_numbers] :
       utl::enumerate(t.house_coordinates_)) {
    auto const street_idx = street_idx_t{street};

    for (auto const [hn_idx, c] : utl::enumerate(house_numbers)) {
      auto const min = osmium::Location{c.lat_, c.lng_};
      auto const min_corner = std::array<double, 2U>{min.lon(), min.lat()};
      rtree_insert(
          rt, min_corner.data(), nullptr,
          rtree_entity{.hn_ = {.type_ = entity_type::kHouseNumber,
                               .idx_ = static_cast<std::uint16_t>(hn_idx),
                               .street_ = street_idx}}
              .to_data());
    }
  }

  return rt;
}

}  // namespace adr