#pragma once

#include "adr/normalize.h"
#include "adr/types.h"

namespace adr {

edit_dist_t levenshtein_distance(std::string_view source,
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

score_t get_match_score(
    std::string_view s,  // name from dataset - will be normalized!
    std::string_view p,  // input phrase - won't be normalized!
    std::vector<edit_dist_t>& lev_dist,
    std::string& tmp) {
  if (s.empty() || p.empty()) {
    return kNoMatch;
  }

  auto const normalized_str = std::string_view{normalize(s, tmp)};
  auto const cut_normalized_str =
      normalized_str.substr(0U, std::min(normalized_str.size(), p.size()));
  auto const dist = levenshtein_distance(cut_normalized_str, p, lev_dist);
  if (dist >= cut_normalized_str.size()) {
    //    std::cout << "dist=" << static_cast<int>(dist) << " > "
    //              << cut_normalized_str.size() << "\n";
    return std::numeric_limits<score_t>::max();
  }
  auto const overhang_penality =
      (static_cast<float>(normalized_str.size() -
                          std::min(normalized_str.size(), p.size())) /
       4.0F);
  auto const relative_coverage =
      2.0F * (static_cast<float>(dist) /
              static_cast<float>(cut_normalized_str.size()));

  auto common_prefix_bonus = 0.0F;
  auto const end = std::min(cut_normalized_str.size(), p.size());
  for (auto i = 0U; i != end; ++i) {
    if (cut_normalized_str[i] != p[i]) {
      break;
    }
    common_prefix_bonus -= 0.15F;
  }

  auto const first_letter_mismatch_penality = s[0] != p[0] ? 2.0F : -0.75F;
  auto const second_letter_mismatch_penality = s[1] != p[1] ? 1.0F : -0.5F;

  auto const score = dist + first_letter_mismatch_penality +
                     second_letter_mismatch_penality + overhang_penality +
                     relative_coverage + common_prefix_bonus;
  std::cout << "dist=" << static_cast<int>(dist)
            << ", size=" << cut_normalized_str.size()
            << ", overhang_penality=" << overhang_penality
            << ", relative_coverage=" << relative_coverage
            << ", score=" << score
            << ", max=" << (std::ceil(cut_normalized_str.size() / 2.0F) + 1.0F)
            << "\n";

  return std::floor(score) > std::ceil(cut_normalized_str.size() / 2.0F + 1.0F)
             ? kNoMatch
             : score;
}

}  // namespace adr