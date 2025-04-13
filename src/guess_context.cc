#include "adr/guess_context.h"

#include "fmt/format.h"

#include "utl/overloaded.h"

#include "adr/area_set.h"
#include "adr/typeahead.h"

namespace adr {

void suggestion::print(std::ostream& out,
                       adr::typeahead const& t,
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
  out << ", pos=" << coordinates_
      << ", areas=" << area_set{t,         languages,      city_area_idx_,
                                area_set_, matched_areas_, matched_area_lang_}
      << " -> score=" << static_cast<float>(score_) << "\n";
}

std::string suggestion::areas(typeahead const& t) const { return ""; }

std::string suggestion::description(adr::typeahead const& t) const {
  return std::visit(
      utl::overloaded{
          [&](place_idx_t const p) {
            return fmt::format("{}, {}", t.strings_[str_].view(), areas(t));
          },
          [&](address const addr) {
            if (addr.house_number_ == address::kNoHouseNumber) {
              return fmt::format("{}, {}", t.strings_[str_].view(), areas(t));
            } else {
              return fmt::format(
                  "{} {}, {}", t.strings_[str_].view(),
                  t.strings_[t.house_numbers_[addr.street_][addr.house_number_]]
                      .view(),
                  areas(t));
            }
          }},
      location_);
}

void guess_context::resize(adr::typeahead const& t) {
  area_phrase_lang_.resize(t.area_names_.size());
  area_phrase_match_scores_.resize(t.area_names_.size());
  area_active_.resize(t.area_names_.size());
}

}  // namespace adr