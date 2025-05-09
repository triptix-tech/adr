#pragma once

#include <cinttypes>
#include <iosfwd>
#include <limits>
#include <ostream>
#include <string>
#include <variant>

#include "adr/types.h"

namespace adr {

struct typeahead;

struct area_set {
  friend std::ostream& operator<<(std::ostream& out, area_set const& s);

  std::int16_t get_area_lang_idx(area_idx_t const a) const;
  basic_string<area_idx_t> get_areas() const;

  typeahead const& t_;
  language_list_t const& languages_;
  std::optional<unsigned> city_area_idx_;
  std::variant<area_set_idx_t, basic_string<area_idx_t>> areas_;
  std::uint32_t matched_mask_{std::numeric_limits<std::uint32_t>::max()};
  area_set_lang_t matched_area_lang_;
};

}  // namespace adr