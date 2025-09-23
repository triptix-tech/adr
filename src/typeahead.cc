#include "adr/typeahead.h"

#include <string_view>

#include "cista/io.h"

#include "utl/erase_duplicates.h"
#include "utl/get_or_create.h"
#include "utl/insert_sorted.h"
#include "utl/parser/arg_parser.h"
#include "utl/timing.h"
#include "utl/zip.h"

#include "osmium/geom/coordinates.hpp"
#include "osmium/geom/haversine.hpp"

#include "adr/adr.h"
#include "adr/guess_context.h"
#include "adr/import_context.h"
#include "adr/trace.h"

using namespace std::string_view_literals;

namespace adr {

language_idx_t typeahead::get_or_create_lang_idx(std::string_view s) {
  return utl::get_or_create(lang_, s, [&]() {
    // +1 skips zero as 0 == default language
    lang_names_.emplace_back(s);
    return language_idx_t{lang_.size() + 1U};
  });
}

template <typename Fn>
void for_each_name(typeahead& t, osmium::TagList const& tags, Fn&& fn) {
  auto const call_fn = [&](char const* name, language_idx_t const l) {
    if (name != nullptr) {
      fn(std::string_view{name}, l);
    }
  };

  call_fn(tags["name"], kDefaultLang);
  call_fn(tags["old_name"], kDefaultLang);
  call_fn(tags["alt_name"], kDefaultLang);
  call_fn(tags["official_name"], kDefaultLang);

  for (auto const& tag : tags) {
    constexpr auto const kNamePrefix = "name:"sv;
    auto const key = std::string_view{tag.key()};
    if (key.starts_with(kNamePrefix)) {
      auto const lang_str = key.substr(kNamePrefix.size());
      call_fn(tag.value(), t.get_or_create_lang_idx(lang_str));
    }
  }
}

area_idx_t typeahead::add_postal_code_area(import_context& ctx,
                                           osmium::TagList const& tags) {
  auto const postal_code = tags["postal_code"];
  if (postal_code == nullptr) {
    return area_idx_t::invalid();
  }

  auto const lock = std::scoped_lock{ctx.mutex_};
  auto const idx = area_idx_t{area_admin_level_.size()};
  area_admin_level_.emplace_back(kPostalCodeAdminLevel);
  area_population_.emplace_back(population{.value_ = 0U});
  area_names_.emplace_back({get_or_create_string(ctx, postal_code)});
  area_name_lang_.emplace_back({kDefaultLang});
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
  if (admin_lvl_int < 2 || admin_lvl_int > 10) {
    return area_idx_t::invalid();
  }

  auto const lock = std::scoped_lock{ctx.mutex_};

  auto names = area_names_.add_back_sized(0U);
  auto langs = area_name_lang_.add_back_sized(0U);
  for_each_name(*this, tags,
                [&](std::string_view name, language_idx_t const lang) {
                  names.push_back(get_or_create_string(ctx, name));
                  langs.push_back(lang);
                });

  auto const p = tags["population"];
  area_population_.emplace_back(population{
      p == nullptr ? std::uint16_t{0U}
                   : static_cast<uint16_t>(utl::parse<unsigned>(p) /
                                           population::kCompressionFactor)});

  auto const c = tags["ISO3166-1"];
  area_country_code_.emplace_back(
      (c == nullptr || std::string_view{c}.size() <= 2U)
          ? kNoCountryCode
          : country_code_t{c[0], c[1]});

  auto const idx = area_idx_t{area_admin_level_.size()};
  area_admin_level_.emplace_back(admin_level_t{admin_lvl_int});

  return idx;
}

street_idx_t typeahead::get_or_create_street(import_context& ctx,
                                             std::string_view street_name) {
  auto const string_idx = get_or_create_string(ctx, street_name);
  return utl::get_or_create(ctx.street_lookup_, string_idx, [&]() {
    auto const idx = street_idx_t{street_names_.size()};
    ctx.string_to_location_[string_idx].emplace_back(to_idx(idx),
                                                     location_type_t::kStreet);
    street_names_.emplace_back({string_idx});
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
  ctx.house_coordinates_[street_idx].emplace_back(
      coordinates::from_location(l));

  assert(ctx.house_numbers_[street_idx].size() ==
         ctx.house_coordinates_[street_idx].size());
}

street_idx_t typeahead::add_street(import_context& ctx,
                                   osmium::TagList const& tags,
                                   osmium::Location const& l) {
  auto const name = tags["name"];
  if (name == nullptr) {
    return street_idx_t::invalid();
  }

  auto const lock = std::scoped_lock{ctx.mutex_};
  auto const street_idx = get_or_create_street(ctx, name);
  for (auto const p : ctx.street_pos_[street_idx]) {
    if (osmium::geom::haversine::distance(
            osmium::geom::Coordinates{l},
            osmium::geom::Coordinates{p.as_location()}) < 1500.0) {
      return street_idx;
    }
  }

  ctx.street_pos_[street_idx].emplace_back(coordinates::from_location(l));

  return street_idx;
}

void typeahead::add_place(import_context& ctx,
                          std::int64_t const id,
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

  auto names = place_names_.add_back_sized(0U);
  auto langs = place_name_lang_.add_back_sized(0U);
  for_each_name(*this, tags,
                [&](std::string_view name, language_idx_t const l) {
                  auto const str_idx = get_or_create_string(ctx, name);
                  ctx.string_to_location_[str_idx].emplace_back(
                      idx, location_type_t::kPlace);
                  names.push_back(str_idx);
                  langs.push_back(l);
                });

  auto const population = tags["population"];
  place_population_.emplace_back(
      population == nullptr
          ? std::uint16_t{0U}
          : static_cast<std::uint16_t>(utl::parse<unsigned>(population) /
                                       population::kCompressionFactor));

  place_coordinates_.emplace_back(coordinates::from_location(l));
  place_osm_ids_.emplace_back(id);
  place_is_way_.resize(place_is_way_.size() + 1U);
  place_is_way_.set(idx, is_way);
  place_type_.emplace_back(place_type::kUnkown);
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
    n_bigrams_[string_idx_t{i}] = std::min(
        static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max()),
        normalized.size() - 1U);
    for_each_bigram(normalized, [&, i = i](std::string_view bigram) {
      tmp[compress_bigram(bigram)].emplace_back(i);
    });
  }

  bigrams_.clear();
  for (auto& x : tmp) {
    utl::erase_duplicates(x);
    bigrams_.emplace_back(x);
  }
}

bool typeahead::verify() {
  auto const has = [](auto&& haystack, auto&& needle) {
    return std::find(begin(haystack), end(haystack), needle) != end(haystack);
  };

  auto i = 0U;
  for (auto const [str, locations, types] :
       utl::zip(strings_, string_to_location_, string_to_type_)) {
    for (auto const& [l, type] : utl::zip(locations, types)) {
      switch (type) {
        case location_type_t::kStreet:
          if (!has(street_names_[street_idx_t{l}], i)) {
            std::cerr << "ERROR: street " << l << ": street_name="
                      << street_names_[street_idx_t{l}][kDefaultLangIdx]
                      << " != i=" << i << "\n";
            return false;
          }
          break;
        case location_type_t::kPlace:
          if (!has(place_names_[place_idx_t{l}], i)) {
            std::cerr << "ERROR: place " << l << ": place_name="
                      << place_names_[place_idx_t{l}][kDefaultLangIdx]
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
  trace("guess: {}", normalized);

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
  trace("counting matches [{} ms]", UTL_TIMING_MS(t1));

  auto min_match_count = 2U + n_in_ngrams / (4U + n_in_ngrams / 10U);

  UTL_START_TIMING(t2);
  std::vector<float> sampling;
  trace("#strings = {}", strings_.size());
  auto const n_strings = strings_.size();
  for (auto i = string_idx_t{0U}; i < n_strings; i += 1) {
    if (string_match_counts[i] < min_match_count) {
      continue;
    }
    auto const match_count = string_match_counts[i];
    auto const cos_sim = static_cast<float>(match_count * match_count) /
                         (n_bigrams_[i] * n_in_ngrams);
    if (cos_sim > 0.01) {
      sampling.emplace_back(cos_sim);
      i += 3072U;
    }
  }
  auto const q_idx = std::ceil(sampling.size() / (n_in_ngrams * 3.0F));
  std::nth_element(begin(sampling), begin(sampling) + q_idx, end(sampling),
                   [](auto&& a, auto&& b) { return a > b; });
  UTL_STOP_TIMING(t2);
  if (sampling.empty()) {
    return;
  }
  auto const cutoff = sampling[q_idx == 0 ? 0 : q_idx - 1];
  trace("cutoff {} [size={}, idx={}] [{} ms]", cutoff, sampling.size(), q_idx,
        UTL_TIMING_MS(t2));
  //  std::cout << "ESTIMATED CUTOFF: " << sampling[q_idx - 1] << "\n";

  // ================
  // COMPUTE COS SIM
  // ----------------
  UTL_START_TIMING(t3);
  for (auto i = string_idx_t{0U}; i < n_strings; ++i) {
    if (string_match_counts[i] < min_match_count) {
      continue;
    }
    auto const match_count = string_match_counts[i];
    auto const cos_sim = static_cast<float>(match_count * match_count) /
                         (n_bigrams_[i] * n_in_ngrams);
    if (cos_sim >= cutoff) {
      matches.emplace_back(cos_sim_match{i, cos_sim});
    }
  }
  std::sort(begin(matches), end(matches));

  if (Debug) {
    trace("n_in_ngrams={}, min_match_count={}\n", n_in_ngrams, min_match_count);
    for (auto i = string_idx_t{0U}; i < n_strings; ++i) {
      if (string_match_counts[i] < min_match_count) {
        continue;
      }

      auto const match_count = string_match_counts[i];
      auto const cos_sim = static_cast<float>(match_count * match_count) /
                           (n_bigrams_[i] * n_bigrams_[i] * n_in_ngrams);

      if (cos_sim > 0.01 && (matches.size() != 11'000 || matches.empty() ||
                             matches.back().cos_sim_ < cos_sim)) {
        utl::insert_sorted(matches, {i, cos_sim});
        matches.resize(std::min(std::size_t{11'000}, matches.size()));
      }
    }
    std::cout << "REAL CUTOFF: " << matches.back().cos_sim_ << "\n";
  }

  UTL_STOP_TIMING(t3);
  trace("{} matches [{} ms]", matches.size(), UTL_TIMING_MS(t3));
}

cista::wrapped<typeahead> read(std::filesystem::path const& p) {
  return cista::read<typeahead>(p);
}

template void typeahead::guess<true>(std::string_view normalized,
                                     guess_context&) const;
template void typeahead::guess<false>(std::string_view normalized,
                                      guess_context&) const;

}  // namespace adr