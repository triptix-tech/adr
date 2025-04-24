#pragma once

#include "ankerl/cista_adapter.h"

#include "adr/types.h"

namespace adr {

struct import_context {
  template <typename K, typename V>
  using raw_hash_map = cista::raw::ankerl_map<K, V>;

  template <typename K, typename V>
  using raw_vector_map = cista::raw::vector_map<K, V>;

  template <typename K, typename V>
  using raw_mutable_vecvec = cista::raw::mutable_fws_multimap<K, V>;

  raw_hash_map<basic_string<area_idx_t>, area_set_idx_t> area_set_lookup_;
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
  std::vector<cista::raw::vecvec<std::uint32_t, coordinates>> street_segments_;
  std::mutex mutex_;
  std::mutex reverse_mutex_;
};

}  // namespace adr