#include "adr/typeahead.h"

#include "cista/mmap.h"

#include "utl/timing.h"

#include "osmium/geom/coordinates.hpp"
#include "osmium/geom/haversine.hpp"

#include "adr/trace.h"

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
  area_names_.emplace_back(get_or_create_string(ctx, postal_code));
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
  area_names_.emplace_back(get_or_create_string(ctx, name));
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

  auto const house_number_idx = get_or_create_string(ctx, house_number);
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

  for (auto const p : ctx.street_pos_[street_idx]) {
    if (osmium::geom::haversine::distance(
            osmium::geom::Coordinates{l},
            osmium::geom::Coordinates{osmium::Location{p.lat_, p.lng_}}) <
        5000.0) {
      return;
    }
  }

  ctx.street_pos_[street_idx].emplace_back(coordinates{l.x(), l.y()});
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
  place_names_.emplace_back(get_or_create_string(ctx, name));
  place_coordinates_.emplace_back(l.x(), l.y());
  place_osm_ids_.emplace_back(id);
  place_is_way_.resize(place_is_way_.size() + 1U);
  place_is_way_.set(idx, is_way);
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
  auto const sort_by_str_length = [&](std::basic_string<area_idx_t> x) {
    std::sort(begin(x), end(x), [&](auto&& a, auto&& b) {
      return strings_[area_names_[a]].size() > strings_[area_names_[b]].size();
    });
    return x;
  };

  return utl::get_or_create(ctx.area_set_lookup_, p, [&]() {
    auto const set_idx = area_set_idx_t{area_sets_.size()};
    area_sets_.emplace_back(sort_by_str_length(p));
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

  auto const build_bigram_index =
      [&]<typename T>(data::vector_map<T, string_idx_t> const& strings,
                      ngram_index_t<T>& trigram_index) {
        auto tmp = std::vector<std::vector<T>>{};
        tmp.resize(kNBigrams);
        for (auto const [i, str_idx] : utl::enumerate(strings)) {
          normalize(strings_[str_idx].view(), normalized);
          for_each_bigram(normalized, [&](std::string_view bigram) {
            tmp[compress_bigram(bigram)].emplace_back(i);
          });
        }

        for (auto& x : tmp) {
          utl::erase_duplicates(x);
          trigram_index.emplace_back(x);
        }
      };

  build_bigram_index(area_names_, area_bigrams_);
  build_bigram_index(street_names_, street_bigrams_);
  build_bigram_index(place_names_, place_bigrams_);

  match_sqrts_.resize(strings_.size());
  for (auto i = string_idx_t{0U}; i != strings_.size(); ++i) {
    normalize(strings_[i].view(), normalized);
    match_sqrts_[string_idx_t{i}] =
        static_cast<float>(std::sqrt(normalized.size() - 1U));
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

template <bool Debug>
void typeahead::guess(std::string_view normalized, guess_context& ctx) const {
  ctx.reset();
  if (ctx.tmp_.length() < 2U) {
    return;
  }

  auto min_bigram_matches =
      std::max(1U, static_cast<unsigned>(normalized.size() / 5U)) - 1U;

  ctx.sqrt_len_vec_in_ = static_cast<float>(std::sqrt(ctx.tmp_.size() - 1U));
  auto const [in_ngrams_buf, n_in_ngrams] = split_ngrams(ctx.tmp_);

  auto const match_bigrams =
      [&]<typename T>(data::vector_map<T, string_idx_t> const& names,
                      ngram_index_t<T> const& ngrams,
                      std::vector<match<T>>& matches,
                      cista::raw::vector_map<T, std::uint8_t>& match_counts) {
        UTL_START_TIMING(t);

        // Collect candidate indices matched by the bigrams in the input
        // string.
        match_counts.clear();
        match_counts.resize(names.size());
        for (auto i = 0U; i != n_in_ngrams; ++i) {
          for (auto const item_idx : ngrams[in_ngrams_buf[i]]) {
            ++match_counts[item_idx];
          }
        }

        // Calculate cosine-similarity.
        for (auto i = T{0U}; i < match_counts.size(); ++i) {
          if (match_counts[T{i}] < min_bigram_matches) {
            continue;
          }

          auto const match_count = match_counts[i];
          auto const m =
              match<T>{i, static_cast<float>(match_count) /
                              (ctx.sqrt_len_vec_in_ * match_sqrts_[names[i]])};
          matches.emplace_back(m);

          if (matches.size() > 20000) {
            std::nth_element(begin(matches), begin(matches) + 4000,
                             end(matches));
            matches.resize(4000);
          }
        }
        UTL_STOP_TIMING(t);

        trace("{}: {} matches [{}]\n", cista::type_str<T>(), matches.size(),
              UTL_TIMING_MS(t));

        // Sort matches by cosine - similarity.
        UTL_START_TIMING(sort);
        auto const result_count = static_cast<std::ptrdiff_t>(
            std::min(std::size_t{4000}, matches.size()));
        std::nth_element(begin(matches), begin(matches) + result_count,
                         end(matches));
        matches.resize(result_count);
        utl::sort(matches);
        UTL_STOP_TIMING(sort);
        trace("sort [{} us]\n", UTL_TIMING_MS(sort));
      };

  match_bigrams(place_names_, place_bigrams_, ctx.place_matches_,
                ctx.place_match_counts_);
  match_bigrams(area_names_, area_bigrams_, ctx.area_matches_,
                ctx.area_match_counts_);
  match_bigrams(street_names_, street_bigrams_, ctx.street_matches_,
                ctx.street_match_counts_);
}

cista::wrapped<typeahead> read(std::filesystem::path const& path_in) {
  constexpr auto const kMode =
      cista::mode::UNCHECKED | cista::mode::WITH_STATIC_VERSION;
  auto b = cista::buf{
      cista::mmap{path_in.string().c_str(), cista::mmap::protection::READ}};
  auto const p = cista::deserialize<typeahead, kMode>(b);
  return cista::wrapped{std::move(b), p};
}

template void typeahead::guess<true>(std::string_view normalized,
                                     guess_context&) const;
template void typeahead::guess<false>(std::string_view normalized,
                                      guess_context&) const;

}  // namespace adr