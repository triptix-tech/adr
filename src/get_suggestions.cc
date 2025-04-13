#include "adr/adr.h"

#include "fmt/ranges.h"

#include "utl/erase_duplicates.h"
#include "utl/helpers/algorithm.h"
#include "utl/insert_sorted.h"
#include "utl/timing.h"
#include "utl/to_vec.h"
#include "utl/zip.h"

#include "cista/containers/flat_matrix.h"

#include "adr/score.h"
#include "adr/trace.h"
#include "adr/typeahead.h"

namespace adr {

constexpr auto const kMaxScoredMatches = std::size_t{10000};

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
                    area_set_idx_t const area_set_idx,
                    language_list_t const languages) {
  for (auto const area : t.area_sets_[area_set_idx]) {
    if (ctx.area_active_[to_idx(area)]) {
      continue;
    }

    ctx.area_active_[to_idx(area)] = true;
    for (auto const& [j, area_p] : utl::enumerate(ctx.phrases_)) {
      auto const match_allowed = (  // Zip-code areas only match numeric tokens.
          (((area_p.token_bits_ & numeric_tokens_mask) == area_p.token_bits_) &&
           t.area_admin_level_[area] == kPostalCodeAdminLevel) ||
          // No-zip-code areas only match non-numeric tokens.
          (((area_p.token_bits_ & numeric_tokens_mask) == 0U) &&
           t.area_admin_level_[area] != kPostalCodeAdminLevel)  //
      );

      ctx.area_phrase_match_scores_[area][j] = kNoMatch;
      if (!match_allowed) {
        continue;
      }

      // Determine best match for all languages.
      auto& score = ctx.area_phrase_match_scores_[area][j];
      auto& lang = ctx.area_phrase_lang_[area][j];
      for (auto const [i, l] : utl::enumerate(languages)) {
        auto const lang_idx = find_lang(t.area_name_lang_[area], l);
        if (lang_idx == -1) {
          continue;
        }

        auto const area_name = t.strings_[t.area_names_[area][lang_idx]].view();
        auto const lang_match_score = get_match_score(
            area_name, area_p.s_, ctx.sift4_offset_arr_, ctx.normalize_buf_);
        if (lang_match_score < score) {
          score = lang_match_score;
          lang = lang_idx;
        }
      }
    }
  }
}

