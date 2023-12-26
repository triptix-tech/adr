#pragma once

#include <string>
#include <vector>

#include "adr/guess_context.h"

namespace adr {

enum class api { kGPlaces, kMB, kPelias };

std::string request(std::string_view,
                    typeahead const& t,
                    adr::guess_context&,
                    api);

std::string to_json(typeahead const& t, std::vector<suggestion> const&, api);

}  // namespace adr