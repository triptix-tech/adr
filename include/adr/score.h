#pragma once

#include <bitset>
#include "adr/normalize.h"
#include "adr/types.h"
#include "utl/parser/cstr.h"

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
    std::string_view dataset_token_raw,
    std::string_view dataset_token_normalized,
    std::string_view p_raw,
    std::string_view p_normalized,
    std::vector<sift_offset>& sift4_offset_arr) {
  auto const cut_dataset_token_raw = dataset_token_raw.substr(
      0U, std::min(dataset_token_raw.size(), p_raw.size()));
  auto const cut_dataset_token_normalized = dataset_token_normalized.substr(
      0U, std::min(dataset_token_normalized.size(), p_normalized.size()));

  auto const dist_normalized = sift4(
      cut_dataset_token_normalized, p_normalized, 3,
      std::min(cut_dataset_token_normalized.size(), p_normalized.size()) / 2 +
          2,
      sift4_offset_arr);
  auto const dist_raw =
      dataset_token_raw == dataset_token_normalized && p_raw == p_normalized
          ? dist_normalized
          : sift4(cut_dataset_token_raw, p_raw, 3,
                  std::min(cut_dataset_token_raw.size(), p_raw.size()) / 2 + 2,
                  sift4_offset_arr);

  if (dist_normalized >= cut_dataset_token_normalized.size()) {
    //    std::cout << "dist=" << static_cast<int>(dist) << " > "
    //              << cut_normalized_str.size() << "\n";
    return kNoMatch;
  }
  auto const overhang_penality =
      (static_cast<float>(
           dataset_token_normalized.size() -
           std::min(dataset_token_normalized.size(), p_normalized.size())) /
       3.0F);
  auto const relative_coverage =
      6.0F * (static_cast<float>(dist_normalized) /
              static_cast<float>(cut_dataset_token_normalized.size()));

  auto common_prefix_bonus = 0.0F;
  auto const end =
      std::min(cut_dataset_token_normalized.size(), p_normalized.size());
  for (auto i = 0U; i != end; ++i) {
    if (cut_dataset_token_normalized[i] != p_normalized[i]) {
      break;
    }
    common_prefix_bonus -= 0.25F;
  }

  auto const first_letter_mismatch_penality =
      cut_dataset_token_normalized[0] != p_normalized[0] ? 2.0F : -0.5F;
  auto const second_letter_mismatch_penality =
      cut_dataset_token_normalized[1] != p_normalized[1] ? 1.0F : -0.25F;

  auto const diff_penalty = (dist_raw - dist_normalized) / 2.0F;

  auto score = dist_normalized + first_letter_mismatch_penality +
               second_letter_mismatch_penality + overhang_penality +
               relative_coverage + common_prefix_bonus + diff_penalty;

  //  std::cout << "  " << dataset_token_raw << " vs " << p_raw
  //            << ": raw_dist=" << static_cast<int>(dist_raw)
  //            << ", normalized_dist=" << static_cast<int>(dist_normalized)
  //            << ", raw_size=" << cut_dataset_token_raw.size()
  //            << ", normalize_size=" << cut_dataset_token_normalized.size()
  //            << ", overhang_penality=" << overhang_penality
  //            << ", relative_coverage=" << relative_coverage
  //            << ", first_letter_mismatch_penality="
  //            << first_letter_mismatch_penality
  //            << ", second_letter_mismatch_penality="
  //            << second_letter_mismatch_penality << ", score=" << score
  //            << ", max="
  //            << (std::ceil(cut_dataset_token_normalized.size() / 2.0F)) <<
  //            "\n";

  return score > std::ceil(cut_dataset_token_normalized.size() / 2.0F)
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

inline score_t get_match_score(std::string_view s,
                               phrase const& p,
                               std::vector<sift_offset>& sift4_offset_arr,
                               utf8_normalize_buf_t& normalize_buf,
                               utf8_normalize_buf_t& to_lower_case_buf) {
  if (s.empty() || p.raw_.empty()) {
    return kNoMatch;
  }

  auto const s_normalized = std::string_view{normalize(s, normalize_buf)};
  auto const s_raw = std::string_view{to_lower_case(s, to_lower_case_buf)};

  auto const split_tokens = [](std::string_view x,
                               std::vector<std::string_view>& tokens) {
    for_each_token(
        x,
        [&, c = 0U](auto&& p_token) mutable {
          if (++c > 8) {
            return utl::continue_t::kBreak;
          }
          if (!p_token.empty()) {
            tokens.emplace_back(p_token);
          }
          return utl::continue_t::kContinue;
        },
        ' ', '-');
  };

  std::vector<std::string_view> p_tokens_normalized, s_tokens_normalized;
  split_tokens(s_normalized, s_tokens_normalized);
  split_tokens(p.normalized_, p_tokens_normalized);

  auto const fallback = get_token_match_score(s_raw, s_normalized, p.raw_,
                                              p.normalized_, sift4_offset_arr);
  if (p_tokens_normalized.size() > s_tokens_normalized.size()) {
    return fallback +
           (p_tokens_normalized.size() - s_tokens_normalized.size()) * 2;
  }

  std::vector<std::string_view> s_tokens_raw, p_tokens_raw;
  split_tokens(s_raw, s_tokens_raw);
  split_tokens(p.raw_, p_tokens_raw);

  if (p_tokens_normalized.size() == 1U && s_tokens_normalized.size() == 1U) {
    return fallback;
  }

  auto const s_phrases = get_phrases(s_tokens_normalized, s_tokens_raw);

  //  std::cout << s_raw << " vs " << p.raw_ << "\n";

  auto covered = std::uint8_t{0U};
  auto sum = 0.0F;
  auto no_match = false;
  for (auto p_idx = 0U; p_idx != p_tokens_normalized.size(); ++p_idx) {
    auto best_s_score = kNoMatch;
    auto best_s_idx = 0U;

    for (auto s_idx = 0U; s_idx != s_phrases.size(); ++s_idx) {
      auto const& s_phrase = s_phrases[s_idx];
      if ((covered & s_phrase.token_bits_) != 0U) {
        continue;
      }

      auto s_p_match_score = 0.0F;
      //      if (s_phrase.raw_ == p_tokens_raw[p_idx]) {
      //        s_p_match_score = -2.5F;
      //      } else if (s_phrase.normalized_ == p_tokens_normalized[p_idx]) {
      //        s_p_match_score = -2.0F;
      //      } else {
      s_p_match_score = get_token_match_score(
          s_phrase.raw_, s_phrase.normalized_, p_tokens_raw[p_idx],
          p_tokens_normalized[p_idx], sift4_offset_arr);
      //      }

      if (best_s_score > s_p_match_score) {
        best_s_idx = s_idx;
        best_s_score = s_p_match_score;
      }
    }

    if (best_s_score == kNoMatch) {
      //      std::cout << "  NO MATCH FOUND: " << p_tokens_raw[p_idx] << "\n";
      sum += p_tokens_normalized[p_idx].size() * 2.0;
      continue;
    }

    //    std::cout << "  MATCHED: " << p_tokens_raw[p_idx] << " vs "
    //              << s_phrases[best_s_idx].raw_ << ": " << best_s_score <<
    //              "\n";
    sum += best_s_score;
    covered |= s_phrases[best_s_idx].token_bits_;
  }

  auto n_not_matched = 0U;
  for (auto s_idx = 0U; s_idx != s_tokens_normalized.size(); ++s_idx) {
    if ((covered & (1U << s_idx)) == 0U) {
      ++n_not_matched;
      auto const not_matched_penalty = s_tokens_normalized[s_idx].size() / 4U;
      //      std::cout << "PENALITY NOT MATCHED: " <<
      //      s_tokens_normalized[s_idx]
      //                << ": " << not_matched_penalty << "\n";
      sum += not_matched_penalty;
    }
  }

  if (n_not_matched == s_tokens_normalized.size()) {
    return kNoMatch;
  }

  //  std::cout << "  SUM: " << sum << "\n";

  auto const max =
      std::ceil(std::min(s_normalized.size(), p.normalized_.size()) / 2.0F);
  auto const score = std::min(fallback, sum);
  return score > max ? kNoMatch : score;
}

}  // namespace adr