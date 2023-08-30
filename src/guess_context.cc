#include "adr/guess_context.h"

#include "utl/overloaded.h"

#include "adr/typeahead.h"

namespace adr {

void suggestion::print(std::ostream& out, adr::typeahead const& t) const {
  out << "  ";
  std::visit(utl::overloaded{
                 [&](place_idx_t const p) {
                   out << t.strings_[t.place_names_[p]].view();
                 },
                 [&](address const addr) {
                   out << t.strings_[t.street_names_[addr.street_]].view();
                   if (addr.house_number_ != address::kNoHouseNumber) {
                     out << " "
                         << t.strings_[t.house_numbers_[addr.street_]
                                                       [addr.house_number_]]
                                .view();
                   }
                 },
                 [&](area_idx_t const area) {
                   out << t.strings_[t.area_names_[area]].view();
                 }},
             location_);
  out << ", street_token=" << street_token_ << ", area_token=" << area_token_
      << ", area=" << t.strings_[t.area_names_[area_]].view() << " ("
      << to_str(t.area_admin_level_[area_])
      << ") -> score=" << static_cast<float>(score_) << "\n";
}

}  // namespace adr