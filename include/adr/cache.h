#pragma once

#include <chrono>
#include <cinttypes>
#include <deque>
#include <iostream>
#include <mutex>
#include <thread>

#include "ankerl/cista_adapter.h"

#include "utl/helpers/algorithm.h"

#include "adr/ngram.h"
#include "adr/types.h"

namespace adr {

using ngram_set_t = std::set<ngram_t>;

inline std::ostream& operator<<(std::ostream& out, ngram_set_t const& set) {
  out << "[";
  auto first = true;
  for (auto const& s : set) {
    if (!first) {
      out << ", ";
    }
    first = false;
    out << decompress_bigram(s);
  }
  return out << "]";
}

inline bool is_subset(ngram_set_t const& subset, ngram_set_t const& superset) {
  return subset.size() <= superset.size() &&
         std::all_of(begin(subset), end(subset), [&](ngram_t const x) {
           return superset.find(x) != end(superset);
         });
}

inline ngram_set_t missing_elements(ngram_set_t const& subset,
                                    ngram_set_t const& superset) {
  auto missing = ngram_set_t{};
  for (auto const& x : superset) {
    if (subset.find(x) == end(subset)) {
      missing.emplace(x);
    }
  }
  return missing;
}

struct cache {
  cache(string_match_count_vector_t::size_type const n_strings,
        std::size_t const max_size)
      : n_strings_{n_strings}, max_size_{max_size} {}

  void add(ngram_set_t const& ref,
           std::shared_ptr<string_match_count_vector_t> v) {
    auto const lock = std::lock_guard{mtx_};
    entries_.emplace(ref, std::move(v));
    insert_order_.push_back(ref);

    while (entries_.size() > max_size_ && !insert_order_.empty()) {
      auto const& front = insert_order_.front();
      auto const it = entries_.find(front);
      insert_order_.pop_front();
      if (it != end(entries_)) {
        entries_.erase(it);
        break;
      }
    }
  }

  std::shared_ptr<string_match_count_vector_t> get_closest(
      ngram_set_t const& ref, ngram_set_t& missing) {
    auto const lock = std::lock_guard{mtx_};

    auto const existing_it = entries_.find(ref);
    if (existing_it != end(entries_)) {
      missing.clear();
      return existing_it->second;
    }

    auto max_size = 0;
    auto max_it = end(entries_);
    for (auto it = begin(entries_); it != end(entries_); ++it) {
      auto const& subset = it->first;
      if (is_subset(subset, ref) && subset.size() > max_size) {
        max_size = subset.size();
        max_it = it;
      }
    }

    if (max_it == end(entries_)) {
      missing = ref;
      return std::make_shared<string_match_count_vector_t>(n_strings_);
    } else {
      missing = missing_elements(max_it->first, ref);
      return std::make_shared<string_match_count_vector_t>(*max_it->second);
    }
  }

  std::mutex mtx_;
  string_match_count_vector_t::size_type n_strings_{0U};
  std::size_t max_size_{0U};
  std::deque<ngram_set_t> insert_order_;
  std::map<ngram_set_t, std::shared_ptr<string_match_count_vector_t>> entries_;
};

}  // namespace adr