#pragma once

#include <array>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

#include "ankerl/cista_adapter.h"
#include "cista/containers/vector.h"

#include "adr/normalize.h"
#include "adr/types.h"
#include "ngram.h"

namespace adr {

struct typeahead;

constexpr auto const kMaxInputTokens = 8U;
constexpr auto const kMaxInputPhrases = 32U;

using edit_dist_t = std::uint8_t;
constexpr auto const kMaxEditDist = std::numeric_limits<edit_dist_t>::max();

template <typename T>
constexpr std::array<T, kMaxInputPhrases> inf_edit_dist() {
  auto a = std::array<T, kMaxInputPhrases>{};
  for (auto& x : a) {
    x = std::numeric_limits<T>::max();
  }
  return a;
}

template <typename T>
struct match {
  bool operator==(match const& o) const { return false; }
  bool operator<(match const& o) const { return cos_sim_ > o.cos_sim_; }
  T idx_;
  float cos_sim_;
  std::array<float, kMaxInputPhrases> edit_dist_{inf_edit_dist<float>()};
};

struct address {
  static constexpr auto const kNoHouseNumber =
      std::numeric_limits<std::uint16_t>::max();

  street_idx_t street_;
  std::uint16_t house_number_;
};

struct suggestion {
  void print(std::ostream&,
             typeahead const&,
             std::vector<phrase> const& phrases) const;
  bool operator<(suggestion const& o) const { return score_ < o.score_; }

  std::variant<place_idx_t, address, area_idx_t> location_;
  coordinates coordinates_;
  area_idx_t area_;
  edit_dist_t score_;
  std::uint8_t street_phase_idx_, area_phrase_idx_;
  edit_dist_t street_edit_dist_, area_edit_dist_;
};

struct area_src {
  enum type : std::uint8_t { kStreet, kHouseNumber, kPlace };

  area_src(std::uint8_t const idx, type const src)
      : idx_{idx}, src_{static_cast<std::uint8_t>(src)} {}

  type src() const { return static_cast<type>(src_); }
  unsigned idx() const { return idx_; }

private:
  std::uint8_t idx_ : 6U;
  std::uint8_t src_ : 2U;
};

struct guess_context {
  void reset() {
    suggestions_.clear();
    place_matches_.clear();
    area_matches_.clear();
    street_matches_.clear();
  }

  std::string tmp_;
  std::vector<ngram_t> street_name_ngrams_;
  std::array<std::vector<ngram_t>, 32> phrase_ngrams_;
  std::vector<std::uint16_t> house_number_candidates_;
  std::vector<std::uint8_t> lev_dist_;
  std::vector<match<place_idx_t>> place_matches_;
  cista::raw::vector_map<place_idx_t, std::uint8_t> place_match_counts_;
  std::vector<match<area_idx_t>> area_matches_;
  cista::raw::vector_map<area_idx_t, std::uint8_t> area_match_counts_;
  std::vector<match<street_idx_t>> street_matches_;
  cista::raw::vector_map<street_idx_t, std::uint8_t> street_match_counts_;
  std::vector<phrase> phrases_;
  std::vector<suggestion> suggestions_;
  cista::raw::ankerl_map<area_set_idx_t, std::vector<area_src>> areas_;
  float sqrt_len_vec_in_;
};

}  // namespace adr