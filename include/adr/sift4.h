#pragma once

#include <cmath>
#include <string_view>
#include <vector>

#include "adr/types.h"

namespace adr {

struct sift_offset {
  std::uint8_t c1_;
  std::uint8_t c2_;
  bool trans_;
};

inline edit_dist_t sift4(std::string_view s1,
                         std::string_view s2,
                         edit_dist_t const maxOffset,
                         edit_dist_t const maxDistance,
                         std::vector<sift_offset>& offset_arr) {
  assert(s1.size() < std::numeric_limits<edit_dist_t>::max());
  assert(s2.size() < std::numeric_limits<edit_dist_t>::max());

  offset_arr.clear();
  if (s1.empty()) {
    if (s2.empty()) {
      return 0U;
    }
    return static_cast<edit_dist_t>(s2.length());
  }

  if (s2.empty()) {
    return static_cast<edit_dist_t>(s1.length());
  }

  auto const l1 = s1.size();
  auto const l2 = s2.size();

  auto c1 = std::uint8_t{0U};  // cursor for string 1
  auto c2 = std::uint8_t{0U};  // cursor for string 2
  auto lcss = 0U;  // largest common subsequence
  auto local_cs = 0U;  // local common substring
  auto trans = 0U;  // number of transpositions ('ab' vs 'ba')
  while ((c1 < l1) && (c2 < l2)) {
    if (s1[c1] == s2[c2]) {
      local_cs++;
      auto isTrans = false;
      // see if current match is a transposition
      auto i = 0U;
      while (i < offset_arr.size()) {
        auto& ofs = offset_arr[i];
        if (c1 <= ofs.c1_ || c2 <= ofs.c2_) {
          // when two matches cross, the one considered a transposition is the
          // one with the largest difference in offsets
          isTrans = std::abs(c2 - c1) >= std::abs(ofs.c2_ - ofs.c1_);
          if (isTrans) {
            trans++;
          } else {
            if (!ofs.trans_) {
              ofs.trans_ = true;
              trans++;
            }
          }
          break;
        } else {
          if (c1 > ofs.c2_ && c2 > ofs.c1_) {
            offset_arr.erase(begin(offset_arr) + i);
          } else {
            i++;
          }
        }
      }
      offset_arr.emplace_back(
          sift_offset{.c1_ = c1, .c2_ = c2, .trans_ = isTrans});
    } else {
      lcss += local_cs;
      local_cs = 0;
      if (c1 != c2) {
        // using min allows the computation of transpositions
        c1 = c2 = std::min(c1, c2);
      }
      if (maxDistance) {
        auto const temporaryDistance = std::max(c1, c2) - lcss + trans;
        if (temporaryDistance > maxDistance) {
          return static_cast<edit_dist_t>(temporaryDistance);
        }
      }
      // if matching characters are found, remove 1 from both cursors (they get
      // incremented at the end of the loop) so that we can have only one code
      // block handling matches
      for (auto i = 0U; i < maxOffset && (c1 + i < l1 || c2 + i < l2); i++) {
        if ((c1 + i < l1) && (s1[c1 + i] == s2[c2])) {
          c1 += i - 1;
          c2--;
          break;
        }
        if ((c2 + i < l2) && (s1[c1] == s2[c2 + i])) {
          c1--;
          c2 += i - 1;
          break;
        }
      }
    }
    c1++;
    c2++;
    // this covers the case where the last match is on the last token in list,
    // so that it can compute transpositions correctly
    if ((c1 >= l1) || (c2 >= l2)) {
      lcss += local_cs;
      local_cs = 0U;
      c1 = c2 = std::min(c1, c2);
    }
  }
  lcss += local_cs;

  // add the cost of transpositions to the final result
  return static_cast<edit_dist_t>(std::max(l1, l2) - lcss + trans);
}

}  // namespace adr