#pragma once

#include <iosfwd>
#include <string>
#include <variant>
#include <vector>

#include "cista/containers/vector.h"

#include "ankerl/cista_adapter.h"

#include "adr/area_set.h"
#include "adr/cache.h"
#include "adr/ngram.h"
#include "adr/normalize.h"
#include "adr/sift4.h"
#include "adr/types.h"

namespace adr {

struct typeahead;
struct guess_context;
struct formatter;

constexpr auto const kNoMatchScores = []() {
  auto a = phrase_match_scores_t{};
  for (auto& x : a) {
    x = kNoMatch;
  }
  return a;
}();

struct address {
  static constexpr auto const kNoHouseNumber =
      std::numeric_limits<std::uint16_t>::max();

  CISTA_FRIEND_COMPARABLE(address)

  street_idx_t street_;
  std::uint32_t house_number_;
};

struct matched_area {
  area_idx_t area_;
  language_idx_t lang_;
};

struct suggestion {
  std::optional<std::string_view> get_country_code(typeahead const&) const;
  std::string format(typeahead const&,
                     formatter const&,
                     std::string_view country_code) const;
  void print(std::ostream&, typeahead const&, language_list_t const&) const;
  bool operator<(suggestion const& o) const {
    return std::tie(score_, location_, area_set_) <
           std::tie(o.score_, o.location_, o.area_set_);
  }

  area_set areas(typeahead const&, language_list_t const&) const;
  std::string description(typeahead const&) const;
  void populate_areas(typeahead const&);
  std::uint64_t get_osm_id(typeahead const&) const;

  string_idx_t str_;
  std::variant<place_idx_t, address> location_;
  coordinates coordinates_;
  area_set_idx_t area_set_;
  area_set_lang_t matched_area_lang_;
  std::uint32_t matched_areas_;
  std::uint8_t matched_tokens_;
  float score_;

  std::optional<unsigned> city_area_idx_{};
  std::optional<unsigned> zip_area_idx_{};
  std::optional<unsigned> unique_area_idx_{};
  timezone_idx_t tz_{timezone_idx_t::invalid()};
};

struct cos_sim_match {
  bool operator==(cos_sim_match const&) const { return false; }
  bool operator<(cos_sim_match const& o) const { return cos_sim_ > o.cos_sim_; }
  string_idx_t idx_;
  score_t cos_sim_;
};

template <typename T>
struct scored_match {
  bool operator==(scored_match const&) const noexcept { return false; }
  bool operator<(scored_match const& o) const noexcept {
    return score_ < o.score_;
  }
  score_t score_;
  phrase_idx_t phrase_idx_;
  string_idx_t string_idx_;
  T idx_;
};

struct match_item {
  enum class type { kStreet, kHouseNumber, kPlace } type_;
  score_t score_;
  std::uint32_t index_;
  phrase_idx_t house_number_p_idx_;
  std::uint8_t matched_mask_;
};

struct guess_context {
  explicit guess_context(cache& cache) : cache_{cache} {}

  void resize(typeahead const&);

  utf8_normalize_buf_t normalize_buf_;
  std::vector<sift_offset> sift4_offset_arr_;

  std::vector<phrase> phrases_;
  std::vector<suggestion> suggestions_;

  //  cista::raw::vector_map<string_idx_t, std::uint8_t> string_match_counts_;

  adr::cache& cache_;

  std::vector<cos_sim_match> string_matches_;

  std::vector<bool> area_active_;

  std::vector<phrase_match_scores_t> string_phrase_match_scores_;
  cista::raw::vector_map<area_idx_t, phrase_match_scores_t>
      area_phrase_match_scores_;
  cista::raw::vector_map<area_idx_t, phrase_lang_t> area_phrase_lang_;

  cista::raw::ankerl_map<area_set_idx_t, std::vector<match_item>>
      area_match_items_;
  cista::raw::ankerl_set<std::uint8_t> item_matched_masks_;

  std::vector<scored_match<street_idx_t>> scored_street_matches_;
  std::vector<scored_match<place_idx_t>> scored_place_matches_;

  float sqrt_len_vec_in_;
};

}  // namespace adr