#pragma once

#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

#include "geo/latlng.h"

namespace adr {

struct suggestion {};

struct typeahead;

std::unique_ptr<typeahead> create();

void extract(std::filesystem::path const& in, std::filesystem::path const& out);

void read(std::filesystem::path const&, std::unique_ptr<typeahead>&);

void get_suggestions(typeahead&,
                     geo::latlng const&,
                     std::string_view input,
                     unsigned n_suggestions,
                     std::vector<suggestion>&);

}  // namespace adr