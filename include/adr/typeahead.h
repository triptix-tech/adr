#pragma once

#include "osmium/geom/coordinates.hpp"
#include "osmium/geom/haversine.hpp"
#include "osmium/osm/location.hpp"
#include "osmium/osm/tag.hpp"

#include "ankerl/cista_adapter.h"

#include "cista/containers/string.h"
#include "cista/containers/vecvec.h"

#include "utl/get_or_create.h"
#include "utl/insert_sorted.h"
#include "utl/parser/arg_parser.h"
#include "utl/zip.h"

namespace adr {

namespace data = cista::offset;

using area_set_idx_t = cista::strong<std::uint32_t, struct area_set_idx_>;
using area_idx_t = cista::strong<std::uint32_t, struct area_idx_>;
using admin_level_t = cista::strong<std::uint8_t, struct admin_level_idx_>;
using string_idx_t = cista::strong<std::uint32_t, struct string_idx_>;
using street_idx_t = cista::strong<std::uint32_t, struct street_idx_>;
using place_idx_t = cista::strong<std::uint32_t, struct place_idx_>;

constexpr auto const kPostalCodeAdminLevel = admin_level_t{11};

enum class location_type_t : std::uint8_t {
  kArea,
  kPlace,
  kStreet,
  kHouseNumber
};

struct coordinates {
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
    area_names_.emplace_back(
        get_or_create_string(postal_code, to_idx(idx), location_type_t::kArea));
    utl::verify(verify(), "verify failed");
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
    area_names_.emplace_back(
        get_or_create_string(name, to_idx(idx), location_type_t::kArea));
    utl::verify(verify(), "verify failed");
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

    auto const string_idx = get_or_create_string(street);
    auto const street_idx =
        utl::get_or_create(street_lookup_, string_idx, [&]() {
          auto const idx = street_idx_t{street_names_.size()};
          street_names_.emplace_back(string_idx);
          return idx;
        });

    if (!street_coordinates_[street_idx].empty()) {
      auto const last = street_coordinates_[street_idx].back();
      if (osmium::geom::haversine::distance(
              osmium::geom::Coordinates{l},
              osmium::geom::Coordinates{
                  osmium::Location{last.lat_, last.lng_}}) < 1000.0) {
        return;
      }
    }

    utl::insert_sorted(
        string_to_location_[string_idx],
        data::pair{to_idx(street_idx), location_type_t::kStreet});

    auto const house_number_idx = get_or_create_string(
        house_number, to_idx(street_idx), location_type_t::kHouseNumber);
    house_numbers_[street_idx].push_back(house_number_idx);
    house_coordinates_[street_idx].push_back(coordinates{l.x(), l.y()});
    utl::verify(verify(), "verify failed");
  }

  void add_street(osmium::TagList const& tags, osmium::Location const& l) {
    auto const name = tags["name"];
    if (name == nullptr) {
      return;
    }

    auto const string_idx = get_or_create_string(name);
    auto const street_idx =
        utl::get_or_create(street_lookup_, string_idx, [&]() {
          auto const idx = street_idx_t{street_names_.size()};
          street_names_.emplace_back(string_idx);
          return idx;
        });

    if (!street_coordinates_[street_idx].empty()) {
      auto const last = street_coordinates_[street_idx].back();
      if (osmium::geom::haversine::distance(
              osmium::geom::Coordinates{l},
              osmium::geom::Coordinates{
                  osmium::Location{last.lat_, last.lng_}}) < 100.0) {
        return;
      }
    }

    street_coordinates_[street_idx].emplace_back(coordinates{l.x(), l.y()});
    utl::insert_sorted(
        string_to_location_[string_idx],
        data::pair{to_idx(street_idx), location_type_t::kStreet});
    utl::verify(verify(), "verify failed");
  }

