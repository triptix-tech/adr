#include "adr/adr.h"

#include "fmt/ranges.h"

#include "utl/helpers/algorithm.h"
#include "utl/timing.h"
#include "utl/to_vec.h"

#include "cista/containers/flat_matrix.h"

#include "adr/score.h"
#include "adr/trace.h"
#include "adr/typeahead.h"

namespace adr {

constexpr auto const kMaxScoredMatches = std::size_t{250};

struct area {
  friend bool operator==(area const& a, area const& b) {
    return a.area_ == b.area_;
  }
  area_idx_t area_;
  coordinates coordinates_;
  float cos_sim_{0.0};
};

void activate_areas(typeahead const& t,
                    guess_context& ctx,
                    std::uint8_t const numeric_tokens_mask,
                    area_set_idx_t const area_set_idx) {
  for (auto const area : t.area_sets_[area_set_idx]) {
    if (ctx.area_active_[to_idx(area)]) {
      continue;
    }

    ctx.area_active_[to_idx(area)] = true;
    auto const area_name = t.strings_[t.area_names_[area]].view();
    for (auto const& [j, area_p] : utl::enumerate(ctx.phrases_)) {
      ctx.area_phrase_match_scores_[area][j] =
          (  // Zip-code areas only match numeric tokens.
              (((area_p.token_bits_ & numeric_tokens_mask) ==
                area_p.token_bits_) &&
               t.area_admin_level_[area] == kPostalCodeAdminLevel) ||
              // No-zip-code areas only match non-numeric tokens.
              (((area_p.token_bits_ & numeric_tokens_mask) == 0U) &&
               t.area_admin_level_[area] != kPostalCodeAdminLevel)  //
              )
              ? get_match_score(area_name, area_p.s_, ctx.lev_dist_, ctx.tmp_)
              : kNoMatch;
    }
  }
}

template <bool Debug>
void match_streets(std::uint8_t const numeric_tokens_mask,
                   typeahead const& t,
                   guess_context& ctx,
                   std::vector<std::string> const& tokens) {
  UTL_START_TIMING(t);

  //  std::cout << "SCORED STREET MATCHES: " <<
  //  ctx.scored_street_matches_.size()
  //            << "\n";

  auto i = 0U;
  for (auto const [street_edit_dist, street_p_idx, street] :
       ctx.scored_street_matches_) {
    ctx.areas_.clear();

    //    std::cout << "SCORED STREET MATCH [" << i++
    //              << "]: " << t.strings_[t.street_names_[street]].view() << "
    //              vs "
    //              << ctx.phrases_[street_p_idx].s_ << ": " << street_edit_dist
    //              << "}\n";

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
                     .score_ = t.strings_[hn].view() == p.s_ ? -1.5F : hn_score,
                     .index_ = i,
                     .house_number_p_idx_ = static_cast<std::uint8_t>(hn_p_idx),
                     .matched_mask_ = static_cast<std::uint8_t>(
                         ctx.phrases_[street_p_idx].token_bits_ |
                         ctx.phrases_[hn_p_idx].token_bits_)});
      }
      ++i;
    }

    for (auto const& [area_set_idx, items] : ctx.areas_) {
      activate_areas(t, ctx, numeric_tokens_mask, area_set_idx);

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
            //            std::cout <<
            //            t.strings_[t.street_names_[street]].view()
            //                      << "  NOT matched: " << area_p.s_
            //                      << ": match not allowed\n";
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
              //              std::cout <<
              //              t.strings_[t.street_names_[street]].view()
              //                        << "  NOT matched: "
              //                        <<
              //                        t.strings_[t.area_names_[t.area_sets_[area_set_idx]
              //                                                                [area_idx]]]
              //                               .view()
              //                        << " vs " << area_p.s_
              //                        << ": area_active=" <<
              //                        ctx.area_active_[to_idx(area)]
              //                        << "\n";
              continue;
            }
            //            else {
            //              std::cout <<
            //              t.strings_[t.street_names_[street]].view()
            //                        << "  NOT matched: "
            //                        <<
            //                        t.strings_[t.area_names_[t.area_sets_[area_set_idx]
            //                                                                [area_idx]]]
            //                               .view()
            //                        << " vs " << area_p.s_ << ": match not
            //                        allowed\n";
            //            }

            auto const edit_dist =
                ctx.area_phrase_match_scores_[area][area_p_idx];
            if (best_edit_dist > edit_dist) {
              best_edit_dist = edit_dist;
              best_area_idx = area_idx;
            }
            //            else {
            //              std::cout <<
            //              t.strings_[t.street_names_[street]].view()
            //                        << "  NOT matched: "
            //                        <<
            //                        t.strings_[t.area_names_[t.area_sets_[area_set_idx]
            //                                                                [area_idx]]]
            //                               .view()
            //                        << " vs " << area_p.s_ << ": " <<
            //                        edit_dist << "\n";
            //            }
          }

          if (best_edit_dist != kNoMatch) {
            matched_areas |= (1U << best_area_idx);
            areas_edit_dist += best_edit_dist;
            matched_mask |= area_p.token_bits_;
            //            std::cout <<
            //            t.strings_[t.street_names_[street]].view()
            //                      << "  matched: "
            //                      <<
            //                      t.strings_[t.area_names_[t.area_sets_[area_set_idx]
            //                                                              [best_area_idx]]]
            //                             .view()
            //                      << " vs " << area_p.s_ << ": " <<
            //                      best_edit_dist << "\n";
          }
        }

        for (auto const item : items) {
          if (item.matched_mask_ != item_matched_mask) {
            continue;
          }

          auto total_score = street_edit_dist + areas_edit_dist + item.score_;
          for (auto const [t_idx, token] : utl::enumerate(tokens)) {
            if ((matched_mask & (1U << t_idx)) == 0U) {
              total_score += token.size() * 3.0F;
            }
          }

          total_score -= std::popcount(matched_areas) * 2.0F;

          //          if (item.type_ == area_src::type::kHouseNumber) {
          //            std::cout
          //                << "HN="
          //                <<
          //                t.strings_[t.house_numbers_[street][item.index_]].view()
          //                << ", STREET=" <<
          //                t.strings_[t.street_names_[street]].view()
          //                << ", STREET_PHRASE=" <<
          //                ctx.phrases_[street_p_idx].s_
          //                << ", ITEM_SCORE=" << item.score_
          //                << ", MATCHED=" <<
          //                bit_mask_to_str(item.matched_mask_)
          //                << ", TOTAL_SCORE=" << total_score << "\n";
          //          }

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

  UTL_STOP_TIMING(t);
  trace("STREETS: {} ms\n", UTL_TIMING_MS(t));
}

