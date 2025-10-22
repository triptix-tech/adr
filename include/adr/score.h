#pragma once

#include <bitset>

#include "utl/parser/cstr.h"

#include "adr/normalize.h"
#include "adr/sift4.h"
#include "adr/types.h"

namespace adr {

inline edit_dist_t levenshtein_distance(std::string_view source,
                                        std::string_view target,
                                        std::vector<edit_dist_t>& lev_dist) {
  if (source.size() > target.size()) {
    std::swap(source, target);
  }

  auto const min_size = source.size();
  auto const max_size = target.size();
  auto const limit = (min_size / 2) + 2;
  lev_dist.resize(min_size + 1);

  for (auto i = 0U; i <= min_size; ++i) {
    lev_dist[i] = static_cast<edit_dist_t>(i);
  }

  for (auto j = 1U; j <= max_size; ++j) {
    auto previous_diagonal = lev_dist[0];
    auto previous_diagonal_save = edit_dist_t{0U};
    ++lev_dist[0];

    for (auto i = 1U; i <= min_size; ++i) {
      previous_diagonal_save = lev_dist[i];
      if (source[i - 1] == target[j - 1]) {
        lev_dist[i] = previous_diagonal;
      } else {
        lev_dist[i] = std::min(
            std::min(
                static_cast<edit_dist_t>(lev_dist[i - 1] + 1U /* delete */),
                static_cast<edit_dist_t>(lev_dist[i] + 1U /* insert */)),
            static_cast<edit_dist_t>(previous_diagonal + 1U /* replace */));
      }

      previous_diagonal = previous_diagonal_save;
    }

    if (lev_dist[std::min(min_size, static_cast<std::size_t>(j - 1))] > limit) {
      return std::numeric_limits<edit_dist_t>::max();
    }
  }

  return lev_dist[min_size];
}

inline score_t get_token_match_score(
    std::string_view dataset_token,
    std::string_view p,
    std::vector<sift_offset>& sift4_offset_arr) {
  auto const cut_normalized_str =
      dataset_token.substr(0U, std::min(dataset_token.size(), p.size()));
  //  std::vector<edit_dist_t> lev_dist;
  //  auto const dist = levenshtein_distance(cut_normalized_str, p, lev_dist);
  auto const dist =
      sift4(cut_normalized_str, p, 3,
            static_cast<edit_dist_t>(
                std::min(dataset_token.size(), p.size()) / 2U + 2U),
            sift4_offset_arr);

  if (dist >= cut_normalized_str.size()) {
    // std::cout << "dist=" << static_cast<int>(dist) << " > "
    //           << cut_normalized_str.size() << "\n";
    return kNoMatch;
  }
  auto const overhang_penality =
      (static_cast<float>(dataset_token.size() -
                          std::min(dataset_token.size(), p.size())) /
       3.0F);
  auto const relative_coverage =
      6.0F * (static_cast<float>(dist) /
              static_cast<float>(cut_normalized_str.size()));

  auto common_prefix_bonus = 0.0F;
  auto const end = std::min(cut_normalized_str.size(), p.size());
  for (auto i = 0U; i != end; ++i) {
    if (cut_normalized_str[i] != p[i]) {
      break;
    }
    common_prefix_bonus -= 0.25F;
  }

  auto const first_letter_mismatch_penality =
      cut_normalized_str[0] != p[0] ? 2.0F : -0.5F;
  auto const second_letter_mismatch_penality =
      cut_normalized_str.size() > 1 && p.size() > 1
          ? cut_normalized_str[1] != p[1] ? 1.0F : -0.25F
          : -0.25F;

  auto score = dist + first_letter_mismatch_penality +
               second_letter_mismatch_penality + overhang_penality +
               relative_coverage + common_prefix_bonus;
  // std::cout << "  " << dataset_token << " vs " << p
  //           << ": dist=" << static_cast<int>(dist)
  //           << ", size=" << cut_normalized_str.size()
  //           << ", overhang_penality=" << overhang_penality
  //           << ", relative_coverage=" << relative_coverage
  //           << ", first_letter_mismatch_penality="
  //           << first_letter_mismatch_penality
  //           << ", second_letter_mismatch_penality="
  //           << second_letter_mismatch_penality
  //           << ", common_prefix_bonus=" << common_prefix_bonus
  //           << ", score=" << score
  //           << ", max=" << (std::ceil(cut_normalized_str.size() / 2.0F))
  //           << "\n";

  return score > std::ceil(static_cast<float>(cut_normalized_str.size()) / 2.0F)
             ? kNoMatch
             : score;
}

template <typename... Delimiter>
std::string_view get_until(std::string_view s, Delimiter&&... d) {
  auto const it = std::find_if(begin(s), end(s),
                               [&](auto&& c) { return ((c == d) || ...); });
  return (it == end(s))
             ? s
             : std::string_view{s.data(), static_cast<std::size_t>(
                                              std::distance(begin(s), it))};
}

template <typename Fn, typename... Delimiter>
void for_each_token(std::string_view s, Fn&& f, Delimiter&&... d) {
  while (!s.empty()) {
    auto const token = get_until(s, std::forward<Delimiter>(d)...);
    if (f(token) == utl::continue_t::kBreak) {
      break;
    }
    s = s.substr(token.length());
    // skip separator
    if (!s.empty()) {
      s = s.substr(1U);
    }
  }
}

inline score_t get_match_score(
    std::string_view s,  // name from dataset - will be normalized!
    std::string_view p,  // input phrase - won't be normalized!
    std::vector<sift_offset>& sift4_offset_arr,
    utf8_normalize_buf_t& tmp) {
  if (s.empty() || p.empty()) {
    return kNoMatch;
  }

  auto normalized_str = std::string{normalize(s, tmp)};
  erase_fillers(normalized_str);

  auto s_tokens = std::vector<std::string_view>{};
  auto p_tokens = std::vector<std::string_view>{};

  for_each_token(
      p,
      [&, c = 0U](auto&& p_token) mutable {
        if (++c > 8) {
          return utl::continue_t::kBreak;
        }
        if (!p_token.empty()) {
          p_tokens.emplace_back(p_token);
        }
        return utl::continue_t::kContinue;
      },
      ' ', '-');

  for_each_token(
      normalized_str,
      [&, c = 0U](auto&& s_token) mutable {
        if (++c > 8) {
          return utl::continue_t::kBreak;
        }
        if (!s_token.empty()) {
          s_tokens.emplace_back(s_token);
        }
        return utl::continue_t::kContinue;
      },
      ' ', '-');

  if (p_tokens.size() > s_tokens.size()) {
    return get_token_match_score(normalized_str, p, sift4_offset_arr) +
           static_cast<float>((p_tokens.size() - s_tokens.size()) * 2U);
  }

  auto const fallback =
      get_token_match_score(normalized_str, p, sift4_offset_arr);
  if (p_tokens.size() == 1U && s_tokens.size() == 1U) {
    //    std::cout << "p_tokens=1, s_tokens=1 => " << fallback << "\n";
    return fallback;
  }

  auto const s_phrases = get_phrases(s_tokens);

  //  std::cout << s << " vs " << p << "\n";

  auto covered = std::uint8_t{0U};
  auto sum = 0.0F;
  for (auto p_idx = 0U; p_idx != p_tokens.size(); ++p_idx) {
    auto const& p_token = p_tokens[p_idx];

    auto best_s_score = kNoMatch;
    auto best_s_idx = 0U;

    for (auto s_idx = 0U; s_idx != s_phrases.size(); ++s_idx) {
      auto const& s_phrase = s_phrases[s_idx];
      if ((covered & s_phrase.token_bits_) != 0U) {
        continue;
      }

      auto const s_p_match_score =
          s_phrase.s_ == p_token
              ? -1.5F
              : get_token_match_score(s_phrase.s_, p_token, sift4_offset_arr);
      if (best_s_score > s_p_match_score) {
        best_s_idx = s_idx;
        best_s_score = s_p_match_score;
      }
    }

    if (best_s_score == kNoMatch) {
      //      std::cout << "  NO MATCH FOUND: " << p_token << "\n";
      sum += static_cast<float>(p_tokens.size()) * 2.F;
      continue;
    }

    //    std::cout << "  MATCHED: " << p_token << " vs " <<
    //    s_phrases[best_s_idx].s_
    //              << ": " << best_s_score << "\n";
    sum += best_s_score;
    covered |= s_phrases[best_s_idx].token_bits_;
  }

  //  std::cout << "BEFORE NOT MATCHED SCORING: " << sum << "\n";

  auto n_not_matched = 0U;
  for (auto s_idx = 0U; s_idx != s_tokens.size(); ++s_idx) {
    if ((covered & (1U << s_idx)) == 0U) {
      ++n_not_matched;
      auto const not_matched_penalty =
          std::max(1.F, static_cast<float>(s_tokens[s_idx].size()) / 3.0F);
      //      std::cout << "PENALITY NOT MATCHED: " << s_tokens[s_idx] << ": "
      //                << not_matched_penalty << "\n";
      sum += not_matched_penalty;
    }
  }

  if (n_not_matched == s_tokens.size()) {
    return kNoMatch;
  }

  auto const max =
      std::ceil(static_cast<float>(std::min(s.size(), p.size())) / 2.0F);
  auto const score = std::min(fallback, sum);
  //  std::cout << "  SUM: " << sum << ", MAX=" << max << ", SCORE=" << score
  //            << "\n";
  return score >= max ? kNoMatch : score;
}

}  // namespace adr