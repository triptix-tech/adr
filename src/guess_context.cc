#include "adr/guess_context.h"

#include "utl/overloaded.h"

#include "adr/typeahead.h"

namespace adr {

void suggestion::print(std::ostream& out,
                       adr::typeahead const& t,
                       std::vector<phrase> const& phrases) const {
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

void guess_context::resize(const adr::typeahead& t) {
  auto const n_places = t.place_names_.size();
  auto const n_areas = t.area_names_.size();
  auto const n_streets = t.street_names_.size();

  place_match_counts_.resize(n_places);
  area_match_counts_.resize(n_areas);
  street_match_counts_.resize(n_streets);

  place_phrase_match_scores_.resize(n_places);
  street_phrase_match_scores_.resize(n_streets);
  area_phrase_match_scores_.resize(n_areas);

  area_active_.resize(n_areas);
}

}  // namespace adr