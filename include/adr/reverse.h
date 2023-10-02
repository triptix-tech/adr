#pragma once

#include "cista/containers/nvec.h"

#include "osmium/osm/way.hpp"

#include "adr/types.h"

struct rtree;

namespace adr {

enum class entity_type : std::uint8_t { kHouseNumber, kStreet, kPlace };

union rtree_entity {
  void* to_data() const {
    void* data;
    std::memcpy(&data, this, sizeof(data));
    return data;
  }

  static rtree_entity from_data(void const* ptr) {
    auto e = rtree_entity{};
    std::memcpy(&e, &ptr, sizeof(ptr));
    return e;
  }

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

static_assert(sizeof(rtree_entity) == sizeof(void*));

struct import_context;
struct guess_context;
struct typeahead;

struct reverse {
  void lookup(typeahead const&,
              guess_context&,
              rtree const*,
              geo::latlng const&,
              std::size_t const n_guesses) const;
  void add_street(import_context&, street_idx_t const, osmium::Way const&);
  void write(import_context&);
  rtree* build_rtree(typeahead const&) const;

  cista::offset::nvec<street_idx_t, coordinates, 2U> street_segments_;
};

}  // namespace adr