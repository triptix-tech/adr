#include "adr/reverse.h"

#include <iostream>

#include "rtree.h"

#include "geo/polyline.h"

#include "utl/enumerate.h"
#include "utl/pairwise.h"
#include "utl/timer.h"

#include "adr/guess_context.h"
#include "adr/import_context.h"
#include "adr/typeahead.h"
#include "geo/box.h"

namespace fs = std::filesystem;

namespace adr {

static_assert(sizeof(rtree_entity) == sizeof(void*));

reverse::reverse(fs::path p, cista::mmap::protection const mode)
    : p_{p},
      mode_{mode},
      street_segments_{{mm_vec<std::uint64_t>{mm("street_segments_idx_0.bin")},
                        mm_vec<std::uint64_t>{mm("street_segments_idx_1.bin")}},
                       mm_vec<coordinates>{mm("street_segments_data.bin")}},
      rtree_{mode == cista::mmap::protection::READ
                 ? *cista::read<reverse::rtree_t::meta>(p_ / "rtree_meta.bin")
                 : reverse::rtree_t::meta{},
             reverse::rtree_t::vector_t{mm("rtree_data.bin")}} {}

cista::mmap reverse::mm(char const* file) {
  return cista::mmap{(p_ / file).generic_string().c_str(), mode_};
}

std::vector<suggestion> reverse::lookup(typeahead const& t,
                                        geo::latlng const& query,
                                        std::size_t const n_guesses,
                                        bool only_external_places) const {
  auto const b = geo::box{query, 500.0};
  auto const min = b.min_.lnglat_float();
  auto const max = b.max_.lnglat_float();

  auto suggestions = std::vector<suggestion>{};
  rtree_.search(
      min, max,
      [&](reverse::rtree_t::coord_t const& min,
          reverse::rtree_t::coord_t const& max, rtree_entity const& e) {
        switch (e.type_) {
          case adr::entity_type::kHouseNumber: {
            if (only_external_places) {
              return true;
            }
            auto const& hn = e.hn_;
            auto const c = t.house_coordinates_[hn.street_][hn.idx_];
            suggestions.emplace_back(adr::suggestion{
                .str_ = t.street_names_[hn.street_][adr::kDefaultLangIdx],
                .location_ = adr::address{.street_ = hn.street_,
                                          .house_number_ = hn.idx_},
                .coordinates_ = c,
                .area_set_ = t.house_areas_[hn.street_][hn.idx_],
                .matched_area_lang_ = {adr::kDefaultLangIdx} /* TODO */,
                .matched_areas_ =
                    std::numeric_limits<decltype(std::declval<adr::suggestion>()
                                                     .matched_areas_)>::max(),
                .matched_tokens_ = 0U,
                .score_ = static_cast<float>(geo::distance(query, c)) - 10.F});
          } break;

          case adr::entity_type::kPlace: {
            if (only_external_places
                && t.place_type_[e.place_.place_] != place_type::kExtra) {
              return true;
            }
            auto const& p = e.place_;
            auto const c = t.place_coordinates_[p.place_];
            suggestions.emplace_back(adr::suggestion{
                .str_ = t.place_names_[p.place_][adr::kDefaultLangIdx],
                .location_ = p.place_,
                .coordinates_ = c,
                .area_set_ = t.place_areas_[p.place_],
                .matched_area_lang_ = {adr::kDefaultLangIdx} /* TODO */,
                .matched_areas_ =
                    std::numeric_limits<decltype(std::declval<adr::suggestion>()
                                                     .matched_areas_)>::max(),
                .matched_tokens_ = 0U,
                .score_ = static_cast<float>(geo::distance(query, c)) - 10.F});
          } break;

          case adr::entity_type::kStreet: {
            if (only_external_places) {
              return true;
            }
            auto const& s = e.street_segment_;
            auto const [dist, closest, _] = geo::distance_to_polyline(
                query, street_segments_[s.street_][s.segment_]);
            suggestions.emplace_back(adr::suggestion{
                .str_ = t.street_names_[s.street_][adr::kDefaultLangIdx],
                .location_ =
                    adr::address{.street_ = s.street_,
                                 .house_number_ = adr::address::kNoHouseNumber},
                .coordinates_ = adr::coordinates::from_latlng(closest),
                .area_set_ = t.street_areas_[s.street_][0U] /* TODO */,
                .matched_area_lang_ = {adr::kDefaultLangIdx} /* TODO */,
                .matched_areas_ =
                    std::numeric_limits<decltype(std::declval<adr::suggestion>()
                                                     .matched_areas_)>::max(),
                .matched_tokens_ = 0U,
                .score_ = static_cast<float>(dist)});
            break;
          }
        }

        return true;
      });

  utl::nth_element(suggestions, 10U);
  suggestions.resize(10U);
  utl::sort(suggestions);

  return suggestions;
}

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
    bucket[i] = coordinates::from_location(n.location());
  }
}

void reverse::write(import_context& ctx) {
  for (auto const& street_segments : ctx.street_segments_) {
    street_segments_.emplace_back(street_segments);
  }
  ctx.street_segments_.clear();
}

void reverse::build_rtree(typeahead const& t) {
  auto const timer = utl::scoped_timer{"build rtree"};

  // Add street segment bounding boxes.
  for (auto const [street, segments] : utl::enumerate(street_segments_)) {
    for (auto const [segment_idx, segment] : utl::enumerate(segments)) {
      auto b = geo::box{};
      for (auto const& c : segment) {
        b.extend(c);
      }

      auto const min_corner = b.min_.lnglat_float();
      auto const max_corner = b.max_.lnglat_float();
      auto const entry =
          rtree_entity{.street_segment_ = {
                           .type_ = entity_type::kStreet,
                           .segment_ = static_cast<std::uint16_t>(segment_idx),
                           .street_ = street_idx_t{street}}};

      rtree_.insert(min_corner, max_corner, entry);
    }
  }

  // Add place coordinates.
  for (auto const [place, c] : utl::enumerate(t.place_coordinates_)) {
    auto const min_corner = c.as_latlng().lnglat_float();
    rtree_.insert(min_corner, min_corner,
                  rtree_entity{.place_ = {.type_ = entity_type::kPlace,
                                          .place_ = place_idx_t{place}}});
  }

  // Add house numbers.
  for (auto const [street, house_numbers] :
       utl::enumerate(t.house_coordinates_)) {
    auto const street_idx = street_idx_t{street};

    for (auto const [hn_idx, c] : utl::enumerate(house_numbers)) {
      auto const min_corner = c.as_latlng().lnglat_float();
      rtree_.insert(
          min_corner, min_corner,
          rtree_entity{.hn_ = {.type_ = entity_type::kHouseNumber,
                               .idx_ = static_cast<std::uint16_t>(hn_idx),
                               .street_ = street_idx}});
    }
  }
}

void reverse::write() { rtree_.write_meta(p_ / "rtree_meta.bin"); }

}  // namespace adr