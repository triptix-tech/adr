#pragma once

#include <filesystem>
#include <memory>

#include "cista/mmap.h"

#include "osmium/osm/area.hpp"

#include "adr/types.h"

namespace adr {

struct typeahead;

struct area_database final {
  area_database(std::filesystem::path, cista::mmap::protection const mode);
  area_database(area_database&&);
  area_database& operator=(area_database&&);
  ~area_database();

  void lookup(typeahead const&, coordinates, basic_string<area_idx_t>&) const;
  void add_area(area_idx_t, osmium::Area const&);
  bool is_within(coordinates, area_idx_t) const;

  struct impl;
  std::unique_ptr<impl> impl_;
};

}  // namespace adr