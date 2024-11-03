#pragma once

#include <filesystem>

#include "cista/containers/nvec.h"
#include "cista/containers/rtree.h"
#include "cista/mmap.h"

#include "osmium/osm/way.hpp"

#include "adr/types.h"

struct rtree;

namespace adr {

struct suggestion;
struct import_context;
struct typeahead;

enum class entity_type : std::uint8_t { kHouseNumber, kStreet, kPlace };

union rtree_entity {
  entity_type type_;

  struct house_number {
    entity_type type_;
    std::uint16_t idx_;
    street_idx_t street_;
  } hn_;

  struct place {
    entity_type type_;
    place_idx_t place_;
  } place_;

  struct street {
    entity_type type_;
    std::uint16_t segment_;
    street_idx_t street_;
  } street_segment_;
};

struct reverse {
  using rtree_t = cista::mm_rtree<rtree_entity>;

  reverse(std::filesystem::path, cista::mmap::protection);

  std::vector<suggestion> lookup(typeahead const&,
                                 geo::latlng const&,
                                 std::size_t const n_guesses) const;
  void add_street(import_context&, street_idx_t, osmium::Way const&);
  void write(import_context&);
  void build_rtree(typeahead const&);
  void write();

private:
  cista::mmap mm(char const*);

  std::filesystem::path p_;
  cista::mmap::protection mode_;
  mm_nvec<street_idx_t, coordinates, 2U> street_segments_;
  cista::mm_rtree<rtree_entity> rtree_;
};

}  // namespace adr