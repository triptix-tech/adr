#include "adr/adr.h"

#include "adr/typeahead.h"

namespace adr {

std::unique_ptr<typeahead> create() { return std::make_unique<typeahead>(); }

}  // namespace adr