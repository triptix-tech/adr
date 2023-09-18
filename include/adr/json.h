#pragma once

#include <string>
#include <vector>

#include "adr/guess_context.h"

namespace adr {

enum class output_format { kGPlaces, kMB, kPelias };

std::string to_json(std::vector<suggestion> const&, output_format);

}  // namespace adr