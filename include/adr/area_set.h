#pragma once

#include <iosfwd>

#include "adr/types.h"

namespace adr {

struct typeahead;

struct area_set {
  friend std::ostream& operator<<(std::ostream& out, area_set const& s);

  std::int16_t get_area_lang_idx(area_idx_t const a) const;

  typeahead const& t_;
  language_list_t const& languages_;
  area_set_idx_t areas_;
  std::uint32_t matched_mask_{std::numeric_limits<std::uint32_t>::max()};
  area_set_lang_t matched_area_lang_;
};

}  // namespace adr