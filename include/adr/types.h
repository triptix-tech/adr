#pragma once

#include <array>
#include <string_view>

#include "osmium/osm/location.hpp"

#include "geo/latlng.h"

#include "tg.h"

#include "cista/containers/mmap_vec.h"
#include "cista/containers/nvec.h"
#include "cista/containers/vector.h"
#include "cista/mmap.h"
#include "cista/strong.h"

namespace cista::raw {}

namespace adr {

namespace data = cista::raw;

template <typename T>
using mm_vec = cista::basic_mmap_vec<T, std::uint64_t>;

template <typename K, typename V, std::size_t N>
using mm_nvec =
    cista::basic_nvec<K, mm_vec<V>, mm_vec<std::uint64_t>, N, std::uint64_t>;

using area_set_idx_t = cista::strong<std::uint32_t, struct area_set_idx_>;
using area_idx_t = cista::strong<std::uint32_t, struct area_idx_>;
using admin_level_t = cista::strong<std::uint8_t, struct admin_level_idx_>;
using string_idx_t = cista::strong<std::uint32_t, struct string_idx_>;
using street_idx_t = cista::strong<std::uint32_t, struct street_idx_>;
using place_idx_t = cista::strong<std::uint32_t, struct place_idx_>;
using language_idx_t = cista::strong<std::uint16_t, struct language_idx_>;

using country_code_t = std::array<char, 2U>;
constexpr auto const kNoCountryCode = country_code_t{'\0', '\0'};

constexpr auto const kDefaultLangIdx = 0U;
constexpr auto const kDefaultLang = language_idx_t{0U};

constexpr auto const kPostalCodeAdminLevel = admin_level_t{11};

using language_list_t = std::basic_string_view<language_idx_t>;

using area_set_lang_t = std::array<std::uint8_t, 32U>;

enum class location_type_t : std::uint8_t {
  kPlace,
  kStreet,
};

enum class place_type : std::uint8_t {
  kUnkown,  // to be extended with
            // https://wiki.openstreetmap.org/wiki/Key:amenity
  kExtra  // for entries that were added externally (not from adr extract)
};

enum class filter_type : std::uint8_t {
  kNone,
  kAddress,
  kPlace,
  kExtra,
};

struct coordinates {
  friend std::ostream& operator<<(std::ostream& out, coordinates const& c) {
    auto const l = c.as_location();
    return out << '(' << l.lat() << ", " << l.lon() << ')';
  }

  static coordinates from_location(osmium::Location const& l) {
    return {.lat_ = l.y(), .lng_ = l.x()};
  }

  static coordinates from_latlng(geo::latlng const& x) {
    return from_location(osmium::Location{x.lng(), x.lat()});
  }

  osmium::Location as_location() const { return osmium::Location{lng_, lat_}; }

  geo::latlng as_latlng() const {
    auto const l = as_location();
    return {l.lat(), l.lon()};
  }

  operator geo::latlng() const { return as_latlng(); }

  double lat() const { return as_location().lat(); }
  double lon() const { return as_location().lon(); }

  std::int32_t lat_, lng_;
};

constexpr auto const kAdminString =
    std::array<std::string_view, 12>{"0",
                                     "1",
                                     "2",
                                     "region",
                                     "state",
                                     "district",
                                     "county",
                                     "municipality",
                                     "town",
                                     "subtownship",
                                     "neighbourhood",
                                     "zip"};

inline std::string_view to_str(admin_level_t const x) {
  return x >= kAdminString.size() ? "" : kAdminString[to_idx(x)];
}

constexpr auto const kMaxInputTokens = 8U;
constexpr auto const kMaxInputPhrases = 32U;

using edit_dist_t = std::uint8_t;
constexpr auto const kMaxEditDist = std::numeric_limits<edit_dist_t>::max();

using score_t = float;
constexpr auto const kNoMatch = std::numeric_limits<score_t>::max();

using phrase_idx_t = std::uint8_t;

using phrase_match_scores_t = std::array<score_t, kMaxInputPhrases>;
using phrase_lang_t =
    std::array<std::uint8_t /* name index */, kMaxInputPhrases>;

using string_match_count_vector_t =
    cista::raw::vector_map<string_idx_t, std::uint8_t>;

}  // namespace adr
