#include "adr/guess_context.h"

#include "fmt/format.h"

#include "utl/overloaded.h"

#include "adr/area_set.h"
#include "adr/formatter.h"
#include "adr/typeahead.h"

namespace adr {

std::optional<std::string_view> suggestion::get_country_code(
    typeahead const& t) const {
  auto const areas = t.area_sets_[area_set_];
  auto const country_it = utl::find_if(areas, [&](area_idx_t const area) {
    return t.area_country_code_[area] != kNoCountryCode;
  });
  return country_it == end(areas) ? std::nullopt
                                  : std::optional{std::string_view{
                                        t.area_country_code_[*country_it]}};
}

std::string suggestion::format(typeahead const& t,
                               formatter const& f,
                               std::string_view country_code) const {
  auto a = formatter::address{};
  a.country_code_ = country_code;
  std::visit(
      utl::overloaded{
          [&](place_idx_t const p) { a.road_ = t.strings_[str_].view(); },
          [&](address const addr) {
            a.road_ = t.strings_[str_].view();
            if (addr.house_number_ != address::kNoHouseNumber) {
              a.house_number_ =
                  t.strings_[t.house_numbers_[addr.street_][addr.house_number_]]
                      .view();
            }
          }},
      location_);
  return f.format(a);
}

void suggestion::print(std::ostream& out,
                       typeahead const& t,
                       language_list_t const& languages) const {
  std::visit(utl::overloaded{
                 [&](place_idx_t const p) {
                   out << "place=" << t.strings_[str_].view() << " [" << p
                       << (t.place_type_[p] == place_type::kExtra ? " EXT" : "")
                       << "]";
                 },
                 [&](address const addr) {
                   out << "street=" << t.strings_[str_].view() << "["
                       << addr.street_ << ", " << addr.house_number_ << "]";
                   if (addr.house_number_ != address::kNoHouseNumber) {
                     out << ", house_number="
                         << t.strings_[t.house_numbers_[addr.street_]
                                                       [addr.house_number_]]
                                .view();
                   }
                 }},
             location_);
  auto const tz = t.get_tz(area_set_);
  auto const tz_name =
      tz == timezone_idx_t::invalid() ? "" : t.timezone_names_[tz].view();
  out << ", pos=" << coordinates_ << ", areas=" << areas(t, languages)
      << ", tz=" << tz_name << " -> score=" << static_cast<float>(score_)
      << "\n";
}

area_set suggestion::areas(typeahead const& t,
                           language_list_t const& languages) const {
  return area_set{t,         languages,      city_area_idx_,
                  area_set_, matched_areas_, matched_area_lang_};
}

std::string suggestion::description(typeahead const& t) const {
  return std::visit(
      utl::overloaded{
          [&](place_idx_t) {
            return fmt::format("{}, {}", t.strings_[str_].view(),
                               fmt::streamed(areas(t, {kDefaultLang})));
          },
          [&](address const addr) {
            if (addr.house_number_ == address::kNoHouseNumber) {
              return fmt::format("{}, {}", t.strings_[str_].view(),
                                 fmt::streamed(areas(t, {kDefaultLang})));
            } else {
              return fmt::format(
                  "{} {}, {}", t.strings_[str_].view(),
                  t.strings_[t.house_numbers_[addr.street_][addr.house_number_]]
                      .view(),
                  fmt::streamed(areas(t, language_list_t{})));
            }
          }},
      location_);
}

void suggestion::populate_areas(typeahead const& t) {
  auto const areas = t.area_sets_[area_set_];

  // Find zip code area index.
  auto const zip_it = utl::find_if(areas, [&](auto&& a) {
    return t.area_admin_level_[a] == kPostalCodeAdminLevel;
  });
  zip_area_idx_ = zip_it == end(areas)
                      ? std::nullopt
                      : std::optional{std::distance(begin(areas), zip_it)};

  // Find timezone area index.
  tz_ = t.get_tz(area_set_);

  // Find city area index.
  auto const city_it =
      std::min_element(begin(areas), end(areas), [&](auto&& a, auto&& b) {
        constexpr auto const kCloseTo = 8;
        auto const x = to_idx(t.area_admin_level_[a]);
        auto const y = to_idx(t.area_admin_level_[b]);
        return (x > kCloseTo ? 10 : 1) * std::abs(x - kCloseTo) <
               (y > kCloseTo ? 10 : 1) * std::abs(y - kCloseTo);
      });
  city_area_idx_ = city_it == end(areas)
                       ? std::nullopt
                       : std::optional{std::distance(begin(areas), city_it)};
  unique_area_idx_ = city_area_idx_;
}

void guess_context::resize(typeahead const& t) {
  area_phrase_lang_.resize(t.area_names_.size());
  area_phrase_match_scores_.resize(t.area_names_.size());
  area_active_.resize(t.area_names_.size());
}

}  // namespace adr