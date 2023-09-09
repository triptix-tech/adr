#pragma once

#include <mutex>
#include <span>
#include <variant>

#include "osmium/osm/location.hpp"
#include "osmium/osm/tag.hpp"

#include "ankerl/cista_adapter.h"

#include "cista/containers/string.h"
#include "cista/containers/vecvec.h"
#include "cista/memory_holder.h"

#include "utl/concat.h"
#include "utl/enumerate.h"
#include "utl/erase_duplicates.h"
#include "utl/get_or_create.h"
#include "utl/helpers/algorithm.h"
#include "utl/insert_sorted.h"
#include "utl/parser/arg_parser.h"
#include "utl/zip.h"

#include "adr/guess_context.h"
#include "adr/ngram.h"
#include "adr/normalize.h"
#include "adr/types.h"

namespace adr {

template <typename T>
using ngram_index_t = data::vecvec<ngram_t, T, std::uint32_t>;

struct import_context {
  template <typename K, typename V>
  using raw_hash_map = cista::raw::ankerl_map<K, V>;

  template <typename K, typename V>
  using raw_vector_map = cista::raw::vector_map<K, V>;

  template <typename K, typename V>
  using raw_mutable_vecvec = cista::raw::mutable_fws_multimap<K, V>;

  raw_hash_map<std::basic_string<area_idx_t>, area_set_idx_t> area_set_lookup_;
  raw_hash_map<std::string, string_idx_t> string_lookup_;
  raw_hash_map<string_idx_t, street_idx_t> street_lookup_;
  raw_vector_map<street_idx_t, string_idx_t> street_names_;

  raw_mutable_vecvec<street_idx_t, coordinates> street_pos_;
  raw_mutable_vecvec<street_idx_t, string_idx_t> house_numbers_;
  raw_mutable_vecvec<street_idx_t, coordinates> house_coordinates_;

  std::mutex mutex_;
};

struct typeahead {
  area_idx_t add_postal_code_area(import_context&, osmium::TagList const&);

  area_idx_t add_admin_area(import_context&, osmium::TagList const&);

  void add_address(import_context&,
                   osmium::TagList const&,
                   osmium::Location const&);

  void add_street(import_context&,
                  osmium::TagList const&,
                  osmium::Location const&);

  void add_place(import_context&,
                 std::uint64_t id,
                 bool is_way,
                 osmium::TagList const&,
                 osmium::Location const&);

  void build_trigram_index();
  bool verify();

  area_set_idx_t get_or_create_area_set(import_context&,
                                        std::basic_string<area_idx_t> const& p);

  template <bool Debug>
  void guess(std::string_view normalized, guess_context&) const;

private:
  string_idx_t get_or_create_string(import_context&, std::string_view s);

public:
  data::vector_map<area_idx_t, string_idx_t> area_names_;
  data::vector_map<area_idx_t, admin_level_t> area_admin_level_;
  data::vector_map<area_idx_t, std::uint32_t> area_population_;

  data::vector_map<place_idx_t, string_idx_t> place_names_;
  data::vector_map<place_idx_t, coordinates> place_coordinates_;
  data::vector_map<place_idx_t, area_set_idx_t> place_areas_;
  data::vector_map<place_idx_t, std::int64_t> place_osm_ids_;
  data::bitvec place_is_way_;

  data::vector_map<street_idx_t, string_idx_t> street_names_;
  data::vecvec<street_idx_t, coordinates> street_pos_;
  data::vecvec<street_idx_t, area_set_idx_t> street_areas_;
  data::vecvec<street_idx_t, string_idx_t> house_numbers_;
  data::vecvec<street_idx_t, coordinates> house_coordinates_;
  data::vecvec<street_idx_t, area_set_idx_t> house_areas_;

  data::vecvec<area_set_idx_t, area_idx_t> area_sets_;

  data::vecvec<string_idx_t, char> strings_;

  data::vector_map<string_idx_t, float> match_sqrts_;

  ngram_index_t<area_idx_t> area_bigrams_;
  ngram_index_t<place_idx_t> place_bigrams_;
  ngram_index_t<street_idx_t> street_bigrams_;
};

struct area_set {
  friend std::ostream& operator<<(std::ostream& out, area_set const& s) {

    auto const areas = s.t_.area_sets_[s.areas_];
    auto const city_it =
        std::min_element(begin(areas), end(areas), [&](auto&& a, auto&& b) {
          return (s.t_.area_admin_level_[a] - 8U) <
                 (s.t_.area_admin_level_[b] - 8U);
        });
    auto const city_idx =
        city_it == end(areas) ? -1 : std::distance(begin(areas), city_it);

    auto first = true;
    out << s.areas_ << " [";
    for (auto const& [i, a] : utl::enumerate(s.t_.area_sets_[s.areas_])) {
      auto const admin_lvl =
          static_cast<unsigned>(to_idx(s.t_.area_admin_level_[a]));
      auto const matched = (((1U << i) & s.matched_mask_) != 0U);
      auto const print_city =
          city_idx != -1 &&
          s.t_.area_admin_level_[areas[city_idx]] == admin_lvl;
      if (!print_city && !matched) {
        continue;
      }

      if (!first) {
        out << ", ";
      }
      first = false;
      auto const name = s.t_.strings_[s.t_.area_names_[a]].view();
      if (matched) {
        out << "*";
      }
      out << "(" << name.substr(std::max(static_cast<int>(name.size()) - 16, 0))
          << ", " << admin_lvl << ")";
    }
    out << "]";
    return out;
  }

  typeahead const& t_;
  area_set_idx_t areas_;
  std::uint32_t matched_mask_{std::numeric_limits<std::uint32_t>::max()};
};

}  // namespace adr