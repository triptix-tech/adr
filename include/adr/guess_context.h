#pragma once

#include <array>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

#include "adr/types.h"

namespace adr {

struct typeahead;

struct match {
  bool operator<(match const& o) const { return cos_sim_ > o.cos_sim_; }
  string_idx_t str_idx_;
  float cos_sim_;
};

struct address {
  static constexpr auto const kNoHouseNumber =
      std::numeric_limits<std::uint16_t>::max();

  street_idx_t street_;
  std::uint16_t house_number_;
};

struct suggestion {
  void print(std::ostream&, typeahead const&) const;

  std::variant<place_idx_t, address, area_idx_t> location_;
  coordinates coordinates_;
  std::array<area_idx_t, 12> areas_;
};

struct guess_context {
  std::string normalized_;
  std::vector<match> matches_;
  std::vector<std::uint8_t> match_counts_;
  std::vector<suggestion> suggestions_;
  float sqrt_len_vec_in_;
};

}  // namespace adr