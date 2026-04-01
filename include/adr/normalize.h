#pragma once

#include <algorithm>
#include <iostream>
#include <string>

#include "utf8proc.h"

#include "utl/concat.h"
#include "utl/enumerate.h"
#include "utl/helpers/algorithm.h"

#include "adr/types.h"

namespace adr {

inline void replace_all(std::string& s,
                        std::string const& from,
                        std::string const& to) {
  std::string::size_type pos;
  while ((pos = s.find(from)) != std::string::npos) {
    s.replace(pos, from.size(), to);
  }
}

using utf8_normalize_buf_t = basic_string<utf8proc_int32_t>;

inline void erase_fillers(std::string& in) {
  std::replace_if(
      begin(in), end(in),
      [](auto c) {
        return c == ',' || c == ';' || c == '-' || c == '/' || c == '(' ||
               c == ')' || c == '.';
      },
      ' ');
  in.erase(std::unique(begin(in), end(in),
                       [](char a, char b) { return a == b && a == ' '; }),
           end(in));

  while (!in.empty() && in.back() == ' ') {
    in.resize(in.size() - 1);
  }
  while (!in.empty() && in.front() == ' ') {
    in.erase(begin(in));
  }
}

inline std::string_view normalize(std::string_view v,
                                  utf8_normalize_buf_t& decomposed) {
  decomposed.resize(v.size());
  auto const decomposed_size = utf8proc_decompose(
      reinterpret_cast<std::uint8_t const*>(v.data()),
      static_cast<utf8proc_ssize_t>(v.size()), decomposed.data(),
      static_cast<utf8proc_ssize_t>(decomposed.size()),
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

inline std::string_view normalize(std::string_view in) {
  static auto decomposed_buf = utf8_normalize_buf_t{};
  return normalize(in, decomposed_buf);
}

using token_bitmask_t = std::uint8_t;

constexpr auto kMaxTokens = sizeof(token_bitmask_t) * 8U;

struct phrase {
  token_bitmask_t token_bits_;
  std::string s_;
};

inline std::string bit_mask_to_str(token_bitmask_t const b) {
  auto r = std::string{};
  for (auto i = 0U; i != sizeof(b) * 8U; ++i) {
    r += (((b >> i) & 0x1) == 0x1) ? '1' : '0';
  }
  return r;
}

inline std::optional<std::pair<std::string_view, std::string_view>>
get_postfix_alt_string(std::string_view s) {
  using m = std::pair<std::string_view, std::string_view>;
  constexpr auto kPostfixMapping = std::array{
      m{"str", "strasse"},
      m{"str.", "strasse"},
      m{"strasse", "str."},
  };

  for (auto [postfix, replacement] : kPostfixMapping) {
    if (s.ends_with(postfix)) {
      return m{postfix, replacement};
    }
  }

  return std::nullopt;
}

inline std::optional<std::string_view> get_exact_alt(std::string_view s) {
  switch (cista::hash(s)) {
    case cista::hash("hbf"): return "hauptbahnhof";
    case cista::hash("hauptbahnhof"): return "hbf";
    case cista::hash("hauptbf"): return "hbf";
    case cista::hash("bahnhof"): return "bhf";
    case cista::hash("bhf"): return "bahnhof";
  }
  return std::nullopt;
}

inline bool append_alt_string(std::string_view s, std::string& out) {
  if (auto const alt = get_exact_alt(s); alt.has_value()) {
    out.append(alt->data(), alt->size());
    return true;
  }

  auto const postfix_alt = get_postfix_alt_string(s);
  if (postfix_alt.has_value()) {
    auto const [postfix, replacement] = *postfix_alt;
    out.append(s.data(), s.size() - postfix.size());
    out.append(replacement.data(), replacement.size());
    return true;
  }

  return false;
}

template <typename String, typename Fn>
inline void for_each_phrase(std::vector<String> const& in_tokens,
                            std::string& mem,
                            Fn&& fn) {
  mem.clear();
  for (auto from = 0U; from != in_tokens.size(); ++from) {
    for (auto len = 1U; from + len <= in_tokens.size() && len != 5U; ++len) {
      auto const to = from + len;
      auto token_bits = token_bitmask_t{0U};
      for (auto i = from; i != to; ++i) {
        token_bits |= static_cast<token_bitmask_t>(1U << i);
      }

      auto const append_until_end = [&](auto&& recurse,
                                        unsigned const token_idx) -> void {
        auto const old_size = mem.size();
        for (auto i = token_idx; i != to; ++i) {
          auto const prefix_size = mem.size();
          auto const token = std::string_view{in_tokens[i]};

          if (append_alt_string(token, mem)) {
            recurse(recurse, i + 1U);
          }

          mem.resize(prefix_size);
          if (!mem.empty()) {
            mem.push_back(' ');
          }
          mem.append(token);
        }

        fn(token_bits, std::string_view{mem});
        mem.resize(old_size);
      };

      append_until_end(append_until_end, from);
      mem.clear();
    }
  }
}

template <typename String>
inline std::vector<phrase> get_sorted_phrases(std::vector<String> const& in_tokens) {
  auto r = std::vector<phrase>{};
  auto mem = std::string{};
  for_each_phrase(
      in_tokens, mem,
      [&](token_bitmask_t const token_bits, std::string_view const s) {
        r.push_back(phrase{token_bits, std::string{s}});
      });
  utl::sort(r, [](auto&& a, auto&& b) { return a.s_.size() > b.s_.size(); });
  r.resize(std::min(static_cast<std::size_t>(kMaxInputPhrases), r.size()));
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
    auto const c = number_count(token);
    if (c != 0U && c >= static_cast<unsigned>((token.size() + 1U) / 2U)) {
      mask |= 1U << i;
    }
  }
  return mask;
}

}  // namespace adr
