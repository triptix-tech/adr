#include "adr/guess_context.h"

#include "utl/overloaded.h"

#include "adr/typeahead.h"

namespace adr {

void suggestion::print(std::ostream& out, adr::typeahead const& t) const {
  std::visit(utl::overloaded{
                 [&](place_idx_t const p) {
                   out << "place=" << t.strings_[t.place_names_[p]].view();
                 },
                 [&](address const addr) {
                   out << "street="
                       << t.strings_[t.street_names_[addr.street_]].view();
                   if (addr.house_number_ != address::kNoHouseNumber) {
                     out << ", house_number="
                         << t.strings_[t.house_numbers_[addr.street_]
                                                       [addr.house_number_]]
                                .view();
                   }
                 },
                 [&](area_idx_t const area) {
                   out << "area=" << t.strings_[t.area_names_[area]].view();
                 }},
             location_);
  out << ", pos=" << coordinates_
      << ", areas=" << area_set{t, area_set_, matched_areas_}
      << " -> score=" << static_cast<float>(score_) << "\n";
}

std::string suggestion::areas(typeahead const& t) const { return ""; }

std::string suggestion::description(adr::typeahead const& t) const {
  return std::visit(
      utl::overloaded{
          [&](place_idx_t const p) {
            return fmt::format("{}, {}", t.strings_[t.place_names_[p]].view(),
                               areas(t));
          },
          [&](address const addr) {
            if (addr.house_number_ == address::kNoHouseNumber) {
              return fmt::format(
                  "{}, {}", t.strings_[t.street_names_[addr.street_]].view(),
                  areas(t));
            } else {
              return fmt::format(
                  "{} {}, {}", t.strings_[t.street_names_[addr.street_]].view(),
                  t.strings_[t.house_numbers_[addr.street_][addr.house_number_]]
                      .view(),
                  areas(t));
            }
          }},
      location_);
}

void guess_context::resize(adr::typeahead const& t) {
  area_phrase_match_scores_.resize(t.area_names_.size());
  area_active_.resize(t.area_names_.size());
}

}  // namespace adr