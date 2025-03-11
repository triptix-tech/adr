#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "geo/latlng.h"

#include "cista/memory_holder.h"

#include "adr/guess_context.h"

namespace adr {

struct typeahead;

struct token {
  std::uint16_t start_idx_;
  std::uint16_t size_;
};

void extract(std::filesystem::path const& in,
             std::filesystem::path const& out,
             std::filesystem::path const& tmp_dname);

cista::wrapped<typeahead> read(std::filesystem::path const&);

template <bool Debug>
std::vector<token> get_suggestions(typeahead const&,
                                   geo::latlng const&,
                                   std::string input,
                                   unsigned n_suggestions,
                                   language_list_t const&,
                                   guess_context&,
                                   filter_type filter = filter_type::kNone);

void print_stats(typeahead const&);

}  // namespace adr