  void add_place(std::uint64_t const id,
                 bool const is_way,
                 osmium::TagList const& tags,
                 osmium::Location const& l) {
    auto const name = tags["name"];
    if (name == nullptr) {
      return;
    }

    auto const idx = place_names_.size();
    place_names_.emplace_back(
        get_or_create_string(name, idx, location_type_t::kPlace));
    place_coordinates_.emplace_back(l.x(), l.y());
    place_osm_ids_.emplace_back(id);
    place_is_way_.resize(place_is_way_.size() + 1U);
    place_is_way_.set(idx, is_way);
    utl::verify(verify(), "verify failed");
  }

  string_idx_t get_or_create_string(std::string_view s,
                                    std::uint32_t const location,
                                    location_type_t const t) {
    auto const string_idx = get_or_create_string(s);
    string_to_location_[string_idx].emplace_back(location, t);
    return string_idx;
  }

  string_idx_t get_or_create_string(std::string_view s) {
    return utl::get_or_create(string_lookup_, s, [&]() {
      strings_.emplace_back(s);
      return string_idx_t{strings_.size() - 1U};
    });
  }

  area_set_idx_t get_or_create_area_set(
      std::basic_string<area_idx_t> const& p) {
    return utl::get_or_create(area_set_lookup_, p, [&]() {
      auto const set_idx = area_set_idx_t{area_sets_.size()};
      area_sets_.emplace_back(p);
      return set_idx;
    });
  }

  bool verify() {
    return true;

    auto i = 0U;
    for (auto const [str, locations] :
         utl::zip(strings_, string_to_location_)) {
      for (auto const& [l, type] : locations) {
        switch (type) {
          case location_type_t::kStreet:
            if (street_names_[street_idx_t{l}] != i) {
              std::cerr << "ERROR: street " << l
                        << ": street_name=" << street_names_[street_idx_t{l}]
                        << " != i=" << i << "\n";
              return false;
            }
            break;
          case location_type_t::kPlace:
            if (place_names_[place_idx_t{l}] != i) {
              std::cerr << "ERROR: place " << l
                        << ": place_name=" << place_names_[place_idx_t{l}]
                        << " != i=" << i << "\n";
              return false;
            }
            break;
          case location_type_t::kHouseNumber: break;
          case location_type_t::kArea:
            if (area_names_[area_idx_t{l}] != i) {
              std::cerr << "ERROR: area " << l
                        << ": area_name=" << area_names_[area_idx_t{l}]
                        << " != i=" << i << "\n";
              return false;
            }
            break;
        }
      }
      ++i;
    }
    return true;
  }

  data::vector_map<area_idx_t, string_idx_t> area_names_;
  data::vector_map<area_idx_t, admin_level_t> area_admin_level_;
  data::vector_map<area_idx_t, std::uint32_t> area_population_;

  data::vector_map<place_idx_t, string_idx_t> place_names_;
  data::vector_map<place_idx_t, coordinates> place_coordinates_;
  data::vector_map<place_idx_t, area_set_idx_t> place_areas_;
  data::vector_map<place_idx_t, std::int64_t> place_osm_ids_;
  data::bitvec place_is_way_;

  data::vector_map<street_idx_t, string_idx_t> street_names_;
  data::mutable_fws_multimap<street_idx_t, coordinates> street_coordinates_;
  data::vecvec<street_idx_t, area_set_idx_t> street_areas_;
  data::mutable_fws_multimap<street_idx_t, string_idx_t> house_numbers_;
  data::mutable_fws_multimap<street_idx_t, coordinates> house_coordinates_;
  data::vecvec<street_idx_t, area_set_idx_t> house_areas_;

  data::vecvec<area_set_idx_t, area_idx_t> area_sets_;

  data::vecvec<string_idx_t, char> strings_;

  data::hash_map<std::basic_string<area_idx_t>, area_set_idx_t>
      area_set_lookup_;
  data::hash_map<data::string, string_idx_t> string_lookup_;
  data::hash_map<string_idx_t, street_idx_t> street_lookup_;

  data::mutable_fws_multimap<string_idx_t,
                             data::pair<std::uint32_t, location_type_t>>
      string_to_location_;
};

}  // namespace adr