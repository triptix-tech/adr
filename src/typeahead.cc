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

  auto const lock = std::scoped_lock{ctx.mutex_};
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
    import_context& ctx, std::basic_string_view<area_idx_t> p) {
  return utl::get_or_create(ctx.area_set_lookup_, p, [&]() {
    auto const set_idx = area_set_idx_t{area_sets_.size()};
    area_sets_.emplace_back(p);
    return set_idx;
  });
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

template <bool Debug, typename T>
void match_bigrams(typeahead const& t,
                   guess_context const& ctx,
                   std::array<ngram_t, 128U> const in_ngrams_buf,
                   unsigned const n_in_ngrams,
                   data::vector_map<T, string_idx_t> const& names,
                   ngram_index_t<T> const& ngrams,
                   std::vector<cos_sim_match<T>>& matches,
                   cista::raw::vector_map<T, std::uint8_t>& match_counts) {
  matches.clear();

  // Collect candidate indices matched by the bigrams in the input
  // string.
  UTL_START_TIMING(t1);
  utl::fill(match_counts, 0U);
  for (auto i = 0U; i != n_in_ngrams; ++i) {
    for (auto const item_idx : ngrams[in_ngrams_buf[i]]) {
      ++match_counts[item_idx];
    }
  }
  UTL_STOP_TIMING(t1);
  trace("{}: counting matches [{} ms]\n", cista::type_str<T>(),
        UTL_TIMING_MS(t1));

  // Calculate cosine-similarity.
  UTL_START_TIMING(t2);
  auto const min_match_count = 2U + n_in_ngrams / 10U;
  trace("{}: n_in_ngrams={}, min_match_count={}\n", cista::type_str<T>(),
        n_in_ngrams, min_match_count);
  for (auto i = T{0U}; i < match_counts.size(); ++i) {
    if (match_counts[T{i}] < min_match_count) {
      continue;
    }

    if (t.match_sqrts_[names[i]] == 0) {
      continue;
    }

    auto const match_count = match_counts[i];
    auto const m = cos_sim_match<T>{
        i, static_cast<float>(match_count) /
               (ctx.sqrt_len_vec_in_ * t.match_sqrts_[names[i]])};

    if (matches.size() != 6000U || matches.back().cos_sim_ < m.cos_sim_) {
      utl::insert_sorted(matches, m);
      matches.resize(std::min(std::size_t{6000U}, matches.size()));
    }
  }
  UTL_STOP_TIMING(t2);
  trace("{}: {} matches [{} ms]\n", cista::type_str<T>(), matches.size(),
        UTL_TIMING_MS(t2));
}

template <bool Debug>
void typeahead::guess(std::string_view normalized, guess_context& ctx) const {
  ctx.place_matches_.clear();
  ctx.street_matches_.clear();

  if (normalized.size() < 2U) {
    return;
  }

  ctx.sqrt_len_vec_in_ = static_cast<float>(std::sqrt(normalized.size() - 1U));
  auto const [in_ngrams_buf, n_in_ngrams] = split_ngrams(normalized);

  match_bigrams<Debug>(*this, ctx, in_ngrams_buf, n_in_ngrams, place_names_,
                       place_bigrams_, ctx.place_matches_,
                       ctx.place_match_counts_);
  match_bigrams<Debug>(*this, ctx, in_ngrams_buf, n_in_ngrams, street_names_,
                       street_bigrams_, ctx.street_matches_,
                       ctx.street_match_counts_);
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