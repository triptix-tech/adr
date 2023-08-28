#include "adr/typeahead.h"

#include "cista/mmap.h"

#include "osmium/geom/coordinates.hpp"
#include "osmium/geom/haversine.hpp"

namespace adr {

area_idx_t typeahead::add_postal_code_area(import_context& ctx,
                                           osmium::TagList const& tags) {
  auto const postal_code = tags["postal_code"];
  if (postal_code == nullptr) {
    return area_idx_t::invalid();
  }

  auto const idx = area_idx_t{area_admin_level_.size()};
  area_admin_level_.emplace_back(kPostalCodeAdminLevel);
  area_population_.emplace_back(0U);
  area_names_.emplace_back(get_or_create_string(ctx, postal_code, to_idx(idx),
                                                location_type_t::kArea));
  return idx;
}

area_idx_t typeahead::add_admin_area(import_context& ctx,
                                     osmium::TagList const& tags) {
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
  area_admin_level_.emplace_back(
      admin_level_t{utl::parse<unsigned>(admin_lvl)});
  area_population_.emplace_back(
      population == nullptr ? 0U : utl::parse<unsigned>(population));
  area_names_.emplace_back(
      get_or_create_string(ctx, name, to_idx(idx), location_type_t::kArea));
  return idx;
}

void typeahead::add_address(import_context& ctx,
                            osmium::TagList const& tags,
                            osmium::Location const& l) {
  auto const house_number = tags["addr:housenumber"];
  if (house_number == nullptr) {
    return;
  }

  auto const street = tags["addr:street"];
  if (street == nullptr) {
    return;
  }

  auto const string_idx = get_or_create_string(ctx, street);
  auto const street_idx =
      utl::get_or_create(ctx.street_lookup_, string_idx, [&]() {
        auto const idx = street_idx_t{street_names_.size()};
        street_names_.emplace_back(string_idx);
        return idx;
      });

  auto too_close = false;
  if (!ctx.street_coordinates_[street_idx].empty()) {
    auto const last = ctx.street_coordinates_[street_idx].back();
    if (osmium::geom::haversine::distance(
            osmium::geom::Coordinates{l},
            osmium::geom::Coordinates{osmium::Location{last.lat_, last.lng_}}) <
        1000.0) {
      too_close = true;
    }
  }

  if (!too_close) {
    utl::insert_sorted(
        ctx.string_to_location_[string_idx],
        data::pair{to_idx(street_idx), location_type_t::kStreet});
  }

  auto const house_number_idx = get_or_create_string(
      ctx, house_number, to_idx(street_idx), location_type_t::kHouseNumber);
  ctx.house_numbers_[street_idx].emplace_back(house_number_idx);
  ctx.house_coordinates_[street_idx].emplace_back(coordinates{l.x(), l.y()});
}

void typeahead::add_street(import_context& ctx,
                           osmium::TagList const& tags,
                           osmium::Location const& l) {
  auto const name = tags["name"];
  if (name == nullptr) {
    return;
  }

  auto const string_idx = get_or_create_string(ctx, name);
  auto const street_idx =
      utl::get_or_create(ctx.street_lookup_, string_idx, [&]() {
        auto const idx = street_idx_t{street_names_.size()};
        street_names_.emplace_back(string_idx);
        return idx;
      });

  if (!ctx.street_coordinates_[street_idx].empty()) {
    auto const last = ctx.street_coordinates_[street_idx].back();
    if (osmium::geom::haversine::distance(
            osmium::geom::Coordinates{l},
            osmium::geom::Coordinates{osmium::Location{last.lat_, last.lng_}}) <
        100.0) {
      return;
    }
  }

  ctx.street_coordinates_[street_idx].emplace_back(coordinates{l.x(), l.y()});
  utl::insert_sorted(ctx.string_to_location_[string_idx],
                     data::pair{to_idx(street_idx), location_type_t::kStreet});
}

void typeahead::add_place(import_context& ctx,
                          std::uint64_t const id,
                          bool const is_way,
                          osmium::TagList const& tags,
                          osmium::Location const& l) {
  auto const name = tags["name"];
  if (name == nullptr) {
    return;
  }

  auto const idx = place_names_.size();
  place_names_.emplace_back(
      get_or_create_string(ctx, name, idx, location_type_t::kPlace));
  place_coordinates_.emplace_back(l.x(), l.y());
  place_osm_ids_.emplace_back(id);
  place_is_way_.resize(place_is_way_.size() + 1U);
  place_is_way_.set(idx, is_way);
}

string_idx_t typeahead::get_or_create_string(import_context& ctx,
                                             std::string_view s,
                                             std::uint32_t const location,
                                             location_type_t const t) {
  auto const string_idx = get_or_create_string(ctx, s);
  ctx.string_to_location_[string_idx].emplace_back(location, t);
  return string_idx;
}

string_idx_t typeahead::get_or_create_string(import_context& ctx,
                                             std::string_view s) {
  return utl::get_or_create(ctx.string_lookup_, s, [&]() {
    strings_.emplace_back(s);
    return string_idx_t{strings_.size() - 1U};
  });
}

area_set_idx_t typeahead::get_or_create_area_set(
    import_context& ctx, std::basic_string<area_idx_t> const& p) {
  return utl::get_or_create(ctx.area_set_lookup_, p, [&]() {
    auto const set_idx = area_set_idx_t{area_sets_.size()};
    area_sets_.emplace_back(p);
    return set_idx;
  });
}

bool typeahead::verify() {
  auto i = 0U;
  for (auto const [str, locations] : utl::zip(strings_, string_to_location_)) {
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

void typeahead::build_trigram_index() {
  auto normalized = std::string{};

  auto const build_index = [&]<typename T>(
                               data::vector_map<T, string_idx_t> const& strings,
                               trigram_idx_t<T>& trigram_index) {
    auto tmp = std::vector<std::vector<T>>{};
    tmp.resize(kNTrigrams);
    for (auto const [i, str_idx] : utl::enumerate(strings)) {
      normalize(strings_[str_idx].view(), normalized);
      for_each_trigram(normalized, [&](std::string_view trigram) {
        tmp[compress_trigram(trigram)].emplace_back(i);
      });
    }

    for (auto& x : tmp) {
      utl::erase_duplicates(x);
      trigram_index.emplace_back(x);
    }
  };

  build_index(area_names_, area_trigrams_);
  build_index(street_names_, street_trigrams_);
  build_index(place_names_, place_trigrams_);

  match_sqrts_.resize(strings_.size());
  for (auto i = string_idx_t{0U}; i != strings_.size(); ++i) {
    match_sqrts_[string_idx_t{i}] =
        static_cast<float>(std::sqrt(normalized.size() - 2U));
    match_sqrts_.resize(strings_.size());
  }
}

std::uint64_t fast_log_2(std::uint64_t v) {
  constexpr std::uint8_t t[64] = {
      63, 0,  58, 1,  59, 47, 53, 2,  60, 39, 48, 27, 54, 33, 42, 3,
      61, 51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4,
      62, 57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21,
      56, 45, 25, 31, 35, 16, 9,  12, 44, 24, 15, 8,  23, 7,  6,  5};

  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  return t[(static_cast<std::uint64_t>((v - (v >> 1U)) * 0x07EDD5E59A4E28C2)) >>
           58U];
}

void typeahead::guess(std::string_view in, guess_context& ctx) const {
  ctx.reset();
  normalize(in, ctx.normalized_);
  if (ctx.normalized_.length() < 3) {
    return;
  }

  auto n_in_trigrams = std::uint16_t{0U};
  auto in_trigrams_buf = std::array<compressed_trigram_t, 128U>{};
  for_each_trigram(ctx.normalized_, [&](std::string_view trigram) {
    if (n_in_trigrams >= 128U) {
      return;
    }
    in_trigrams_buf[n_in_trigrams++] = compress_trigram(trigram);
  });
  std::sort(begin(in_trigrams_buf), begin(in_trigrams_buf) + n_in_trigrams);

  auto const match_trigrams =
      [&]<typename T>(
          data::vector_map<T, string_idx_t> const& names,
          data::vecvec<compressed_trigram_t, T, std::uint32_t> const& trigrams,
          cista::raw::vector_map<T, match<T>>& matches,
          cista::raw::vector_map<T, std::uint8_t>& match_counts) {
        // Collect candidate indices matched by the trigrams in the input
        // string.
        match_counts.clear();
        match_counts.resize(strings_.size());
        for (auto i = 0U; i != n_in_trigrams; ++i) {
          auto const t = in_trigrams_buf[i];
          auto const idf = static_cast<float>(fast_log_2(strings_.size())) /
                           static_cast<float>(fast_log_2(trigrams[t].size()));
          std::cout << decompress_trigram(t) << ": "
                    << "strings_.size=" << strings_.size()
                    << ", trigram_index[t].size=" << trigrams[t].size()
                    << ", nominator=" << fast_log_2(strings_.size())
                    << ", denominator=" << fast_log_2(1 + trigrams[t].size())
                    << ": " << idf << "\n";
          for (auto const item_idx : trigrams[t]) {
            match_counts[item_idx] += std::ceil(idf);
          }
        }

        // Calculate cosine-similarity.
        ctx.sqrt_len_vec_in_ =
            static_cast<float>(std::sqrt(ctx.normalized_.size() - 2));
        for (auto i = T{0U}; i < match_counts.size(); ++i) {
          if (match_counts[T{i}] == 0U) {
            continue;
          }

          auto const match_count = match_counts[i];
          matches.emplace_back(
              i, static_cast<float>(match_count) /
                     (ctx.sqrt_len_vec_in_ * match_sqrts_[names[i]]));
        }

        // Sort matches by cosine-similarity.
        auto const result_count =
            static_cast<std::ptrdiff_t>(std::min(200U, matches.size()));
        std::nth_element(begin(matches), begin(matches) + result_count,
                         end(matches));
        matches.resize(result_count);
        utl::sort(matches);
      };

  match_trigrams(place_names_, place_trigrams_, ctx.place_matches_,
                 ctx.place_match_counts_);
  match_trigrams(area_names_, area_trigrams_, ctx.area_matches_,
                 ctx.area_match_counts_);
  match_trigrams(street_names_, street_trigrams_, ctx.street_matches_,
                 ctx.street_match_counts_);
}

void typeahead::print_match(string_idx_t const str_idx) const {
  using namespace std::string_view_literals;

  auto const print_area = [&](area_idx_t const area) {
    auto const admin_lvl = area_admin_level_[area];
    std::cout << "          name=" << strings_[area_names_[area]].view()
              << ", admin_lvl="
              << (admin_lvl >= kAdminString.size()
                      ? fmt::to_string(to_idx(admin_lvl))
                      : kAdminString[to_idx(admin_lvl)])
              << "\n";
  };

  auto const print_coordinate_and_areas = [&](coordinates const& c,
                                              area_set_idx_t const area_set) {
    std::cout << "        " << osmium::Location{c.lat_, c.lng_} << "\n";

    for (auto const& area : area_sets_[area_set]) {
      print_area(area);
    }
  };

  auto const print_street = [&](street_idx_t const street_idx) {
    auto const name_idx = street_names_[street_idx];
    auto const area_sets = street_areas_[street_idx];
    auto const coordinates = street_coordinates_[street_idx];
    auto const house_numbers = house_numbers_[street_idx];
    auto const house_coordinates = house_coordinates_[street_idx];
    auto const house_areas = house_areas_[street_idx];

    std::cout << "    " << strings_[name_idx].view() << "\n";

    std::cout << "      HOUSE NUMBERS:\n";
    for (auto const [house_number, house_coordinates, area_set] :
         utl::zip(house_numbers, house_coordinates, house_areas)) {
      std::cout << "        " << strings_[house_number].view() << "\n";
      print_coordinate_and_areas(house_coordinates, area_set);
    }

    std::cout << "      COORDINATES [n_coordinates=" << coordinates.size()
              << ", n_areas=" << area_sets.size() << "]:\n";
    for (auto const& [c, area_set] : utl::zip(coordinates, area_sets)) {
      print_coordinate_and_areas(c, area_set);
    }
  };

  auto const locations = string_to_location_[str_idx];
  auto const str = strings_[str_idx];
  std::cout << "STR=" << str.view() << ": " << locations.size() << "\n";

  for (auto const [l, type] : locations) {
    switch (type) {
      case location_type_t::kStreet:
        std::cout << "  TYPE=STREET\n";
        print_street(street_idx_t{l});
        break;

      case location_type_t::kPlace: {
        std::cout << "  TYPE=PLACE " << l << " [way=" << place_is_way_.test(l)
                  << ", osm_id=" << place_osm_ids_[place_idx_t{l}]
                  << "] name=\""
                  << strings_[place_names_[place_idx_t{l}]].view() << "\"\n";
        print_coordinate_and_areas(place_coordinates_[place_idx_t{l}],
                                   place_areas_[place_idx_t{l}]);
        break;
      }

      case location_type_t::kHouseNumber:
        std::cout << "  TYPE=HOUSENUMBER\n";
        break;

      case location_type_t::kArea:
        std::cout << "  TYPE=AREA\n";
        print_area(area_idx_t{l});
        break;
    }
  }
}

cista::wrapped<typeahead> read(std::filesystem::path const& path_in) {
  constexpr auto const kMode =
      cista::mode::UNCHECKED | cista::mode::WITH_STATIC_VERSION;
  auto b = cista::buf{
      cista::mmap{path_in.string().c_str(), cista::mmap::protection::READ}};
  auto const p = cista::deserialize<typeahead, kMode>(b);
  return cista::wrapped{std::move(b), p};
}

}  // namespace adr