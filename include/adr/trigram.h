#pragma once

#include <cassert>
#include <cinttypes>
#include <string_view>

namespace adr {

using compressed_trigram_t = std::uint16_t;

constexpr auto const kTrigramCharBitWidth = 5U;
constexpr auto const kNumbersStartAt = 'z' - 'a' + 1U;
constexpr auto const kNTrigrams =
    (std::numeric_limits<compressed_trigram_t>::max() >> 1U) + 1U;

inline bool is_lower_case(char const c) { return c >= 'a' && c <= 'z'; }
inline bool is_upper_case(char const c) { return c >= 'A' && c <= 'Z'; }
inline bool is_number(char const c) { return c >= '0' && c <= '9'; }

inline char decompress_char(compressed_trigram_t const trigram) {
  if (trigram <= ('z' - 'a')) {
    return 'a' + static_cast<char>(trigram);
  } else {
    return '0' + (static_cast<char>(trigram) - kNumbersStartAt);
  }
}

inline std::string decompress_trigram(compressed_trigram_t const t) {
  auto s = std::string{};
  s.resize(3);
  s[0] = decompress_char((t & 0x001F) >> (0U * kTrigramCharBitWidth));
  s[1] = decompress_char((t & 0x03E0) >> (1U * kTrigramCharBitWidth));
  s[2] = decompress_char((t & 0x7C00) >> (2U * kTrigramCharBitWidth));
  return s;
}

inline compressed_trigram_t compress_char(char const c) {
  if (is_upper_case(c)) {
    return c - 'A';
  } else if (is_lower_case(c)) {
    return c - 'a';
  } else if (is_number(c)) {
    return kNumbersStartAt + (c - '0') % 6;
  } else {
    return c & 0x1F;
  }
}

inline compressed_trigram_t compress_trigram(std::string_view s) {
  assert(s.size() >= 3);
  return compress_char(s[0]) << (0U * kTrigramCharBitWidth) |
         compress_char(s[1]) << (1U * kTrigramCharBitWidth) |
         compress_char(s[2]) << (2U * kTrigramCharBitWidth);
}

template <typename Fn>
void for_each_trigram(std::string_view s, Fn&& fn) {
  if (s.size() < 3) {
    return;
  }
  for (auto i = 0U; i < s.length() - 2U; ++i) {
    fn(s.substr(i, 3));
  }
}

}  // namespace adr