template <bool Debug>
void match_streets(std::uint8_t const all_tokens_mask,
                   std::uint8_t const numeric_tokens_mask,
                   typeahead const& t,
                   guess_context& ctx,
                   std::vector<std::string> const& tokens,
                   language_list_t const languages) {
  UTL_START_TIMING(t);

  auto i = 0U;
  for (auto const [street_edit_dist, street_p_idx, str_idx, street] :
       ctx.scored_street_matches_) {
    ctx.area_match_items_.clear();

    for (auto const [index, area_set] :
         utl::enumerate(t.street_areas_[street])) {
      ctx.area_match_items_[area_set].emplace_back(
          match_item{.type_ = match_item::type::kStreet,
                     .score_ = 0.0F,
                     .index_ = static_cast<std::uint32_t>(index),
                     .matched_mask_ = ctx.phrases_[street_p_idx].token_bits_});
    }

    auto index = 0U;
    for (auto const [hn, areas_idx] :
         utl::zip(t.house_numbers_[street], t.house_areas_[street])) {
      for (auto const& [hn_p_idx, p] : utl::enumerate(ctx.phrases_)) {
        if ((p.token_bits_ & numeric_tokens_mask) != p.token_bits_) {
          continue;
        }

        auto const hn_score =
            get_match_score(t.strings_[hn].view(), p.s_, ctx.sift4_offset_arr_,
                            ctx.normalize_buf_);
        if (hn_score == kNoMatch) {
          continue;
        }

        ctx.area_match_items_[areas_idx].emplace_back(match_item{
            .type_ = match_item::type::kHouseNumber,
            .score_ = t.strings_[hn].view() == p.s_ ? -2.5F : hn_score,
            .index_ = index,
            .house_number_p_idx_ = static_cast<std::uint8_t>(hn_p_idx),
            .matched_mask_ = static_cast<std::uint8_t>(
                ctx.phrases_[street_p_idx].token_bits_ |
                ctx.phrases_[hn_p_idx].token_bits_)});
      }
      ++index;
    }

    for (auto const& [area_set_idx, items] : ctx.area_match_items_) {
      activate_areas(t, ctx, numeric_tokens_mask, area_set_idx, languages);

      ctx.item_matched_masks_.clear();
      for (auto const& item : items) {
        ctx.item_matched_masks_.emplace(item.matched_mask_);
      }

      // For each phrase: greedily match an area name
      // IF matching distance is below the no-match-penalty threshold.
      for (auto const item_matched_mask : ctx.item_matched_masks_) {
        auto matched_tokens_mask = item_matched_mask;
        auto matched_areas_mask = std::uint32_t{0U};
        auto area_lang = area_set_lang_t{};
        auto areas_edit_dist = 0.0F;
        for (auto const& [area_p_idx, area_p] : utl::enumerate(ctx.phrases_)) {
          auto best_edit_dist = std::numeric_limits<float>::max();
          auto best_area_idx = 0U;

          if ((area_p.token_bits_ & matched_tokens_mask) != 0U) {
            trace("[{}] {} [p={}] {} -> ALREADY MATCHED", street,
                  t.strings_[t.street_names_[street][kDefaultLangIdx]].view(),
                  ctx.phrases_[street_p_idx].s_, area_p.s_);
            continue;
          }

          for (auto const [area_idx, area] :
               utl::enumerate(t.area_sets_[area_set_idx])) {
            auto const match_allowed =
                (  // Zip-code areas only match numeric tokens.
                    (((area_p.token_bits_ & numeric_tokens_mask) ==
                      area_p.token_bits_) &&
                     t.area_admin_level_[area] == kPostalCodeAdminLevel) ||
                    // No-zip-code areas only match non-numeric tokens.
                    (((area_p.token_bits_ & numeric_tokens_mask) == 0U) &&
                     t.area_admin_level_[area] != kPostalCodeAdminLevel)  //
                );

            if (!match_allowed) {
              trace("[{}] {} [p={}]\t\t\t{} vs {} -> NOT ALLOWED", street,
                    t.strings_[t.street_names_[street][kDefaultLangIdx]].view(),
                    ctx.phrases_[street_p_idx].s_,
                    t.strings_[t.area_names_[area][kDefaultLangIdx]].view(),
                    area_p.s_);
              continue;
            }

            auto const edit_dist =
                ctx.area_phrase_match_scores_[area][area_p_idx];

            trace("[{}] {} [p={}]\t\t\t{} [idx={}] vs {} [area_p_idx={}] -> {}",
                  street,
                  t.strings_[t.street_names_[street][kDefaultLangIdx]].view(),
                  ctx.phrases_[street_p_idx].s_,
                  t.strings_[t.area_names_[area][kDefaultLangIdx]].view(), area,
                  area_p.s_, area_p_idx, edit_dist);

            if (best_edit_dist > edit_dist) {
              best_edit_dist = edit_dist;
              best_area_idx = area_idx;
            }
          }

          if (best_edit_dist != kNoMatch) {
            auto const best_area = t.area_sets_[area_set_idx][best_area_idx];
            matched_areas_mask |= (1U << best_area_idx);
            area_lang[best_area_idx] =
                ctx.area_phrase_lang_[best_area][area_p_idx];
            areas_edit_dist += best_edit_dist;
            areas_edit_dist -=
                (t.area_population_[best_area].get() / 10'000'000.0F) * 2U;
            matched_tokens_mask |= area_p.token_bits_;

            trace("[{}] {}\t\t\t***MATCHED: {} vs {}: {}", street,
                  t.strings_[t.street_names_[street][kDefaultLangIdx]].view(),
                  t.strings_
                      [t.area_names_[t.area_sets_[area_set_idx][best_area_idx]]
                                    [kDefaultLangIdx]]
                          .view(),
                  area_p.s_, best_edit_dist);
          }
        }

        for (auto const& item : items) {
          if (item.matched_mask_ != item_matched_mask) {
            continue;
          }

          auto total_score = street_edit_dist + areas_edit_dist + item.score_;
          for (auto const [t_idx, token] : utl::enumerate(tokens)) {
            if ((matched_tokens_mask & (1U << t_idx)) == 0U) {
              total_score += token.size() * 3.0F;

              trace("[{}] {} [p={}]\t\t\t***NOTHING MATCHED: {} --> penalty={}",
                    street,
                    t.strings_[t.street_names_[street][kDefaultLangIdx]].view(),
                    ctx.phrases_[street_p_idx].s_, token,
                    (token.size() * 3.0F));
            }
          }

          auto const areas_bonus = std::popcount(matched_areas_mask) * 2.0F;
          auto const no_area_score =
              !matched_areas_mask && matched_tokens_mask == all_tokens_mask
                  ? (item.type_ == match_item::type::kHouseNumber ? 3.F : 1.6F)
                  : 0.F;

          total_score -= areas_bonus;
          total_score -= no_area_score;

          trace(
              "[{}] {} FINAL: street_edit_dist={}, areas_edit_dist={}, "
              "item_score={}, areas_bonus={}, no_area_score={} => "
              "total_score={}",
              street,
              t.strings_[t.street_names_[street][kDefaultLangIdx]].view(),
              street_edit_dist, areas_edit_dist, item.score_,
              std::popcount(matched_areas_mask) * 2.0, no_area_score,
              total_score);

          ctx.suggestions_.emplace_back(suggestion{
              .str_ = str_idx,
              .location_ =
                  address{
                      .street_ = street,
                      .house_number_ =
                          item.type_ == match_item::type::kHouseNumber
                              ? item.index_
                              : address::kNoHouseNumber,
                  },
              .coordinates_ = item.type_ == match_item::type::kHouseNumber
                                  ? t.house_coordinates_[street][item.index_]
                                  : t.street_pos_[street][item.index_],
              .area_set_ = area_set_idx,
              .matched_area_lang_ = area_lang,
              .matched_areas_ = matched_areas_mask,
              .matched_tokens_ = matched_tokens_mask,
              .score_ = total_score});
        }
      }
    }
  }

  UTL_STOP_TIMING(t);
  trace("STREETS: {} ms", UTL_TIMING_MS(t));
}

