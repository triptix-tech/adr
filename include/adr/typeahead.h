#pragma once

#include <mutex>
#include <span>
#include <variant>

#include "osmium/osm/location.hpp"
#include "osmium/osm/tag.hpp"

#include "ankerl/cista_adapter.h"

#include "cista/containers/string.h"
#include "cista/containers/vecvec.h"

#include "adr/ngram.h"
#include "adr/types.h"

namespace adr {

using lang_map_t = data::hash_map<data::string, language_idx_t>;

struct population {
  static constexpr auto const kCompressionFactor = 200U;
  std::uint32_t get() const { return value_ * kCompressionFactor; }
  std::uint16_t value_;
};

struct import_context;
struct guess_context;

template <typename Langs>
long find_lang(Langs const& langs, language_idx_t const l) {
  if (l == kDefaultLang) {
    return kDefaultLangIdx;
  }
  auto const it = std::find(begin(langs), end(langs), l);
  return it == end(langs) ? -1 : std::distance(begin(langs), it);
}

struct typeahead {
  area_idx_t add_postal_code_area(import_context&, osmium::TagList const&);

  area_idx_t add_admin_area(import_context&, osmium::TagList const&);

  void add_address(import_context&,
                   osmium::TagList const&,
                   osmium::Location const&);

  street_idx_t add_street(import_context&,
                          osmium::TagList const&,
                          osmium::Location const&);

  void add_place(import_context&,
                 std::int64_t id,
                 bool is_way,
                 osmium::TagList const&,
                 osmium::Location const&);

  void build_ngram_index();
  bool verify();

  area_set_idx_t get_or_create_area_set(import_context&,
                                        basic_string_view<area_idx_t>);

  template <bool Debug>
  void guess(std::string_view normalized, guess_context&) const;

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
  data::vector_map<area_idx_t, population> area_population_;

  data::vecvec<place_idx_t, string_idx_t> place_names_;
  data::vecvec<place_idx_t, language_idx_t> place_name_lang_;
  data::vector_map<place_idx_t, coordinates> place_coordinates_;
  data::vector_map<place_idx_t, area_set_idx_t> place_areas_;
  data::vector_map<place_idx_t, std::int64_t> place_osm_ids_;
  data::vector_map<place_idx_t, population> place_population_;
  data::vector_map<place_idx_t, place_type> place_type_;
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

}  // namespace adr