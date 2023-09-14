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

  auto const lock = std::scoped_lock{ctx.mutex_};
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

  auto const admin_lvl_int = utl::parse<unsigned>(admin_lvl);
  if (admin_lvl_int < 2 || admin_lvl_int >= 10) {
    return area_idx_t::invalid();
  }

  auto const lock = std::scoped_lock{ctx.mutex_};
  auto const population = tags["population"];
  auto const idx = area_idx_t{area_admin_level_.size()};
  area_admin_level_.emplace_back(admin_level_t{admin_lvl_int});
  area_population_.emplace_back(
      population == nullptr ? 0U : utl::parse<unsigned>(population));
  area_names_.emplace_back(get_or_create_string(ctx, name));
  return idx;
}

street_idx_t typeahead::get_or_create_street(import_context& ctx,
                                             std::string_view street_name) {
  auto const string_idx = get_or_create_string(ctx, street_name);
  return utl::get_or_create(ctx.street_lookup_, string_idx, [&]() {
    auto const idx = street_idx_t{street_names_.size()};
    ctx.string_to_location_[string_idx].emplace_back(to_idx(idx),
                                                     location_type_t::kStreet);
    street_names_.emplace_back(string_idx);
    return idx;
  });
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

  auto const lock = std::scoped_lock{ctx.mutex_};
  auto const street_idx = get_or_create_street(ctx, street);
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

  auto const lock = std::scoped_lock{ctx.mutex_};
  auto const street_idx = get_or_create_street(ctx, name);
  for (auto const p : ctx.street_pos_[street_idx]) {
    if (osmium::geom::haversine::distance(
            osmium::geom::Coordinates{l},
            osmium::geom::Coordinates{osmium::Location{p.lat_, p.lng_}}) <
        1500.0) {
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

  //  {
  //    auto const lock = std::scoped_lock{ctx.place_stats_mutex_};
  //    for (auto const& tag : tags) {
  //      ++ctx.place_stats_[fmt::format("{}__{}", tag.key(), tag.value())];
  //    }
  //  }

  auto const lock = std::scoped_lock{ctx.mutex_};
  auto const idx = place_names_.size();
  auto const string_idx = get_or_create_string(ctx, name);
  place_names_.emplace_back(string_idx);
  place_coordinates_.emplace_back(l.x(), l.y());
  place_osm_ids_.emplace_back(id);
  place_is_way_.resize(place_is_way_.size() + 1U);
  place_is_way_.set(idx, is_way);
  ctx.string_to_location_[string_idx].emplace_back(idx,
                                                   location_type_t::kPlace);
}

string_idx_t typeahead::get_or_create_string(import_context& ctx,
                                             std::string_view s) {
  return utl::get_or_create(ctx.string_lookup_, s, [&]() {
    strings_.emplace_back(s);
    return string_idx_t{strings_.size() - 1U};
  });
}

area_set_idx_t typeahead::get_or_create_area_set(
    import_context& ctx, std::basic_string_view<area_idx_t> p) {
  return utl::get_or_create(ctx.area_set_lookup_, p, [&]() {
    auto const set_idx = area_set_idx_t{area_sets_.size()};
    area_sets_.emplace_back(p);
    return set_idx;
  });
}

void typeahead::build_ngram_index() {
  auto normalize_buf = utf8_normalize_buf_t{};
  auto tmp = std::vector<std::vector<string_idx_t>>{};
  tmp.resize(kNBigrams);
  n_bigrams_.resize(strings_.size());
  for (auto const [i, str_idx] : utl::enumerate(strings_)) {
    auto const normalized = normalize(str_idx.view(), normalize_buf);
    n_bigrams_[string_idx_t{i}] = normalized.size() - 1U;
    for_each_bigram(normalized, [&](std::string_view bigram) {
      tmp[compress_bigram(bigram)].emplace_back(i);
    });
  }

  for (auto& x : tmp) {
    utl::erase_duplicates(x);
    bigrams_.emplace_back(x);
  }
}

bool typeahead::verify() {
  auto i = 0U;
  for (auto const [str, locations, types] :
       utl::zip(strings_, string_to_location_, string_to_type_)) {
    for (auto const& [l, type] : utl::zip(locations, types)) {
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
      }
    }
    ++i;
  }
  return true;
}

template <bool Debug>
void typeahead::guess(std::string_view normalized, guess_context& ctx) const {
  auto& matches = ctx.string_matches_;
  matches.clear();

  // ====================
  // COUNT NGRAM MATCHES
  // --------------------
  if (normalized.size() < 2U) {
    return;
  }

  ctx.sqrt_len_vec_in_ = static_cast<float>(std::sqrt(normalized.size() - 1U));
  auto const [in_ngrams_buf, n_in_ngrams] = split_ngrams(normalized);

  // Collect candidate indices matched by the bigrams in the input
  // string.
  UTL_START_TIMING(t1);
  auto const ngram_set =
      ngram_set_t{begin(in_ngrams_buf), begin(in_ngrams_buf) + n_in_ngrams};
  auto missing = ngram_set_t{};
  auto string_match_counts_ptr = ctx.cache_.get_closest(ngram_set, missing);
  auto& string_match_counts = *string_match_counts_ptr;
  for (auto const& missing_ngram : missing) {
    for (auto const string_idx : bigrams_[missing_ngram]) {
      ++string_match_counts[string_idx];
    }
  }
  ctx.cache_.add(ngram_set, string_match_counts_ptr);
  UTL_STOP_TIMING(t1);
  trace("counting matches [{} ms]\n", UTL_TIMING_MS(t1));

  // ================
  // COMPUTE COS SIM
  // ----------------
  UTL_START_TIMING(t2);
  auto const n_strings = strings_.size();
  auto const min_match_count = 2U + n_in_ngrams / 10U;
  trace("n_in_ngrams={}, min_match_count={}\n", n_in_ngrams, min_match_count);
  for (auto i = string_idx_t{0U}; i < n_strings; ++i) {
    if (string_match_counts[i] < min_match_count) {
      continue;
    }

    auto const match_count = string_match_counts[i];
    auto const cos_sim = static_cast<float>(match_count * match_count) /
                         (n_bigrams_[i] * n_in_ngrams);
    auto const m = cos_sim_match{i, cos_sim};

    if (matches.size() != 6000 || matches.back().cos_sim_ < m.cos_sim_) {
      utl::insert_sorted(matches, m);
      matches.resize(std::min(std::size_t{6000}, matches.size()));
    }
  }
  UTL_STOP_TIMING(t2);
  trace("{} matches [{} ms]\n", matches.size(), UTL_TIMING_MS(t2));
}

cista::wrapped<typeahead> read(std::filesystem::path const& path_in,
                               bool const mapped) {
  constexpr auto const kMode =
      cista::mode::UNCHECKED | cista::mode::WITH_STATIC_VERSION;
  if (mapped) {
    auto b = cista::buf{
        cista::mmap{path_in.string().c_str(), cista::mmap::protection::READ}};
    auto const p = cista::deserialize<typeahead, kMode>(b);
    return cista::wrapped{std::move(b), p};
  } else {
    auto b = cista::file{path_in.c_str(), "r"}.content();
    auto const p = cista::deserialize<typeahead, kMode>(b);
    return cista::wrapped{std::move(b), p};
  }
}

template void typeahead::guess<true>(std::string_view normalized,
                                     guess_context&) const;
template void typeahead::guess<false>(std::string_view normalized,
                                      guess_context&) const;

}  // namespace adr