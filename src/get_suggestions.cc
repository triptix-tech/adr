#include "adr/adr.h"

#include "fmt/ranges.h"

#include "utl/timing.h"
#include "utl/to_vec.h"

#include "cista/containers/flat_matrix.h"

#include "adr/score.h"
#include "adr/trace.h"
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

  auto input_hashes = cista::raw::ankerl_set<cista::hash_t>{};
  auto input_vec = std::vector<std::string>{};
  utl::for_each_token(utl::cstr{ctx.tmp_}, ' ', [&](utl::cstr s) {
    input_hashes.emplace(cista::hash(s.view()));
    auto const start = &s.view()[0];
    auto const size = ctx.tmp_.size() - (start - &ctx.tmp_[0]);
    input_vec.emplace_back(start, size);
  });

  auto const get_edit_dist =
      [&]<typename T>(cista::raw::vector_map<T, std::uint8_t>& match_counts,
                      std::vector<match<T>>& matches,
                      data::vector_map<T, string_idx_t> const& names) {
        for (auto& m : matches) {
          for (auto const& [j, p] : utl::enumerate(ctx.phrases_)) {
            m.edit_dist_[j] =
                levenshtein_distance_normalize(t.strings_[names[m.idx_]].view(),
                                               p.s_, ctx.lev_dist_, ctx.tmp_);
          }
          trace("  {} {} [edit_dist={}]\n", cista::type_str<T>(),
                t.strings_[names[m.idx_]].view(),
                std::span{begin(m.edit_dist_), ctx.phrases_.size()});
        }
      };
  get_edit_dist(ctx.street_match_counts_, ctx.street_matches_, t.street_names_);
  get_edit_dist(ctx.place_match_counts_, ctx.place_matches_, t.place_names_);
  get_edit_dist(ctx.area_match_counts_, ctx.area_matches_, t.area_names_);

  auto area_edit_dist =
      cista::raw::vector_map<area_idx_t, std::array<float, kMaxInputPhrases>>{};
  auto area_active = std::vector<bool>{};
  area_active.resize(t.area_names_.size());
  area_edit_dist.resize(t.area_names_.size(), inf_edit_dist<float>());
  for (auto const m : ctx.area_matches_) {
    area_edit_dist[m.idx_] = m.edit_dist_;
    area_active[to_idx(m.idx_)] = true;
  }

  constexpr auto const kMaxMatches = std::size_t{1000U};

  auto const numeric_tokens_mask = get_numeric_tokens_mask(tokens);
  auto street_matches =
      std::vector<std::tuple<float, std::uint8_t, street_idx_t>>{};
  for (auto const& street_match : ctx.street_matches_) {
    auto const street = street_match.idx_;
    auto const street_name = t.strings_[t.street_names_[street]].view();
    auto const& normalized_street_name = normalize(street_name, ctx.tmp_);
    for (auto p_idx = 0U; p_idx != ctx.phrases_.size(); ++p_idx) {
      if ((ctx.phrases_[p_idx].token_bits_ & numeric_tokens_mask) != 0U) {
        continue;
      }

      auto const edit_dist = street_match.edit_dist_[p_idx];
      if (edit_dist != std::numeric_limits<std::uint8_t>::max() &&
          (street_matches.size() != kMaxMatches ||
           std::get<0>(street_matches.back()) >= edit_dist)) {
        utl::insert_sorted(
            street_matches, {edit_dist, p_idx, street},
            [](auto&& a, auto&& b) { return std::get<0>(a) < std::get<0>(b); });
        street_matches.resize(std::min(kMaxMatches, street_matches.size()));
      }
    }
  }

  for (auto const [street_edit_dist, street_p_idx, street] : street_matches) {
    ctx.areas_.clear();

    std::cout << t.strings_[t.street_names_[street]].view()
              << ", edit_dist=" << street_edit_dist
              << ", phrase=" << ctx.phrases_[street_p_idx].s_ << "\n";

    for (auto const [i, area_set] : utl::enumerate(t.street_areas_[street])) {
      ctx.areas_[area_set].emplace_back(
          area_src{.type_ = area_src::type::kStreet,
                   .dist_ = 0.0F,
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

        auto const hn_edit_dist = levenshtein_distance_normalize(
            t.strings_[hn].view(), p.s_, ctx.lev_dist_, ctx.tmp_);
        if (hn_edit_dist < std::numeric_limits<float>::max()) {
          ctx.areas_[areas_idx].emplace_back(area_src{
              .type_ = area_src::type::kHouseNumber,
              .dist_ = hn_edit_dist,
              .index_ = i,
              .house_number_p_idx_ = static_cast<std::uint8_t>(hn_p_idx),
              .matched_mask_ = static_cast<std::uint8_t>(
                  ctx.phrases_[street_p_idx].token_bits_ |
                  ctx.phrases_[hn_p_idx].token_bits_)});
        }
      }
      ++i;
    }

    for (auto const& [area_set_idx, items] : ctx.areas_) {
      // Activate postal code areas.
      for (auto const area : t.area_sets_[area_set_idx]) {
        if (!area_active[to_idx(area)] &&
            t.area_admin_level_[area] == kPostalCodeAdminLevel) {
          auto const area_name = t.strings_[t.area_names_[area]].view();
          area_active[to_idx(area)] = true;
          for (auto const& [j, p] : utl::enumerate(ctx.phrases_)) {
            area_edit_dist[area][j] =
                ((p.token_bits_ & numeric_tokens_mask) == p.token_bits_)
                    ? levenshtein_distance_normalize(area_name, p.s_,
                                                     ctx.lev_dist_, ctx.tmp_)
                    : std::numeric_limits<float>::max();
          }
        }
      }

      ctx.item_tabu_masks_.clear();
      for (auto const& item : items) {
        ctx.item_tabu_masks_.emplace(item.matched_mask_);
      }

      // For each phrase: greedily match an area name
      // IF matching distance is below the no-match-penalty threshold.
      for (auto const item_matched_mask : ctx.item_tabu_masks_) {
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
                area_active[to_idx(area)];

            if (!match_allowed) {
              continue;
            }

            auto const edit_dist = area_edit_dist[area][area_p_idx];
            if (best_edit_dist > edit_dist) {
              std::cout << t.strings_[t.street_names_[street]].view()
                        << "      matched " << area_p.s_ << " vs " << area_name
                        << ": score=" << edit_dist << "\n";
              best_edit_dist = edit_dist;
              best_area_idx = area_idx;
            } else {
              std::cout << t.strings_[t.street_names_[street]].view()
                        << "      NOT MATCHED " << area_p.s_ << " vs "
                        << area_name << ": score=" << edit_dist << "\n";
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
            auto total_score = street_edit_dist + areas_edit_dist + item.dist_;
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