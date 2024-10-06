#pragma once

#include <filesystem>

#include "cista/containers/nvec.h"
#include "cista/mmap.h"

#include "osmium/osm/way.hpp"

#include "adr/types.h"

struct rtree;

namespace adr {

struct suggestion;
struct import_context;
struct typeahead;

struct reverse {
  reverse(std::filesystem::path, cista::mmap::protection);
  ~reverse();

  std::vector<suggestion> lookup(typeahead const&,
                                 geo::latlng const&,
                                 std::size_t const n_guesses) const;
  void add_street(import_context&, street_idx_t, osmium::Way const&);
  void write(import_context&);
  void build_rtree(typeahead const&);

private:
  cista::mmap mm(char const*);

  std::filesystem::path p_;
  cista::mmap::protection mode_;
  mm_nvec<street_idx_t, coordinates, 2U> street_segments_;
  rtree* rtree_{nullptr};
};

}  // namespace adr