template <bool Debug>
void match_places(std::uint8_t const all_tokens_mask,
                  std::uint8_t const numeric_tokens_mask,
                  typeahead const& t,
                  guess_context& ctx,
                  std::vector<std::string> const& tokens,
                  language_list_t const& languages) {
  UTL_START_TIMING(t);

  auto ii = 0U;
  for (auto const [place_edit_dist, place_p_idx, str_idx, place] :
       ctx.scored_place_matches_) {
    auto const area_set_idx = t.place_areas_[place];

    trace("[{}] {}: edit_dist={}, phrase={}, type={}", ii,
          t.strings_[t.place_names_[place][kDefaultLangIdx]].view(),
          place_edit_dist, ctx.phrases_[place_p_idx].s_,
          t.place_type_[place] == place_type::kExtra ? "EXT" : "REG");

    activate_areas(t, ctx, numeric_tokens_mask, area_set_idx, languages);

    auto matched_tokens_mask = ctx.phrases_[place_p_idx].token_bits_;
    auto matched_areas_mask = std::uint32_t{0U};
    auto area_lang = area_set_lang_t{};
    auto areas_edit_dist = 0.0F;
    for (auto const& [area_p_idx, area_p] : utl::enumerate(ctx.phrases_)) {
      auto best_edit_dist = std::numeric_limits<float>::max();
      auto best_area_idx = 0U;

      if ((area_p.token_bits_ & matched_tokens_mask) != 0U) {
        continue;
      }

      for (auto const [area_idx, area] :
           utl::enumerate(t.area_sets_[area_set_idx])) {
        auto const match_allowed =
            (  // Zip-code areas only match numeric tokens.
                (((area_p.token_bits_ & numeric_tokens_mask) ==
                  area_p.token_bits_) &&
                 t.area_admin_level_[area] == kPostalCodeAdminLevel) ||
                // No-zip-code areas only match non-numeric tokens.
                (((area_p.token_bits_ & numeric_tokens_mask) == 0U) &&
                 t.area_admin_level_[area] != kPostalCodeAdminLevel)  //
            );

        if (!match_allowed) {
          continue;
        }

        auto const edit_dist = ctx.area_phrase_match_scores_[area][area_p_idx];

        trace(
            "[{}] {}: [place_phrase={}, place_edit_dist={}]: {} vs {}, "
            "score={}",
            ii, t.strings_[t.place_names_[place][kDefaultLangIdx]].view(),
            ctx.phrases_[place_p_idx].s_, place_edit_dist,
            ctx.phrases_[area_p_idx].s_,
            t.strings_[t.area_names_[area][kDefaultLangIdx]].view(), edit_dist);

        if (best_edit_dist > edit_dist) {
          best_edit_dist = edit_dist;
          best_area_idx = area_idx;
        }
      }

      if (best_edit_dist != kNoMatch) {
        trace(
            "[{}] {}: matched {} vs {}, score={}", ii,
            t.strings_[t.place_names_[place][kDefaultLangIdx]].view(),
            ctx.phrases_[area_p_idx].s_,
            t.strings_[t.area_names_[t.area_sets_[area_set_idx][best_area_idx]]
                                    [kDefaultLangIdx]]
                .view(),
            best_edit_dist);

        auto const best_area = t.area_sets_[area_set_idx][best_area_idx];
        matched_areas_mask |= (1U << best_area_idx);
        area_lang[best_area_idx] = ctx.area_phrase_lang_[best_area][area_p_idx];
        areas_edit_dist += best_edit_dist;
        areas_edit_dist -=
            (t.area_population_[best_area].get() / 10'000'000.0F) * 2U;
        matched_tokens_mask |= area_p.token_bits_;
      }
    }

    auto total_score = place_edit_dist + areas_edit_dist;
    for (auto const [t_idx, token] : utl::enumerate(tokens)) {
      if ((matched_tokens_mask & (1U << t_idx)) == 0U) {
        total_score += token.size() * 3U;

        trace("[{}] {}: [p={}]\t\t\tNOTHING MATCHED: {} --> {}", ii,
              t.strings_[t.place_names_[place][kDefaultLangIdx]].view(),
              ctx.phrases_[place_p_idx].s_, token, token.size() * 3.0F);
      }
    }

    auto const extra_score =
        t.place_type_[place] == place_type::kExtra ? 0.75 : 0;
    auto const areas_score = std::popcount(matched_areas_mask);
    auto const no_area_score =
        !matched_areas_mask && matched_tokens_mask == all_tokens_mask ? 3 : 0;
    auto const population_score =
        std::min(1.5, (t.place_population_[place].get() / 1'000'000.F) * 1.5);
    auto const place_score = 1.0;

    total_score -= extra_score;
    total_score -= areas_score;
    total_score -= no_area_score;
    total_score -= population_score;
    total_score -= place_score;

    trace(
        "[{}] {} FINAL: place_edit_dist={}, areas_edit_dist={}, "
        "areas_bonus={}, no_area_score={}, population_score={} [{}], "
        "place_score={}, extra_score={} => {}",
        ii, t.strings_[t.place_names_[place][kDefaultLangIdx]].view(),
        place_edit_dist, areas_edit_dist, areas_score, no_area_score,
        population_score, t.place_population_[place].get(), place_score,
        extra_score, total_score);

    // Add if it's not a duplicate of the previous one
    // or improves upon the previous one.
    if (ctx.suggestions_.empty() ||
        ctx.suggestions_.back().score_ <= total_score ||
        !holds_alternative<place_idx_t>(ctx.suggestions_.back().location_) ||
        get<place_idx_t>(ctx.suggestions_.back().location_) != place) {
      auto& back [[maybe_unused]] = ctx.suggestions_.emplace_back(
          suggestion{.str_ = str_idx,
                     .location_ = place,
                     .coordinates_ = t.place_coordinates_[place],
                     .area_set_ = area_set_idx,
                     .matched_area_lang_ = area_lang,
                     .matched_areas_ = matched_areas_mask,
                     .matched_tokens_ = matched_tokens_mask,
                     .score_ = total_score});
      if constexpr (Debug) {
        std::cout << "ADDED: ";
        back.print(std::cout, t, languages);
      }
    }

    ++ii;
  }

  UTL_STOP_TIMING(t);
  trace("PLACES: {} ms", UTL_TIMING_MS(t));
}

