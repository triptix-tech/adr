#pragma once

#include <algorithm>
#include <iostream>
#include <string>

#include "utf8proc.h"

#include "utl/enumerate.h"

namespace adr {

inline void replace_all(std::string& s,
                        std::string const& from,
                        std::string const& to) {
  std::string::size_type pos;
  while ((pos = s.find(from)) != std::string::npos) {
    s.replace(pos, from.size(), to);
  }
}

using utf8_normalize_buf_t = std::basic_string<utf8proc_int32_t>;

inline std::string_view normalize(std::string_view v,
                                  utf8_normalize_buf_t& decomposed) {
  decomposed.resize(v.size());
  auto const decomposed_size = utf8proc_decompose(
      reinterpret_cast<std::uint8_t const*>(v.data()), v.size(),
      decomposed.data(), decomposed.size(),
      static_cast<utf8proc_option_t>(utf8proc_option_t::UTF8PROC_DECOMPOSE |
                                     utf8proc_option_t::UTF8PROC_STRIPMARK |
                                     utf8proc_option_t::UTF8PROC_CASEFOLD));
  if (decomposed_size < 0U) {
    throw std::runtime_error{utf8proc_errmsg(decomposed_size)};
  }

  auto const reencoded_size = utf8proc_reencode(
      decomposed.data(), decomposed_size, static_cast<utf8proc_option_t>(0));
  return {reinterpret_cast<char const*>(decomposed.data()),
          static_cast<std::size_t>(reencoded_size)};
}

inline std::string_view to_lower_case(std::string_view v,
                                      utf8_normalize_buf_t& decomposed) {
  decomposed.resize(v.size());
  auto const decomposed_size = utf8proc_decompose(
      reinterpret_cast<std::uint8_t const*>(v.data()), v.size(),
      decomposed.data(), decomposed.size(),
      static_cast<utf8proc_option_t>(utf8proc_option_t::UTF8PROC_CASEFOLD));
  if (decomposed_size < 0U) {
    throw std::runtime_error{utf8proc_errmsg(decomposed_size)};
  }

  auto const reencoded_size = utf8proc_reencode(
      decomposed.data(), decomposed_size, static_cast<utf8proc_option_t>(0));
  return {reinterpret_cast<char const*>(decomposed.data()),
          static_cast<std::size_t>(reencoded_size)};
}

inline std::string_view normalize(std::string_view in) {
  static auto decomposed_buf = utf8_normalize_buf_t{};
  return normalize(in, decomposed_buf);
}

struct phrase {
  std::uint8_t token_bits_;
  std::string raw_, normalized_;
};

inline std::string bit_mask_to_str(std::uint8_t const b) {
  auto r = std::string{};
  for (auto i = 0U; i != sizeof(b) * 8U; ++i) {
    r += (((b >> i) & 0x1) == 0x1) ? '1' : '0';
  }
  return r;
}

template <typename String>
inline std::vector<phrase> get_phrases(
    std::vector<String> const& in_tokens_normalized,
    std::vector<String> const& in_tokens_raw) {
  auto r = std::vector<phrase>{};
  for (auto from = 0U; from != in_tokens_raw.size(); ++from) {
    for (auto length = 1U;
         from + length <= in_tokens_raw.size() && length != 4U; ++length) {
      auto p = phrase{};
      for (auto to = from; to < from + length && to < in_tokens_raw.size();
           ++to) {
        p.token_bits_ |= 1 << to;
        if (to != from) {
          p.raw_ += ' ';
          p.normalized_ += ' ';
        }
        p.raw_ += in_tokens_raw[to];
        p.normalized_ += in_tokens_normalized[to];
      }
      r.emplace_back(std::move(p));
    }
  }
  std::sort(begin(r), end(r),
            [](auto&& a, auto&& b) { return a.raw_.size() > b.raw_.size(); });
  return r;
}

inline std::uint8_t get_numeric_tokens_mask(
    std::vector<std::string> const& tokens) {
  auto const number_count = [](std::string_view s) {
    return std::count_if(begin(s), end(s),
                         [](auto&& c) { return c >= '0' && c <= '9'; });
  };
  auto mask = std::uint8_t{0U};
  for (auto const [i, token] : utl::enumerate(tokens)) {
    if (number_count(token) >= token.size() / 2.0) {
      mask |= 1U << i;
    }
  }
  return mask;
}

}  // namespace adr