template <bool Debug>
void match_places(std::uint8_t const numeric_tokens_mask,
                  typeahead const& t,
                  guess_context& ctx,
                  std::vector<std::string> const& tokens) {
  UTL_START_TIMING(t);

  for (auto const [place_edit_dist, place_p_idx, place] :
       ctx.scored_place_matches_) {
    auto const area_set_idx = t.place_areas_[place];

    activate_areas(t, ctx, numeric_tokens_mask, area_set_idx);

    auto matched_mask = ctx.phrases_[place_p_idx].token_bits_;
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

        auto const edit_dist = ctx.area_phrase_match_scores_[area][area_p_idx];
        if (best_edit_dist > edit_dist) {
          best_edit_dist = edit_dist;
          best_area_idx = area_idx;
        }
      }

      if (best_edit_dist != kNoMatch) {
        matched_areas |= (1U << best_area_idx);
        areas_edit_dist += best_edit_dist;
        matched_mask |= area_p.token_bits_;
      }
    }

    auto total_score = place_edit_dist + areas_edit_dist;
    for (auto const [t_idx, token] : utl::enumerate(tokens)) {
      if ((matched_mask & (1U << t_idx)) == 0U) {
        total_score += token.size() * 2U;
      }
    }
    ctx.suggestions_.emplace_back(
        suggestion{.location_ = place,
                   .coordinates_ = t.place_coordinates_[place],
                   .area_set_ = area_set_idx,
                   .matched_areas_ = matched_areas,
                   .score_ = total_score});
  }

  UTL_STOP_TIMING(t);
  trace("PLACES: {} ms\n", UTL_TIMING_MS(t));
}

