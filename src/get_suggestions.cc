#include "adr/adr.h"

#include "fmt/ranges.h"

#include "utl/timing.h"
#include "utl/to_vec.h"

#include "cista/containers/flat_matrix.h"

#include "adr/score.h"
#include "adr/trace.h"
#include "adr/typeahead.h"

namespace adr {

constexpr auto const kMaxScoredMatches = std::size_t{1000U};

struct area {
  friend bool operator==(area const& a, area const& b) {
    return a.area_ == b.area_;
  }
  area_idx_t area_;
  coordinates coordinates_;
  float cos_sim_{0.0};
};

template <typename T>
void get_match_score(guess_context& ctx,
                     typeahead const& t,
                     cista::raw::vector_map<T, std::uint8_t>& match_counts,
                     std::vector<cos_sim_match<T>>& matches,
                     data::vector_map<T, string_idx_t> const& names) {
  for (auto& m : matches) {
    for (auto const& [j, p] : utl::enumerate(ctx.phrases_)) {
      m.phrase_match_scores_[j] = get_match_score(
          t.strings_[names[m.idx_]].view(), p.s_, ctx.lev_dist_, ctx.tmp_);
    }
  }
}

void match_streets(std::uint8_t const numeric_tokens_mask,
                   typeahead const& t,
                   guess_context& ctx,
                   std::vector<std::string> const& tokens) {
  for (auto const [street_edit_dist, street_p_idx, street] :
       ctx.scored_street_matches_) {
    ctx.areas_.clear();

    for (auto const [i, area_set] : utl::enumerate(t.street_areas_[street])) {
      ctx.areas_[area_set].emplace_back(
          area_src{.type_ = area_src::type::kStreet,
                   .score_ = 0.0F,
                   .index_ = static_cast<std::uint32_t>(i),
                   .matched_mask_ = ctx.phrases_[street_p_idx].token_bits_});
    }

    auto i = 0U;
    for (auto const [hn, areas_idx] :
         utl::zip(t.house_numbers_[street], t.house_areas_[street])) {
      for (auto const& [hn_p_idx, p] : utl::enumerate(ctx.phrases_)) {
        if ((p.token_bits_ & numeric_tokens_mask) != p.token_bits_) {
          continue;
        }

        auto const hn_score = get_match_score(t.strings_[hn].view(), p.s_,
                                              ctx.lev_dist_, ctx.tmp_);
        if (hn_score == kNoMatch) {
          continue;
        }

        ctx.areas_[areas_idx].emplace_back(
            area_src{.type_ = area_src::type::kHouseNumber,
                     .score_ = hn_score,
                     .index_ = i,
                     .house_number_p_idx_ = static_cast<std::uint8_t>(hn_p_idx),
                     .matched_mask_ = static_cast<std::uint8_t>(
                         ctx.phrases_[street_p_idx].token_bits_ |
                         ctx.phrases_[hn_p_idx].token_bits_)});
      }
      ++i;
    }

    for (auto const& [area_set_idx, items] : ctx.areas_) {
      // Activate postal code areas.
      for (auto const area : t.area_sets_[area_set_idx]) {
        if (!ctx.area_active_[to_idx(area)] &&
            t.area_admin_level_[area] == kPostalCodeAdminLevel) {
          auto const area_name = t.strings_[t.area_names_[area]].view();
          ctx.area_active_[to_idx(area)] = true;
          for (auto const& [j, p] : utl::enumerate(ctx.phrases_)) {
            ctx.area_phrase_match_scores_[area][j] =
                ((p.token_bits_ & numeric_tokens_mask) == p.token_bits_)
                    ? get_match_score(area_name, p.s_, ctx.lev_dist_, ctx.tmp_)
                    : std::numeric_limits<float>::max();
          }
        }
      }

      ctx.item_matched_masks_.clear();
      for (auto const& item : items) {
        ctx.item_matched_masks_.emplace(item.matched_mask_);
      }

      // For each phrase: greedily match an area name
      // IF matching distance is below the no-match-penalty threshold.
      for (auto const item_matched_mask : ctx.item_matched_masks_) {
        auto matched_mask = item_matched_mask;
        auto matched_areas = std::uint32_t{0U};
        auto areas_edit_dist = 0.0F;
        for (auto const& [area_p_idx, area_p] : utl::enumerate(ctx.phrases_)) {
          auto best_edit_dist = std::numeric_limits<float>::max();
          auto best_area_idx = 0U;

          if ((area_p.token_bits_ & matched_mask) != 0U) {
            continue;
          }

          for (auto const [area_idx, area] :
               utl::enumerate(t.area_sets_[area_set_idx])) {
            auto const area_name = t.strings_[t.area_names_[area]].view();

            auto const match_allowed =
                (  // Zip-code areas only match numeric tokens.
                    (((area_p.token_bits_ & numeric_tokens_mask) ==
                      area_p.token_bits_) &&
                     t.area_admin_level_[area] == kPostalCodeAdminLevel) ||
                    // No-zip-code areas only match non-numeric tokens.
                    (((area_p.token_bits_ & numeric_tokens_mask) == 0U) &&
                     t.area_admin_level_[area] != kPostalCodeAdminLevel)  //
                    ) &&
                // Area matched by bi-grams.
                ctx.area_active_[to_idx(area)];

            if (!match_allowed) {
              continue;
            }

            auto const edit_dist =
                ctx.area_phrase_match_scores_[area][area_p_idx];
            if (best_edit_dist > edit_dist) {
              best_edit_dist = edit_dist;
              best_area_idx = area_idx;
            }
          }

          if (best_edit_dist < std::numeric_limits<float>::max()) {
            matched_areas |= (1U << best_area_idx);
            areas_edit_dist += best_edit_dist;
            matched_mask |= area_p.token_bits_;
          }
        }

        for (auto const item : items) {
          if (item.matched_mask_ == item_matched_mask) {
            auto total_score = street_edit_dist + areas_edit_dist + item.score_;
            for (auto const [t_idx, token] : utl::enumerate(tokens)) {
              if ((matched_mask & (1U << t_idx)) == 0U) {
                total_score += token.size();
              }
            }
            ctx.suggestions_.emplace_back(suggestion{
                .location_ =
                    address{
                        .street_ = street,
                        .house_number_ =
                            item.type_ == area_src::type::kHouseNumber
                                ? item.index_
                                : address::kNoHouseNumber,
                    },
                .coordinates_ = item.type_ == area_src::type::kHouseNumber
                                    ? t.house_coordinates_[street][item.index_]
                                    : t.street_pos_[street][item.index_],
                .area_set_ = area_set_idx,
                .matched_areas_ = matched_areas,
                .score_ = total_score});
          }
        }
      }
    }
  }
}

template <typename T>
void get_scored_matches(typeahead const& t,
                        guess_context& ctx,
                        std::uint8_t const numeric_tokens_mask,
                        data::vector_map<T, string_idx_t> const& names,
                        std::vector<cos_sim_match<T>> const& filtered,
                        std::vector<scored_match<T>>& scored_matches) {
  scored_matches.clear();
  for (auto const& m : filtered) {
    auto const street = m.idx_;
    auto const street_name = t.strings_[names[street]].view();
    auto const& normalized_street_name = normalize(street_name, ctx.tmp_);
    for (auto p_idx = phrase_idx_t{0U}; p_idx != ctx.phrases_.size(); ++p_idx) {
      if ((ctx.phrases_[p_idx].token_bits_ & numeric_tokens_mask) != 0U) {
        continue;
      }

      auto const p_match_score = m.phrase_match_scores_[p_idx];
      if (p_match_score != kNoMatch &&
          (scored_matches.size() != kMaxScoredMatches ||
           scored_matches.back().score_ >= p_match_score)) {
        utl::insert_sorted(
            scored_matches,
            {.score_ = p_match_score, .phrase_idx_ = p_idx, .idx_ = street});
        scored_matches.resize(
            std::min(kMaxScoredMatches, scored_matches.size()));
      }
    }
  }
}

template <bool Debug>
void get_suggestions(typeahead const& t,
                     geo::latlng const& /* coord */,
                     std::string_view in,
                     unsigned n_suggestions,
                     guess_context& ctx) {
  UTL_START_TIMING(t);

  auto tokens = std::vector<std::string>{};
  auto all_tokens_mask = std::uint8_t{0U};
  utl::for_each_token(utl::cstr{in}, ' ', [&, i = 0U](utl::cstr token) mutable {
    if (token.empty()) {
      return;
    }
    tokens.emplace_back(normalize_alloc(token.view()));
    all_tokens_mask |= 1U << (i++);
  });
  ctx.phrases_ = get_phrases(tokens);

  t.guess<Debug>(normalize(in, ctx.tmp_), ctx);

  get_match_score(ctx, t, ctx.street_match_counts_, ctx.street_matches_,
                  t.street_names_);
  get_match_score(ctx, t, ctx.place_match_counts_, ctx.place_matches_,
                  t.place_names_);
  get_match_score(ctx, t, ctx.area_match_counts_, ctx.area_matches_,
                  t.area_names_);

  ctx.area_active_.resize(t.area_names_.size());
  ctx.area_phrase_match_scores_.resize(t.area_names_.size(), kNoMatchScores);
  for (auto const m : ctx.area_matches_) {
    ctx.area_phrase_match_scores_[m.idx_] = m.phrase_match_scores_;
    ctx.area_active_[to_idx(m.idx_)] = true;
  }

  auto const numeric_tokens_mask = get_numeric_tokens_mask(tokens);

  get_scored_matches(t, ctx, numeric_tokens_mask, t.street_names_,
                     ctx.street_matches_, ctx.scored_street_matches_);
  get_scored_matches(t, ctx, numeric_tokens_mask, t.place_names_,
                     ctx.place_matches_, ctx.scored_place_matches_);

  match_streets(numeric_tokens_mask, t, ctx, tokens);

  UTL_STOP_TIMING(t);
  trace("{} suggestions [{} ms]\n", ctx.suggestions_.size(), UTL_TIMING_MS(t));

  if (ctx.suggestions_.empty()) {
    return;
  }

  UTL_START_TIMING(sort);
  auto const result_count = static_cast<std::ptrdiff_t>(
      std::min(std::size_t{n_suggestions}, ctx.suggestions_.size()));
  std::nth_element(begin(ctx.suggestions_),
                   begin(ctx.suggestions_) + result_count,
                   end(ctx.suggestions_));
  ctx.suggestions_.resize(result_count);
  std::sort(begin(ctx.suggestions_), end(ctx.suggestions_));
  UTL_STOP_TIMING(sort);
  trace("sort [{} us]\n", UTL_TIMING_US(sort));
}

template void get_suggestions<true>(typeahead const&,
                                    geo::latlng const&,
                                    std::string_view,
                                    unsigned,
                                    guess_context&);
template void get_suggestions<false>(typeahead const&,
                                     geo::latlng const&,
                                     std::string_view,
                                     unsigned,
                                     guess_context&);

}  // namespace adr