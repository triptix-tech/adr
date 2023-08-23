#pragma once

#include "osmium/osm/location.hpp"
#include "osmium/osm/tag.hpp"

#include "ankerl/cista_adapter.h"
#include "cista/containers/string.h"
#include "cista/containers/vecvec.h"
#include "utl/get_or_create.h"
#include "utl/parser/arg_parser.h"

namespace adr {

namespace data = cista::offset;

using area_idx_t = cista::strong<std::uint32_t, struct area_idx_>;
using admin_level_t = cista::strong<std::uint8_t, struct admin_level_idx_>;
using string_idx_t = cista::strong<std::uint32_t, struct string_idx_>;
using street_idx_t = cista::strong<std::uint32_t, struct street_idx_>;
using place_idx_t = cista::strong<std::uint32_t, struct place_idx_>;

constexpr auto const kPostalCodeAdminLevel = admin_level_t{11};

struct location {
  std::int32_t lat_, lng_;
};

struct typeahead {
  area_idx_t add_postal_code_area(osmium::TagList const& tags) {
    auto const postal_code = tags["postal_code"];
    if (postal_code == nullptr) {
      return area_idx_t::invalid();
    }

    auto const idx = area_idx_t{area_admin_level_.size()};
    area_admin_level_.emplace_back(kPostalCodeAdminLevel);
    area_population_.emplace_back(0U);
    area_names_.emplace_back(get_or_create_string(postal_code));
    return idx;
  }

  area_idx_t add_admin_area(osmium::TagList const& tags) {
    auto const admin_lvl = tags["admin_level"];
    if (admin_lvl == nullptr) {
      return area_idx_t::invalid();
    }

    auto const name = tags["name"];
    if (name == nullptr) {
      return area_idx_t::invalid();
    }

    auto const population = tags["population"];

    auto const idx = area_idx_t{area_admin_level_.size()};
    area_admin_level_.push_back(admin_level_t{utl::parse<unsigned>(admin_lvl)});
    area_population_.emplace_back(
        population == nullptr ? 0U : utl::parse<unsigned>(population));
    area_names_.emplace_back(get_or_create_string(name));
    return idx;
  }

  void add_address(osmium::TagList const& tags, osmium::Location const& l) {
    auto const house_number = tags["addr:housenumber"];
    if (house_number == nullptr) {
      return;
    }

    auto const street = tags["addr:street"];
    if (street == nullptr) {
      return;
    }

    auto const house_number_idx = get_or_create_string(house_number);
    auto const street_str_idx = get_or_create_string(street);
    auto const street_idx =
        utl::get_or_create(street_lookup_, street_str_idx, [&]() {
          auto const idx = street_idx_t{street_names_.size()};
          street_coordinates_.emplace_back(l.x(), l.y());
          street_names_.emplace_back(street_str_idx);
          return idx;
        });
    preprocessing_house_numbers_[street_idx].push_back(house_number_idx);
    preprocessing_house_coordinates_[street_idx].push_back(
        location{l.x(), l.y()});
  }

  void add_place(osmium::TagList const& tags, osmium::Location const& l) {
    auto const name = tags["name"];
    if (name == nullptr) {
      return;
    }

    place_names_.emplace_back(get_or_create_string(name));
    place_coordinates_.emplace_back(l.x(), l.y());
  }

  string_idx_t get_or_create_string(std::string_view s) {
    return utl::get_or_create(string_lookup_, s, [&]() {
      strings_.emplace_back(s);
      return string_idx_t{strings_.size() - 1U};
    });
  }

  void finalize() {
    for (auto const& x : preprocessing_house_numbers_) {
      house_numbers_.emplace_back(x);
    }
    for (auto const& x : preprocessing_house_coordinates_) {
      house_coordinates_.emplace_back(x);
    }
  }

  data::vector_map<area_idx_t, string_idx_t> area_names_;
  data::vector_map<area_idx_t, admin_level_t> area_admin_level_;
  data::vector_map<area_idx_t, std::uint32_t> area_population_;

  data::vector_map<place_idx_t, string_idx_t> place_names_;
  data::vector_map<place_idx_t, location> place_coordinates_;
  data::vecvec<place_idx_t, area_idx_t> place_areas_;

  data::vector_map<street_idx_t, location> street_coordinates_;
  data::vecvec<street_idx_t, area_idx_t> street_areas_;
  data::vector_map<street_idx_t, string_idx_t> street_names_;
  data::mutable_fws_multimap<street_idx_t, string_idx_t>
      preprocessing_house_numbers_;
  data::mutable_fws_multimap<street_idx_t, location>
      preprocessing_house_coordinates_;
  data::vecvec<street_idx_t, string_idx_t> house_numbers_;
  data::vecvec<street_idx_t, location> house_coordinates_;

  data::vecvec<string_idx_t, char> strings_;

  data::hash_map<data::string, string_idx_t> string_lookup_;
  data::hash_map<string_idx_t, street_idx_t> street_lookup_;

  data::vecvec <
};

}  // namespace adr