template <bool Debug, typename T>
void get_match_score(guess_context& ctx,
                     typeahead const& t,
                     cista::raw::vector_map<T, uint8_t> const& match_counts,
                     std::vector<cos_sim_match<T>> const& matches,
                     cista::offset::vector_map<T, string_idx_t> const& names,
                     std::vector<phrase_match_scores_t>& phrase_match_scores) {
  UTL_START_TIMING(t);
  phrase_match_scores.resize(matches.size());
  for (auto const& [i, m] : utl::enumerate(matches)) {
    for (auto const& [j, p] : utl::enumerate(ctx.phrases_)) {
      phrase_match_scores[i][j] = get_match_score(
          t.strings_[names[m.idx_]].view(), p.s_, ctx.lev_dist_, ctx.tmp_);
    }
  }
  UTL_STOP_TIMING(t);
  trace("{}: match scores [{} ms]\n", cista::type_str<T>(), UTL_TIMING_MS(t));
}

template <bool Debug, typename T>
void get_scored_matches(
    typeahead const& t,
    guess_context& ctx,
    std::uint8_t const numeric_tokens_mask,
    data::vector_map<T, string_idx_t> const& names,
    std::vector<cos_sim_match<T>> const& filtered,
    std::vector<phrase_match_scores_t> const& phrase_match_scores,
    std::vector<scored_match<T>>& scored_matches) {
  UTL_START_TIMING(t);

  scored_matches.clear();
  //  auto i = 0U;
  for (auto const& [i, m] : utl::enumerate(filtered)) {
    for (auto p_idx = phrase_idx_t{0U}; p_idx != ctx.phrases_.size(); ++p_idx) {
      if ((ctx.phrases_[p_idx].token_bits_ & numeric_tokens_mask) != 0U) {
        continue;
      }

      auto const p_match_score = phrase_match_scores[i][p_idx];
      //      std::cout << cista::type_str<T>() << " " << i
      //                << ": name=" << t.strings_[names[m.idx_]].view()
      //                << ", cos_sim=" << m.cos_sim_ << ", score=" <<
      //                p_match_score
      //                << ", phrase=" << ctx.phrases_[p_idx].s_ << " -> ";

      if (p_match_score != kNoMatch &&
          (scored_matches.size() != kMaxScoredMatches ||
           scored_matches.back().score_ > p_match_score)) {
        auto const [it, inserted] = utl::insert_sorted(
            scored_matches,
            {.score_ = p_match_score, .phrase_idx_ = p_idx, .idx_ = m.idx_});
        //        if (inserted) {
        //          std::cout << std::distance(begin(scored_matches), it);
        //        }
        scored_matches.resize(
            std::min(kMaxScoredMatches, scored_matches.size()));
      }

      //      std::cout << "\n";
    }

    //    ++i;
  }

  UTL_STOP_TIMING(t);
  trace("{}: score matches [{} ms]\n", cista::type_str<T>(), UTL_TIMING_MS(t));
}

template <bool Debug>
void get_suggestions(typeahead const& t,
                     geo::latlng const& /* coord */,
                     std::string_view in,
                     unsigned n_suggestions,
                     guess_context& ctx) {
  UTL_START_TIMING(t);

  ctx.suggestions_.clear();
  if (in.size() < 3) {
    return;
  }

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

  get_match_score<Debug>(ctx, t, ctx.street_match_counts_, ctx.street_matches_,
                         t.street_names_, ctx.street_phrase_match_scores_);
  get_match_score<Debug>(ctx, t, ctx.place_match_counts_, ctx.place_matches_,
                         t.place_names_, ctx.place_phrase_match_scores_);

  utl::fill(ctx.area_active_, false);

  auto const numeric_tokens_mask = get_numeric_tokens_mask(tokens);

  get_scored_matches<Debug>(
      t, ctx, numeric_tokens_mask, t.street_names_, ctx.street_matches_,
      ctx.street_phrase_match_scores_, ctx.scored_street_matches_);
  get_scored_matches<Debug>(t, ctx, numeric_tokens_mask, t.place_names_,
                            ctx.place_matches_, ctx.place_phrase_match_scores_,
                            ctx.scored_place_matches_);

  match_streets<Debug>(numeric_tokens_mask, t, ctx, tokens);
  match_places<Debug>(numeric_tokens_mask, t, ctx, tokens);

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