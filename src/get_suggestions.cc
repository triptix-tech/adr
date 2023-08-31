#include "adr/adr.h"

#include "cista/containers/flat_matrix.h"

#include "adr/typeahead.h"

namespace adr {

struct area {
  friend bool operator==(area const& a, area const& b) {
    return a.area_ == b.area_;
  }
  area_idx_t area_;
  coordinates coordinates_;
  float cos_sim_{0.0};
};

std::uint8_t soundex(char const c) {
  switch (c) {
    case 'b':
    case 'f':
    case 'p':
    case 'v': return 1;
    case 'c':
    case 'g':
    case 'j':
    case 'k':
    case 'q':
    case 's':
    case 'x':
    case 'z': return 2;
    case 'd':
    case 't': return 3;
    case 'l': return 4;
    case 'm':
    case 'n': return 5;
    case 'r': return 6;
    default: return c;
  }
}

std::uint8_t soundex_diff(char const a, char const b) {
  return soundex(a) == soundex(b) ? 1U : 1U;
}

unsigned levenshtein_distance(std::string_view source,
                              std::string_view target,
                              std::vector<unsigned>& lev_dist) {
  using size_type = unsigned;

  if (source.size() > target.size()) {
    return levenshtein_distance(target, source, lev_dist);
  }

  auto const min_size = source.size();
  auto const max_size = target.size();
  lev_dist.resize(min_size + 1);

  for (auto i = 0; i <= min_size; ++i) {
    lev_dist[i] = i;
  }

  for (auto j = 1; j <= max_size; ++j) {
    auto previous_diagonal = lev_dist[0];
    auto previous_diagonal_save = 0U;
    ++lev_dist[0];

    for (auto i = 1; i <= min_size; ++i) {
      previous_diagonal_save = lev_dist[i];
      if (source[i - 1] == target[j - 1]) {
        lev_dist[i] = previous_diagonal;
      } else {
        lev_dist[i] = std::min(std::min(lev_dist[i - 1], lev_dist[i]),
                               previous_diagonal) +
                      soundex_diff(source[i - 1], target[j - 1]);
      }
      previous_diagonal = previous_diagonal_save;
    }
  }

  return lev_dist[min_size];
}

unsigned levenshtein_distance_normalize(std::string_view s,
                                        std::string_view p,
                                        std::vector<unsigned>& lev_dist) {
  auto const normalized_str = normalize_alloc(s);
  auto const cut_normalized_str =
      normalized_str.substr(0U, std::min(normalized_str.size(), p.size()));
  auto const dist = levenshtein_distance(cut_normalized_str, p, lev_dist);
  return dist + ((normalized_str.size() - cut_normalized_str.size()) / 4) +
         static_cast<unsigned>(
             std::ceil(2.0F * (static_cast<float>(dist) /
                               static_cast<float>(cut_normalized_str.size()))));
}

void get_suggestions(typeahead const& t,
                     geo::latlng const& /* coord */,
                     std::string_view in,
                     unsigned n_suggestions,
                     guess_context& ctx) {
  auto tokens = std::vector<std::string>{};
  auto all_tokens_mask = std::uint8_t{0U};
  auto i = 0U;
  utl::for_each_token(utl::cstr{in}, ' ', [&](utl::cstr token) {
    if (token.empty()) {
      return;
    }
    tokens.emplace_back(normalize_alloc(token.view()));
    all_tokens_mask |= 1U << (i++);
  });
  ctx.phrases_ = get_phrases(tokens);

  t.guess(in, ctx);

  auto input_hashes = cista::raw::ankerl_set<cista::hash_t>{};
  auto input_vec = std::vector<std::string>{};
  utl::for_each_token(utl::cstr{ctx.normalized_}, ' ', [&](utl::cstr s) {
    input_hashes.emplace(cista::hash(s.view()));
    auto const start = &s.view()[0];
    auto const size = ctx.normalized_.size() - (start - &ctx.normalized_[0]);
    input_vec.emplace_back(start, size);
  });

  auto const get_edit_dist =
      [&]<typename T>(cista::raw::vector_map<T, std::uint8_t>& match_counts,
                      std::vector<match<T>>& matches,
                      data::vector_map<T, string_idx_t> const& names) {
        for (auto& m : matches) {
          for (auto const& [j, p] : utl::enumerate(ctx.phrases_)) {
            m.edit_dist_[j] = levenshtein_distance_normalize(
                t.strings_[names[m.idx_]].view(), p.s_, ctx.lev_dist_);
          }
        }
      };
  get_edit_dist(ctx.street_match_counts_, ctx.street_matches_, t.street_names_);
  get_edit_dist(ctx.place_match_counts_, ctx.place_matches_, t.place_names_);
  get_edit_dist(ctx.area_match_counts_, ctx.area_matches_, t.area_names_);

  auto area_edit_dist =
      cista::raw::vector_map<area_idx_t,
                             std::array<std::uint8_t, kMaxInputPhrases>>{};
  auto area_active = std::vector<bool>{};
  area_active.resize(t.area_names_.size());
  area_edit_dist.resize(t.area_names_.size(), default_edit_dist());
  for (auto const m : ctx.area_matches_) {
    area_edit_dist[m.idx_] = m.edit_dist_;
    area_active[to_idx(m.idx_)] = true;
  }

  for (auto const& street_match : ctx.street_matches_) {
    auto const street = street_match.idx_;

    auto zip_mask = std::uint8_t{0U};
    std::set<area_idx_t> areas;
    std::set<area_idx_t> zip_areas;
    for (auto const [area_set, pos] :
         utl::zip(t.street_areas_[street], t.street_pos_[street])) {
      for (auto const a : t.area_sets_[area_set]) {
        if (t.area_admin_level_[a] == kPostalCodeAdminLevel) {
          for (auto const& [j, p] : utl::enumerate(ctx.phrases_)) {
            area_edit_dist[a][j] = levenshtein_distance_normalize(
                t.strings_[t.area_names_[a]].view(), p.s_, ctx.lev_dist_);
          }
          areas.emplace(a);
          area_active[to_idx(a)] = true;
        }
        if (!area_active[to_idx(a)]) {
          continue;
        }
        auto const admin_lvl = t.area_admin_level_[a];
        if (admin_lvl >= 6 && admin_lvl <= 8 ||
            admin_lvl == kPostalCodeAdminLevel) {
          areas.emplace(a);
        }
      }
    }

    auto house_number_candidates = std::vector<std::uint16_t>{};
    auto house_number_mask = std::uint8_t{0U};
    for (auto const [house_number_idx, str_and_pos] : utl::enumerate(utl::zip(
             t.house_numbers_[street], t.house_coordinates_[street]))) {
      auto const& [house_number, pos] = str_and_pos;
      auto const normalized_hn =
          normalize_alloc(t.strings_[house_number].view());
      if (input_hashes.contains(cista::hash(normalized_hn))) {
        auto const token_it =
            std::find(begin(tokens), end(tokens), normalized_hn);
        auto const token_idx = std::distance(begin(tokens), token_it);
        house_number_mask |= 1U << token_idx;
        house_number_candidates.emplace_back(house_number_idx);
      }
    }

    for (auto const a : areas) {
      if (!area_active[to_idx(a)]) {
        continue;
      }

      auto min_score = std::numeric_limits<std::uint8_t>::max();
      auto min_area_phrase_idx = std::numeric_limits<std::uint8_t>::max();
      auto min_street_phrase_idx = std::numeric_limits<std::uint8_t>::max();

      for (auto street_phrase_idx = 0U;
           street_phrase_idx != ctx.phrases_.size(); ++street_phrase_idx) {
        for (auto area_phrase_idx = 0U; area_phrase_idx != ctx.phrases_.size();
             ++area_phrase_idx) {
          if ((ctx.phrases_[street_phrase_idx].input_token_bits_ &
               ctx.phrases_[area_phrase_idx].input_token_bits_) != 0U) {
            continue;
          }

          if ((ctx.phrases_[street_phrase_idx].input_token_bits_ |
               ctx.phrases_[area_phrase_idx].input_token_bits_) !=
              (all_tokens_mask & ~house_number_mask)) {
            continue;
          }

          auto const worst_match_penalty =
              std::max(street_match.edit_dist_[street_phrase_idx],
                       area_edit_dist[a][area_phrase_idx]);
          auto const total_edit_dist =
              street_match.edit_dist_[street_phrase_idx] +
              area_edit_dist[a][area_phrase_idx];
          auto const score = total_edit_dist + worst_match_penalty;

          if (score < min_score) {
            min_score = score;
            min_street_phrase_idx = street_phrase_idx;
            min_area_phrase_idx = area_phrase_idx;
          }
        }
      }

      if (min_street_phrase_idx != std::numeric_limits<std::uint8_t>::max() &&
          min_area_phrase_idx != std::numeric_limits<std::uint8_t>::max() &&
          min_score != std::numeric_limits<std::uint8_t>::max()) {
        if (house_number_candidates.empty()) {
          ctx.suggestions_.emplace_back(suggestion{
              .location_ = address{.street_ = street_match.idx_,
                                   .house_number_ = address::kNoHouseNumber},
              .area_ = a,
              .score_ = min_score,
              .street_phase_idx_ = min_street_phrase_idx,
              .area_phrase_idx_ = min_area_phrase_idx,
              .street_edit_dist_ =
                  street_match.edit_dist_[min_street_phrase_idx],
              .area_edit_dist_ = area_edit_dist[a][min_area_phrase_idx]});
        } else {
          //          fmt::print("STREET {}\n",
          //          t.strings_[t.street_names_[street]].view());
          for (auto const house_number_idx : house_number_candidates) {
            auto const areas = t.house_areas_[street][house_number_idx];
            auto county = std::string_view{};
            auto area_found = false;
            for (auto const house_number_area : t.area_sets_[areas]) {
              if (house_number_area == a) {
                area_found = true;
              }
              if (t.area_admin_level_[a] == 6) {
                county = t.strings_[t.area_names_[a]].view();
              }
            }
            if (!area_found) {
              continue;
            }
            //            fmt::print("  HOUSE NUMBER {}: {} @ {} [area_set={},
            //            county={}]\n",
            //                       house_number_idx,
            //                       t.strings_[t.house_numbers_[street][house_number_idx]],
            //                       t.house_coordinates_[street][house_number_idx],
            //                       areas, county);
            ctx.suggestions_.emplace_back(suggestion{
                .location_ = address{.street_ = street_match.idx_,
                                     .house_number_ = house_number_idx},
                .area_ = a,
                .score_ = min_score,
                .street_phase_idx_ = min_street_phrase_idx,
                .area_phrase_idx_ = min_area_phrase_idx,
                .street_edit_dist_ =
                    street_match.edit_dist_[min_street_phrase_idx],
                .area_edit_dist_ = area_edit_dist[a][min_area_phrase_idx]});
          }
        }
      }
    }
  }

  if (ctx.suggestions_.empty()) {
    return;
  }

  auto const result_count = static_cast<std::ptrdiff_t>(
      std::min(std::size_t{n_suggestions}, ctx.suggestions_.size()));
  std::nth_element(begin(ctx.suggestions_),
                   begin(ctx.suggestions_) + result_count,
                   end(ctx.suggestions_));
  ctx.suggestions_.resize(result_count);
  std::sort(begin(ctx.suggestions_), end(ctx.suggestions_));
}

}  // namespace adr