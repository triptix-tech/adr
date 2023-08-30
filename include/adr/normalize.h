#pragma once

#include <algorithm>
#include <string>

namespace adr {

inline void replace_all(std::string& s,
                        std::string const& from,
                        std::string const& to) {
  std::string::size_type pos;
  while ((pos = s.find(from)) != std::string::npos) {
    s.replace(pos, from.size(), to);
  }
}

inline std::string const& normalize(std::string_view v, std::string& s) {
  s.resize(v.size());
  std::copy(begin(v), end(v), begin(s));

  replace_all(s, "è", "e");
  replace_all(s, "é", "e");
  replace_all(s, "Ä", "a");
  replace_all(s, "ä", "a");
  replace_all(s, "Ö", "o");
  replace_all(s, "ö", "o");
  replace_all(s, "Ü", "u");
  replace_all(s, "ü", "u");
  replace_all(s, "ß", "ss");
  replace_all(s, "-", " ");
  replace_all(s, "/", " ");
  replace_all(s, ".", " ");
  replace_all(s, ",", " ");
  replace_all(s, "(", " ");
  replace_all(s, ")", " ");

  for (int i = 0; i < s.length(); ++i) {
    char c = s[i];
    bool is_lower_case_char = c >= 'a' && c <= 'z';
    bool is_upper_case_char = c >= 'A' && c <= 'Z';
    if (!is_lower_case_char && !is_upper_case_char) {
      s[i] = ' ';
    }
  }

  replace_all(s, "  ", " ");

  std::transform(begin(s), end(s), begin(s), ::tolower);

  return s;
}

inline std::string normalize_alloc(std::string_view in) {
  std::string normalized;
  normalize(in, normalized);
  return normalized;
}

struct phrase {
  std::uint8_t input_token_bits_;
  std::string s_;
};

inline std::string bit_mask_to_str(std::uint8_t const b) {
  auto r = std::string{};
  for (auto i = 0U; i != sizeof(b) * 8U; ++i) {
    r += (((b >> i) & 0x1) == 0x1) ? '1' : '0';
  }
  return r;
}

inline std::vector<phrase> get_phrases(
    std::vector<std::string> const& in_tokens) {
  auto r = std::vector<phrase>{};
  for (auto from = 0U; from != in_tokens.size(); ++from) {
    auto p = phrase{};
    for (auto length = 1U; from + length <= in_tokens.size() && length != 4U;
         ++length) {
      for (auto to = from; to < from + length && to < in_tokens.size(); ++to) {
        p.input_token_bits_ |= 1 << to;
        if (to != from) {
          p.s_ += ' ';
        }
        p.s_ += in_tokens[to];
      }
      r.emplace_back(std::move(p));
    }
  }
  return r;
}

}  // namespace adr