template <bool Debug>
void compute_string_phrase_match_scores(guess_context& ctx,
                                        typeahead const& t) {
  UTL_START_TIMING(t);
  ctx.string_phrase_match_scores_.resize(ctx.string_matches_.size());
  for (auto const& [i, m] : utl::enumerate(ctx.string_matches_)) {
    for (auto const& [j, p] : utl::enumerate(ctx.phrases_)) {
      ctx.string_phrase_match_scores_[i][j] =
          get_match_score(t.strings_[m.idx_].view(), p.s_,
                          ctx.sift4_offset_arr_, ctx.normalize_buf_);
    }
  }
  UTL_STOP_TIMING(t);
  trace("match scores [{} ms]", UTL_TIMING_MS(t));
}

template <bool Debug>
void get_scored_matches(typeahead const& t,
                        guess_context& ctx,
                        std::uint8_t const numeric_tokens_mask,
                        language_list_t const& languages,
                        filter_type const filter) {
  UTL_START_TIMING(t);

  ctx.scored_street_matches_.clear();
  ctx.scored_place_matches_.clear();

  for (auto const& [i, m] : utl::enumerate(ctx.string_matches_)) {
    for (auto p_idx = phrase_idx_t{0U}; p_idx != ctx.phrases_.size(); ++p_idx) {
      if ((ctx.phrases_[p_idx].token_bits_ & numeric_tokens_mask) != 0U) {
        continue;
      }

      auto const p_match_score = ctx.string_phrase_match_scores_[i][p_idx];
      trace("{}: name={}, cos_sim={}, score={}, phrase={}", i,
            t.strings_[m.idx_].view(), m.cos_sim_, p_match_score,
            ctx.phrases_[p_idx].s_);

      if (p_match_score == kNoMatch) {
        continue;
      }

      for (auto const [idx, type] :
           utl::zip(t.string_to_location_[m.idx_], t.string_to_type_[m.idx_])) {
        switch (type) {
          case location_type_t::kStreet: {
            if (filter != filter_type::kNone &&
                filter != filter_type::kAddress) {
              continue;
            }
            auto const street_idx = street_idx_t{idx};
            if (ctx.scored_street_matches_.size() != kMaxScoredMatches ||
                ctx.scored_street_matches_.back().score_ > p_match_score) {
              utl::insert_sorted(ctx.scored_street_matches_,
                                 {.score_ = p_match_score,
                                  .phrase_idx_ = p_idx,
                                  .string_idx_ = m.idx_,
                                  .idx_ = street_idx});
              ctx.scored_street_matches_.resize(std::min(
                  kMaxScoredMatches, ctx.scored_street_matches_.size()));
            }
            break;
          }

          case location_type_t::kPlace:
            auto const place_idx = place_idx_t{idx};
            if (filter != filter_type::kNone &&
                (filter == filter_type::kAddress ||
                 ((filter == filter_type::kExtra) !=
                  (t.place_type_[place_idx] == place_type::kExtra)))) {
              continue;
            }
            if (ctx.scored_place_matches_.size() != kMaxScoredMatches ||
                ctx.scored_place_matches_.back().score_ > p_match_score) {
              utl::insert_sorted(ctx.scored_place_matches_,
                                 {.score_ = p_match_score,
                                  .phrase_idx_ = p_idx,
                                  .string_idx_ = m.idx_,
                                  .idx_ = place_idx});
              ctx.scored_place_matches_.resize(std::min(
                  kMaxScoredMatches, ctx.scored_place_matches_.size()));
            }
            break;
        }
      }
    }
  }

  UTL_STOP_TIMING(t);
  trace("score matches [{} ms]", UTL_TIMING_MS(t));
}

