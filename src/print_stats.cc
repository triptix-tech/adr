#include "adr/adr.h"

#include "adr/typeahead.h"

namespace adr {

void print_stats(typeahead const& t) {
  std::cout << "streets: " << t.street_names_.size() << "\n";
  std::cout << "places: " << t.place_names_.size() << "\n";
  std::cout << "area: " << t.area_names_.size() << "\n";
}

}  // namespace adr