#include "adr/area_set.h"

#include <ostream>

#include "utl/enumerate.h"

#include "adr/typeahead.h"
#include "utl/overloaded.h"

namespace adr {

std::ostream& operator<<(std::ostream& out, area_set const& s) {
  auto const areas = s.get_areas();
  auto const city_admin_lvl =
      s.city_area_idx_.has_value()
          ? s.t_.area_admin_level_[areas[*s.city_area_idx_]]
          : admin_level_t::invalid();

  if (s.city_area_idx_.has_value()) {
    for (auto const [i, a] : utl::enumerate(areas)) {
      auto const admin_lvl = s.t_.area_admin_level_[a];
      auto const matched = (((1U << i) & s.matched_mask_) != 0U);
      if (city_admin_lvl == admin_lvl && matched) {
        break;
      }
    }
  }

  auto first = true;
  out << " [";
  for (auto const [i, a] : utl::enumerate(areas)) {
    if (!first) {
      out << ", ";
    }
    first = false;

    auto const admin_lvl = s.t_.area_admin_level_[a];
    if (admin_lvl == kTimezoneAdminLevel) {
      continue;
    }

    auto const matched = (((1U << i) & s.matched_mask_) != 0U);

    auto const language_idx =
        matched ? s.matched_area_lang_[i] : s.get_area_lang_idx(a);
    auto const name =
        s.t_.strings_[s.t_.area_names_[a][language_idx < 0
                                              ? kDefaultLangIdx
                                              : static_cast<unsigned>(
                                                    language_idx)]]
            .view();
    if (matched) {
      out << " *";
    }
    out << "(" << name << ", " << static_cast<int>(to_idx(admin_lvl)) << ")";
  }
  out << "]";
  return out;
}

std::int16_t area_set::get_area_lang_idx(area_idx_t const a) const {
  for (auto i = 0U; i != languages_.size(); ++i) {
    auto const j = languages_.size() - i - 1U;
    auto const lang_idx = find_lang(t_.area_name_lang_[a], languages_[j]);
    if (lang_idx != -1) {
      return lang_idx;
    }
  }
  return -1;
}

basic_string<area_idx_t> area_set::get_areas() const {
  return std::visit(
      utl::overloaded{[&](area_set_idx_t x) -> basic_string<area_idx_t> {
                        auto const set = t_.area_sets_[x];
                        return {begin(set), end(set)};
                      },
                      [](basic_string<area_idx_t> const& x) { return x; }},
      areas_);
}

}  // namespace adr