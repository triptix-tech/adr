#pragma once

#include <cassert>
#include <cinttypes>
#include <string_view>

namespace adr {

using ngram_t = std::uint16_t;

constexpr auto const kNgramCharBitWidth = 8U;
constexpr auto const kMaxNgram = 0xFF;
constexpr auto const kNgram0 = kMaxNgram;
constexpr auto const kNgram1 = kNgram0 << kNgramCharBitWidth;
constexpr auto const kNBigrams = std::numeric_limits<ngram_t>::max();

inline bool is_lower_case(char const c) { return c >= 'a' && c <= 'z'; }
inline bool is_upper_case(char const c) { return c >= 'A' && c <= 'Z'; }
inline bool is_number(char const c) { return c >= '0' && c <= '9'; }

inline char decompress_char(ngram_t const trigram) {
  return static_cast<char>(trigram);
}

inline std::string decompress_bigram(ngram_t const t) {
  auto s = std::string{};
  s.resize(2);
  s[0] = decompress_char((t & kNgram0) >> (0U * kNgramCharBitWidth));
  s[1] = decompress_char((t & kNgram1) >> (1U * kNgramCharBitWidth));
  return s;
}

inline ngram_t compress_char(char const c) { return static_cast<ngram_t>(c); }

inline ngram_t compress_bigram(std::string_view s) {
  assert(s.size() >= 2);
  return static_cast<ngram_t>(compress_char(s[0]) << (0U * kNgramCharBitWidth) |
                              compress_char(s[1]) << (1U * kNgramCharBitWidth));
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

template <typename Fn>
void for_each_bigram(std::string_view s, Fn&& fn) {
  if (s.size() < 2) {
    return;
  }
  for (auto i = 0U; i < s.length() - 1U; ++i) {
    auto const bigram = s.substr(i, 2);
    if (bigram[0] != ' ' && bigram[1] != ' ') {
      fn(bigram);
    }
  }
}

inline std::pair<std::array<ngram_t, 128U>, unsigned> split_ngrams(
    std::string_view normalized) {
  auto n_in_ngrams = 0U;
  auto in_bigrams_buf = std::array<ngram_t, 128U>{};
  for_each_bigram(normalized, [&](std::string_view bigram) {
    if (n_in_ngrams >= 128U) {
      return;
    }
    in_bigrams_buf[n_in_ngrams++] = compress_bigram(bigram);
  });
  std::sort(begin(in_bigrams_buf), begin(in_bigrams_buf) + n_in_ngrams);
  return {in_bigrams_buf, n_in_ngrams};
}

}  // namespace adr