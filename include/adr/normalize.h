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

inline void normalize(std::string_view v, std::string& s) {
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
}

}  // namespace adr