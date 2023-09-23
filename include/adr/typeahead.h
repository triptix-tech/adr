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

using lang_map_t = data::hash_map<data::string, language_idx_t>;

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

  std::mutex place_stats_mutex_;
  raw_hash_map<std::string, std::uint32_t> place_stats_;

  raw_mutable_vecvec<string_idx_t,
                     cista::raw::pair<std::uint32_t, location_type_t>>
      string_to_location_;

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

  void build_ngram_index();
  bool verify();

  area_set_idx_t get_or_create_area_set(import_context&,
                                        std::basic_string_view<area_idx_t>);

  template <bool Debug>
  void guess(std::string_view normalized, guess_context&) const;

  template <typename Langs>
  std::int16_t find_lang(Langs const& langs, language_idx_t const l) const {
    if (l == kDefaultLang) {
      return kDefaultLangIdx;
    }
    auto const it = std::find(begin(langs), end(langs), l);
    return it == end(langs) ? -1 : std::distance(begin(langs), it);
  }

  language_idx_t resolve_language(std::string_view s) const {
    auto const it = lang_.find(s);
    return it == end(lang_) ? language_idx_t::invalid() : it->second;
  }

  language_idx_t get_or_create_lang_idx(std::string_view);

  street_idx_t get_or_create_street(import_context&,
                                    std::string_view street_name);

  string_idx_t get_or_create_string(import_context&, std::string_view);

  data::vecvec<area_idx_t, string_idx_t> area_names_;
  data::vecvec<area_idx_t, language_idx_t> area_name_lang_;
  data::vector_map<area_idx_t, admin_level_t> area_admin_level_;
  data::vector_map<area_idx_t, std::uint32_t> area_population_;

  data::vecvec<place_idx_t, string_idx_t> place_names_;
  data::vecvec<place_idx_t, language_idx_t> place_name_lang_;
  data::vector_map<place_idx_t, coordinates> place_coordinates_;
  data::vector_map<place_idx_t, area_set_idx_t> place_areas_;
  data::vector_map<place_idx_t, std::int64_t> place_osm_ids_;
  data::bitvec place_is_way_;

  data::vecvec<street_idx_t, string_idx_t> street_names_;
  data::vecvec<street_idx_t, coordinates> street_pos_;
  data::vecvec<street_idx_t, area_set_idx_t> street_areas_;
  data::vecvec<street_idx_t, string_idx_t> house_numbers_;
  data::vecvec<street_idx_t, coordinates> house_coordinates_;
  data::vecvec<street_idx_t, area_set_idx_t> house_areas_;

  data::vecvec<area_set_idx_t, area_idx_t> area_sets_;

  data::vecvec<string_idx_t, char> strings_;

  data::vector_map<string_idx_t, std::uint8_t> n_bigrams_;

  data::vecvec<ngram_t, string_idx_t, std::uint32_t> bigrams_;

  data::vecvec<string_idx_t, std::uint32_t> string_to_location_;
  data::vecvec<string_idx_t, location_type_t> string_to_type_;

  lang_map_t lang_;
  data::vecvec<language_idx_t, char, std::uint32_t> lang_names_;
};

struct area_set {
  friend std::ostream& operator<<(std::ostream& out, area_set const& s) {
    auto const areas = s.t_.area_sets_[s.areas_];
    auto const city_it =
        std::min_element(begin(areas), end(areas), [&](auto&& a, auto&& b) {
          return std::abs(to_idx(s.t_.area_admin_level_[a]) - 7) <
                 std::abs(to_idx(s.t_.area_admin_level_[b]) - 7);
        });
    auto const city_idx =
        city_it == end(areas) ? -1 : std::distance(begin(areas), city_it);
    auto const city_admin_lvl = city_idx == -1
                                    ? admin_level_t::invalid()
                                    : s.t_.area_admin_level_[areas[city_idx]];

    auto print_city = city_idx != -1;
    if (city_idx != -1) {
      for (auto const& [i, a] : utl::enumerate(s.t_.area_sets_[s.areas_])) {
        auto const admin_lvl = s.t_.area_admin_level_[a];
        auto const matched = (((1U << i) & s.matched_mask_) != 0U);
        if (city_admin_lvl == admin_lvl && matched) {
          print_city = false;
          break;
        }
      }
    }

    auto first = true;
    out << "[";
    for (auto const& [i, a] : utl::enumerate(s.t_.area_sets_[s.areas_])) {
      auto const admin_lvl = s.t_.area_admin_level_[a];
      auto const matched = (((1U << i) & s.matched_mask_) != 0U);
      auto const is_city =
          print_city && s.t_.area_admin_level_[areas[city_idx]] == admin_lvl;
      if (!is_city && !matched) {
        continue;
      }

      if (!first) {
        out << ", ";
      }
      first = false;
      auto const language_idx =
          matched ? s.matched_area_lang_[i] : s.get_area_lang_idx(a);
      auto const name = s.t_.strings_[s.t_.area_names_[a][language_idx]].view();
      if (matched) {
        out << "i=" << static_cast<int>(s.matched_area_lang_[i]) << " *";
      }
      out << "(" << name.substr(std::max(static_cast<int>(name.size()) - 16, 0))
          << ", " << static_cast<int>(to_idx(admin_lvl)) << ")";
    }
    out << "]";
    return out;
  }

  std::int16_t get_area_lang_idx(area_idx_t const a) const {
    for (auto i = 0U; i != languages_.size(); ++i) {
      auto const j = languages_.size() - i - 1U;
      auto const lang_idx = t_.find_lang(t_.area_name_lang_[a], languages_[j]);
      if (lang_idx == -1) {
        return lang_idx;
      }
    }
    return -1;
  }

  typeahead const& t_;
  language_list_t const& languages_;
  area_set_idx_t areas_;
  std::uint32_t matched_mask_{std::numeric_limits<std::uint32_t>::max()};
  area_set_lang_t matched_area_lang_;
};

}  // namespace adr