template <bool Debug>
std::vector<token> get_suggestions(typeahead const& t,
                                   std::string in,
                                   unsigned n_suggestions,
                                   language_list_t const& languages,
                                   guess_context& ctx,
                                   std::optional<geo::latlng> const& coord,
                                   double const bias,
                                   filter_type const filter) {
  UTL_START_TIMING(t);

  erase_fillers(in);

  ctx.suggestions_.clear();
  if (in.size() < 3) {
    return {};
  }

  auto token_pos = std::vector<token>{};
  auto tokens = std::vector<std::string>{};
  auto all_tokens_mask = std::uint8_t{0U};
  utl::for_each_token(utl::cstr{in}, ' ', [&, i = 0U](utl::cstr t) mutable {
    if (t.empty()) {
      return;
    }
    tokens.emplace_back(normalize(t.view(), ctx.normalize_buf_));
    all_tokens_mask |= 1U << (i++);

    token_pos.push_back(token{static_cast<std::uint16_t>(t.data() - in.data()),
                              static_cast<std::uint16_t>(t.length())});
  });
  ctx.phrases_ = get_phrases(tokens);

  t.guess<Debug>(normalize(in, ctx.normalize_buf_), ctx);

  compute_string_phrase_match_scores<Debug>(ctx, t);

  utl::fill(ctx.area_active_, false);

  auto const numeric_tokens_mask = get_numeric_tokens_mask(tokens);

  get_scored_matches<Debug>(t, ctx, numeric_tokens_mask, languages, filter);

  match_streets<Debug>(all_tokens_mask, numeric_tokens_mask, t, ctx, tokens,
                       languages);
  match_places<Debug>(all_tokens_mask, numeric_tokens_mask, t, ctx, tokens,
                      languages);

  UTL_STOP_TIMING(t);
  trace("{} suggestions [{} ms]", ctx.suggestions_.size(), UTL_TIMING_MS(t));

  if (ctx.suggestions_.empty()) {
    return token_pos;
  }

  if (coord.has_value()) {
    for (auto& s : ctx.suggestions_) {
      auto const dist = geo::distance(s.coordinates_.as_latlng(), *coord);

      auto dist_bonus = 0.0;
      if (dist < 10'000) {
        dist_bonus = 8.1 * bias;  // 10 km bonus
      } else if (dist < 100'000) {
        dist_bonus = 4.1 * bias;  // 100 km bonus
      } else if (dist < 1'000'000) {
        dist_bonus = 2.1 * bias;  // 1000 km bonus
      }

      if constexpr (Debug) {
        s.print(std::cout, t, languages);
        std::cout << "dist=" << dist << "  -> bonus = " << dist_bonus
                  << " (score=" << s.score_ - dist_bonus << ")" << "\n";
      }
      s.score_ -= dist_bonus;
    }
  }

  UTL_START_TIMING(sort);
  //  auto const result_count = static_cast<std::ptrdiff_t>(
  //      std::min(std::size_t{n_suggestions * 10U}, ctx.suggestions_.size()));
  //  std::nth_element(begin(ctx.suggestions_),
  //                   begin(ctx.suggestions_) + result_count,
  //                   end(ctx.suggestions_));
  //  ctx.suggestions_.resize(result_count);
  std::sort(begin(ctx.suggestions_), end(ctx.suggestions_));
  ctx.suggestions_.erase(
      std::unique(begin(ctx.suggestions_), end(ctx.suggestions_),
                  [&](suggestion const& a, suggestion const& b) {
                    return a.location_ == b.location_;
                  }),
      end(ctx.suggestions_));
  UTL_STOP_TIMING(sort);
  trace("sort [{} us]", UTL_TIMING_US(sort));

  if constexpr (Debug) {
    for (auto const& [i, s] : utl::enumerate(ctx.suggestions_)) {
      std::cout << "[" << i << "]\t";
      s.print(std::cout, t, languages);
    }
  }

  ctx.suggestions_.resize(std::min(static_cast<std::size_t>(n_suggestions),
                                   ctx.suggestions_.size()));

  return token_pos;
}

template std::vector<token> get_suggestions<true>(
    typeahead const&,
    std::string,
    unsigned,
    language_list_t const&,
    guess_context&,
    std::optional<geo::latlng> const&,
    double,
    filter_type);

template std::vector<token> get_suggestions<false>(
    typeahead const&,
    std::string,
    unsigned,
    language_list_t const&,
    guess_context&,
    std::optional<geo::latlng> const&,
    double,
    filter_type);

}  // namespace adr