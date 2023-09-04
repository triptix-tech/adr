#pragma once

#include "adr/normalize.h"
#include "adr/types.h"

namespace adr {

edit_dist_t levenshtein_distance(std::string_view source,
                                 std::string_view target,
                                 std::vector<edit_dist_t>& lev_dist) {
  if (source.size() > target.size()) {
    std::cout << "SWAP\n";
    std::swap(source, target);
  }

  auto const min_size = source.size();
  auto const max_size = target.size();
  auto const limit = (min_size / 2) + 1;
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
                      1U;
      }

      previous_diagonal = previous_diagonal_save;
    }

    //    std::cout << static_cast<int>(lev_dist[j - 1]) << " vs "
    //              << static_cast<int>(min_size / 2) << "\n";

    //    for (auto i = 0U; i != lev_dist.size(); ++i) {
    //      if (i == j - 1) {
    //        std::cout << '*';
    //      }
    //      std::cout << static_cast<int>(lev_dist[i]) << " ";
    //    }
    //    std::cout << "\n";

    if (lev_dist[std::min(min_size, static_cast<std::size_t>(j - 1))] > limit) {
      return std::numeric_limits<edit_dist_t>::max();
    }
  }

  //  std::cout << "lev_dist[" << min_size
  //            << "] = " << static_cast<int>(lev_dist[min_size]) << "\n";
  return lev_dist[min_size];
}

float levenshtein_distance_normalize(std::string_view s,
                                     std::string_view p,
                                     std::vector<edit_dist_t>& lev_dist,
                                     std::string& tmp) {
  auto const normalized_str = std::string_view{normalize(s, tmp)};
  auto const cut_normalized_str =
      normalized_str.substr(0U, std::min(normalized_str.size(), p.size()));
  auto const dist = levenshtein_distance(cut_normalized_str, p, lev_dist);
  if (dist >= cut_normalized_str.size()) {
    return std::numeric_limits<float>::max();
  }
  auto const ret =
      dist +
      (static_cast<float>(normalized_str.size() - cut_normalized_str.size()) /
       4.0F) +
      2.0F * (static_cast<float>(dist) /
              static_cast<float>(cut_normalized_str.size()));
  return ret > ((cut_normalized_str.size() / 2) + 1)
             ? std::numeric_limits<float>::max()
             : ret;
}

}  // namespace adr