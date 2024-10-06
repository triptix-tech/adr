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

enum class entity_type : std::uint8_t { kHouseNumber, kStreet, kPlace };

union rtree_entity {
  void* to_data() const {
    void* data;
    std::memcpy(&data, this, sizeof(data));
    return data;
  }

  static rtree_entity from_data(void const* ptr) {
    auto e = rtree_entity{};
    std::memcpy(&e, &ptr, sizeof(ptr));
    return e;
  }

  entity_type type_;

  struct house_number {
    entity_type type_;
    std::uint16_t idx_;
    street_idx_t street_;
  } hn_;

  struct place {
    entity_type type_;
    place_idx_t place_;
  } place_;

  struct street {
    entity_type type_;
    std::uint16_t segment_;
    street_idx_t street_;
  } street_segment_;
};

static_assert(sizeof(rtree_entity) == sizeof(void*));

reverse::reverse(fs::path p, cista::mmap::protection const mode)
    : p_{p},
      mode_{mode},
      street_segments_{{mm_vec<std::uint64_t>{mm("street_segments_idx_0.bin")},
                        mm_vec<std::uint64_t>{mm("street_segments_idx_1.bin")}},
                       mm_vec<coordinates>{mm("street_segments_data.bin")}} {}

reverse::~reverse() {
  if (rtree_ != nullptr) {
    rtree_free(rtree_);
  }
}

cista::mmap reverse::mm(char const* file) {
  return cista::mmap{(p_ / file).generic_string().c_str(), mode_};
}

std::vector<suggestion> reverse::lookup(typeahead const& t,
                                        geo::latlng const& query,
                                        std::size_t const n_guesses) const {
  auto const b = geo::box{query, 500.0};
  auto const min = b.min_.lnglat();
  auto const max = b.max_.lnglat();

  using udata_t = std::tuple<adr::typeahead const*, adr::reverse const*,
                             std::vector<adr::suggestion>*, geo::latlng>;
  auto suggestions = std::vector<suggestion>{};
  auto udata = udata_t{&t, this, &suggestions, query};
  rtree_search(
      rtree_, min.data(), max.data(),
      [](double const* min, double const* max, void const* item, void* udata) {
        auto const [t, r, results, query] = *reinterpret_cast<udata_t*>(udata);
        auto const e = adr::rtree_entity::from_data(item);

        auto const min_latlng = geo::latlng{min[1], min[0]};

        switch (e.type_) {
          case adr::entity_type::kHouseNumber: {
            auto const& hn = e.hn_;
            auto const c = t->house_coordinates_[hn.street_][hn.idx_];
            results->emplace_back(adr::suggestion{
                .str_ = t->street_names_[hn.street_][adr::kDefaultLangIdx],
                .location_ = adr::address{.street_ = hn.street_,
                                          .house_number_ = hn.idx_},
                .coordinates_ = c,
                .area_set_ = t->house_areas_[hn.street_][hn.idx_],
                .matched_area_lang_ = {adr::kDefaultLangIdx} /* TODO */,
                .matched_areas_ =
                    std::numeric_limits<decltype(std::declval<adr::suggestion>()
                                                     .matched_areas_)>::max(),
                .matched_tokens_ = 0U,
                .score_ = static_cast<float>(geo::distance(query, c)) - 10.F});
          } break;

          case adr::entity_type::kPlace: {
            auto const& p = e.place_;
            auto const c = t->place_coordinates_[p.place_];
            results->emplace_back(adr::suggestion{
                .str_ = t->place_names_[p.place_][adr::kDefaultLangIdx],
                .location_ = p.place_,
                .coordinates_ = c,
                .area_set_ = t->place_areas_[p.place_],
                .matched_area_lang_ = {adr::kDefaultLangIdx} /* TODO */,
                .matched_areas_ =
                    std::numeric_limits<decltype(std::declval<adr::suggestion>()
                                                     .matched_areas_)>::max(),
                .matched_tokens_ = 0U,
                .score_ = static_cast<float>(geo::distance(query, c)) - 10.F});
          } break;

          case adr::entity_type::kStreet:
            auto const& s = e.street_segment_;
            auto const [dist, closest, _] = geo::distance_to_polyline(
                query, r->street_segments_[s.street_][s.segment_]);
            results->emplace_back(adr::suggestion{
                .str_ = t->street_names_[s.street_][adr::kDefaultLangIdx],
                .location_ =
                    adr::address{.street_ = s.street_,
                                 .house_number_ = adr::address::kNoHouseNumber},
                .coordinates_ = adr::coordinates::from_latlng(closest),
                .area_set_ = t->street_areas_[s.street_][0U] /* TODO */,
                .matched_area_lang_ = {adr::kDefaultLangIdx} /* TODO */,
                .matched_areas_ =
                    std::numeric_limits<decltype(std::declval<adr::suggestion>()
                                                     .matched_areas_)>::max(),
                .matched_tokens_ = 0U,
                .score_ = static_cast<float>(dist)});
            break;
        }

        return true;
      },
      &udata);

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

  rtree_ = rtree_new();

  // Add street segment bounding boxes.
  for (auto const [street, segments] : utl::enumerate(street_segments_)) {
    for (auto const [segment_idx, segment] : utl::enumerate(segments)) {
      auto b = geo::box{};
      for (auto const& c : segment) {
        b.extend(c);
      }

      auto const min_corner = b.min_.lnglat();
      auto const max_corner = b.max_.lnglat();

      rtree_insert(
          rtree_, min_corner.data(), max_corner.data(),
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
    auto const min_corner = c.as_latlng().lnglat();
    rtree_insert(rtree_, min_corner.data(), nullptr,
                 rtree_entity{.place_ = {.type_ = entity_type::kPlace,
                                         .place_ = place_idx_t{place}}}
                     .to_data());
  }

  // Add house numbers.
  for (auto const [street, house_numbers] :
       utl::enumerate(t.house_coordinates_)) {
    auto const street_idx = street_idx_t{street};

    for (auto const [hn_idx, c] : utl::enumerate(house_numbers)) {
      auto const min_corner = c.as_latlng().lnglat();
      rtree_insert(
          rtree_, min_corner.data(), nullptr,
          rtree_entity{.hn_ = {.type_ = entity_type::kHouseNumber,
                               .idx_ = static_cast<std::uint16_t>(hn_idx),
                               .street_ = street_idx}}
              .to_data());
    }
  }
}

}  // namespace adr