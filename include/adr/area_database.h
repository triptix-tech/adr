#pragma once

#include <filesystem>
#include <memory>

#include "osmium/osm/area.hpp"

#include "adr/types.h"

namespace adr {

struct typeahead;

struct area_database final {
  area_database();
  ~area_database();

  void add_area(area_idx_t, osmium::Area const&);
  bool is_within(coordinates, area_idx_t) const;
  void eliminate_duplicates(typeahead const&,
                            coordinates,
                            std::basic_string<area_idx_t>&);

  struct impl;
  std::unique_ptr<impl> impl_;
};

}  // namespace adr