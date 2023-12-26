#pragma once

#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

#include "geo/latlng.h"

#include "cista/memory_holder.h"

#include "adr/guess_context.h"

namespace adr {

struct typeahead;

void extract(std::filesystem::path const& in,
             std::filesystem::path const& out,
             std::filesystem::path const& tmp_dname);

cista::wrapped<typeahead> read(std::filesystem::path const&, bool mapped);

template <bool Debug>
void get_suggestions(typeahead const&,
                     geo::latlng const&,
                     std::string_view input,
                     unsigned n_suggestions,
                     language_list_t const&,
                     guess_context&);

void print_stats(typeahead const&);

}  // namespace adr