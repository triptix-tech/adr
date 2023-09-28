#pragma once

#include "osmium/osm/location.hpp"

#include "cista/containers/vector.h"
#include "cista/strong.h"

namespace cista::raw {}

namespace adr {

namespace data = cista::raw;

using area_set_idx_t = cista::strong<std::uint32_t, struct area_set_idx_>;
using area_idx_t = cista::strong<std::uint32_t, struct area_idx_>;
using admin_level_t = cista::strong<std::uint8_t, struct admin_level_idx_>;
using string_idx_t = cista::strong<std::uint32_t, struct string_idx_>;
using street_idx_t = cista::strong<std::uint32_t, struct street_idx_>;
using place_idx_t = cista::strong<std::uint32_t, struct place_idx_>;
using language_idx_t = cista::strong<std::uint8_t, struct language_idx_>;

constexpr auto const kDefaultLangIdx = 0U;
constexpr auto const kDefaultLang = language_idx_t{0U};

constexpr auto const kPostalCodeAdminLevel = admin_level_t{11};

using language_list_t = std::basic_string_view<language_idx_t>;

using area_set_lang_t = std::array<std::uint8_t, 32U>;

enum class location_type_t : std::uint8_t {
  kPlace,
  kStreet,
};

struct coordinates {
  friend std::ostream& operator<<(std::ostream& out, coordinates const& c) {
    auto const l = osmium::Location{c.lat_, c.lng_};
    return out << '(' << l.lat() << ", " << l.lon() << ')';
  }
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
