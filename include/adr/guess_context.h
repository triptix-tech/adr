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
  float dist_;
  std::uint32_t index_;
  std::uint8_t house_number_p_idx_;
  std::uint8_t matched_mask_;
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
  cista::raw::ankerl_set<std::uint8_t> item_tabu_masks_;
  float sqrt_len_vec_in_;
};

}  // namespace adr