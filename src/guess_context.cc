#include "adr/guess_context.h"

#include "utl/overloaded.h"

#include "adr/typeahead.h"

namespace adr {

void suggestion::print(std::ostream& out, adr::typeahead const& t) const {
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
  out << ", coordinates="
      << osmium::Location{coordinates_.lat_, coordinates_.lng_} << ", areas:\n";

  for (auto const [i, a] : utl::enumerate(areas_)) {
    if (a == area_idx_t::invalid()) {
      out << "  " << t.strings_[t.area_names_[a]].view() << " ("
          << kAdminString[i] << ")\n";
    }
  }
}

}  // namespace adr