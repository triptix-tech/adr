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

constexpr auto const kNoMatchScores = []() {
  auto a = phrase_match_scores_t{};
  for (auto& x : a) {
    x = kNoMatch;
  }
  return a;
}();

template <typename T>
struct cos_sim_match {
  bool operator==(cos_sim_match const& o) const { return false; }
  bool operator<(cos_sim_match const& o) const { return cos_sim_ > o.cos_sim_; }
  T idx_;
  score_t cos_sim_;
  phrase_match_scores_t phrase_match_scores_{kNoMatchScores};
};

struct address {
  static constexpr auto const kNoHouseNumber =
      std::numeric_limits<std::uint16_t>::max();

  street_idx_t street_;
  std::uint32_t house_number_;
};

struct suggestion {
  void print(std::ostream&,
             typeahead const&,
             std::vector<phrase> const& phrases) const;
  bool operator<(suggestion const& o) const { return score_ < o.score_; }

  std::variant<place_idx_t, address, area_idx_t> location_;
  coordinates coordinates_;
  area_set_idx_t area_set_;
  std::uint32_t matched_areas_;
  float score_;
};

struct area_src {
  enum class type { kStreet, kHouseNumber, kPlace } type_;
  score_t score_;
  std::uint32_t index_;
  phrase_idx_t house_number_p_idx_;
  std::uint8_t matched_mask_;
};

template <typename T>
struct scored_match {
  bool operator==(scored_match const& o) const noexcept { return false; }
  bool operator<(scored_match const& o) const noexcept {
    return score_ < o.score_;
  }
  score_t score_;
  phrase_idx_t phrase_idx_;
  T idx_;
};

struct guess_context {
  void reset() {
    suggestions_.clear();
    place_matches_.clear();
    area_matches_.clear();
    street_matches_.clear();
  }

  std::string tmp_;
  std::vector<std::uint8_t> lev_dist_;
  std::vector<cos_sim_match<place_idx_t>> place_matches_;
  cista::raw::vector_map<place_idx_t, std::uint8_t> place_match_counts_;
  std::vector<cos_sim_match<area_idx_t>> area_matches_;
  cista::raw::vector_map<area_idx_t, std::uint8_t> area_match_counts_;
  std::vector<cos_sim_match<street_idx_t>> street_matches_;
  cista::raw::vector_map<street_idx_t, std::uint8_t> street_match_counts_;
  std::vector<phrase> phrases_;
  std::vector<suggestion> suggestions_;
  cista::raw::ankerl_map<area_set_idx_t, std::vector<area_src>> areas_;
  cista::raw::ankerl_set<std::uint8_t> item_matched_masks_;
  std::vector<scored_match<street_idx_t>> scored_street_matches_;
  std::vector<scored_match<place_idx_t>> scored_place_matches_;
  cista::raw::vector_map<area_idx_t, phrase_match_scores_t>
      area_phrase_match_scores_;
  std::vector<bool> area_active_;
  float sqrt_len_vec_in_;
};

}  // namespace adr