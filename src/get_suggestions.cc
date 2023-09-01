#include "adr/adr.h"

#include "fmt/ranges.h"

#include "utl/timing.h"

#include "cista/containers/flat_matrix.h"

#include "adr/trace.h"
#include "adr/typeahead.h"
#include "utl/to_vec.h"

namespace adr {

struct area {
  friend bool operator==(area const& a, area const& b) {
    return a.area_ == b.area_;
  }
  area_idx_t area_;
  coordinates coordinates_;
  float cos_sim_{0.0};
};

std::uint8_t levenshtein_distance(std::string_view source,
                                  std::string_view target,
                                  std::vector<std::uint8_t>& lev_dist) {
  if (source.size() > target.size()) {
    return levenshtein_distance(target, source, lev_dist);
  }

  auto const min_size = source.size();
  auto const max_size = target.size();
  lev_dist.resize(min_size + 1);

  for (auto i = 0; i <= min_size; ++i) {
    lev_dist[i] = i;
  }

  for (auto j = 1; j <= max_size; ++j) {
    auto previous_diagonal = lev_dist[0];
    auto previous_diagonal_save = 0U;
    ++lev_dist[0];

    for (auto i = 1; i <= min_size; ++i) {
      previous_diagonal_save = lev_dist[i];
      if (source[i - 1] == target[j - 1]) {
        lev_dist[i] = previous_diagonal;
      } else {
        lev_dist[i] = std::min(std::min(lev_dist[i - 1], lev_dist[i]),
                               previous_diagonal) +
                      1U;
      }

      previous_diagonal = previous_diagonal_save;
    }

    if (lev_dist[j] > source.size() / 2 + 1) {
      return std::numeric_limits<std::uint8_t>::max();
    }
  }

  return lev_dist[min_size];
}

std::uint8_t levenshtein_distance_normalize(std::string_view s,
                                            std::string_view p,
                                            std::vector<std::uint8_t>& lev_dist,
                                            std::string& tmp) {
  auto const normalized_str = std::string_view{normalize(s, tmp)};
  auto const cut_normalized_str =
      normalized_str.substr(0U, std::min(normalized_str.size(), p.size()));
  auto const dist = levenshtein_distance(cut_normalized_str, p, lev_dist);
  if (dist == std::numeric_limits<std::uint8_t>::max()) {
    return dist;
  }
  return dist + ((normalized_str.size() - cut_normalized_str.size()) / 4) +
         static_cast<unsigned>(
             std::ceil(2.0F * (static_cast<float>(dist) /
                               static_cast<float>(cut_normalized_str.size()))));
}

float cos_similarity(std::vector<ngram_t> const& a_grams,
                     std::vector<ngram_t> const& b_grams) {
  struct push_back_count {
    using value_type = ngram_t;
    void push_back(ngram_t) { ++count_; }
    std::uint8_t count_;
  };
  auto counter = push_back_count{};
  std::set_intersection(begin(a_grams), end(a_grams), begin(b_grams),
                        end(b_grams), std::back_inserter(counter));
  return static_cast<float>(counter.count_) /
         static_cast<float>(std::sqrt(a_grams.size() * b_grams.size()));
}

void get_bigrams(std::string_view s, std::vector<ngram_t>& bigrams) {
  for_each_bigram(s, [&](std::string_view bigram) {
    bigrams.emplace_back(compress_bigram(bigram));
  });
  utl::erase_duplicates(bigrams);
}

template <bool Debug>
void get_suggestions(typeahead const& t,
                     geo::latlng const& /* coord */,
                     std::string_view in,
                     unsigned n_suggestions,
                     guess_context& ctx) {
  UTL_START_TIMING(t);

  auto tokens = std::vector<std::string>{};
  auto all_tokens_mask = std::uint8_t{0U};
  auto i = 0U;
  utl::for_each_token(utl::cstr{in}, ' ', [&](utl::cstr token) {
    if (token.empty()) {
      return;
    }
    tokens.emplace_back(normalize_alloc(token.view()));
    all_tokens_mask |= 1U << (i++);
  });
  ctx.phrases_ = get_phrases(tokens);
  for (auto const& [p_idx, p] : utl::enumerate(ctx.phrases_)) {
    get_bigrams(p.s_, ctx.phrase_ngrams_[p_idx]);
  }

  t.guess<Debug>(normalize(in, ctx.tmp_), ctx);

  auto input_hashes = cista::raw::ankerl_set<cista::hash_t>{};
  auto input_vec = std::vector<std::string>{};
  utl::for_each_token(utl::cstr{ctx.tmp_}, ' ', [&](utl::cstr s) {
    input_hashes.emplace(cista::hash(s.view()));
    auto const start = &s.view()[0];
    auto const size = ctx.tmp_.size() - (start - &ctx.tmp_[0]);
    input_vec.emplace_back(start, size);
  });

  auto const get_edit_dist =
      [&]<typename T>(cista::raw::vector_map<T, std::uint8_t>& match_counts,
                      std::vector<match<T>>& matches,
                      data::vector_map<T, string_idx_t> const& names) {
        for (auto& m : matches) {
          for (auto const& [j, p] : utl::enumerate(ctx.phrases_)) {
            m.edit_dist_[j] =
                levenshtein_distance_normalize(t.strings_[names[m.idx_]].view(),
                                               p.s_, ctx.lev_dist_, ctx.tmp_);
          }
        }
      };
  get_edit_dist(ctx.street_match_counts_, ctx.street_matches_, t.street_names_);
  get_edit_dist(ctx.place_match_counts_, ctx.place_matches_, t.place_names_);
  get_edit_dist(ctx.area_match_counts_, ctx.area_matches_, t.area_names_);

  auto area_edit_dist =
      cista::raw::vector_map<area_idx_t,
                             std::array<std::uint8_t, kMaxInputPhrases>>{};
  auto area_active = std::vector<bool>{};
  area_active.resize(t.area_names_.size());
  area_edit_dist.resize(t.area_names_.size(), default_edit_dist());
  for (auto const m : ctx.area_matches_) {
    area_edit_dist[m.idx_] = m.edit_dist_;
    area_active[to_idx(m.idx_)] = true;
    trace("  AREA {} [edit_dist={}]\n",
          t.strings_[t.area_names_[m.idx_]].view(),
          std::span{begin(m.edit_dist_), ctx.phrases_.size()});
  }

  auto street_matches =
      std::vector<std::tuple<float, street_idx_t, std::uint8_t>>{};
  for (auto const& street_match : ctx.street_matches_) {
    auto const street = street_match.idx_;
    auto const street_name = t.strings_[t.street_names_[street]].view();
    auto const& normalized_street_name = normalize(street_name, ctx.tmp_);
    get_bigrams(normalized_street_name, ctx.street_name_ngrams_);
    for (auto p_idx = 0U; p_idx != ctx.phrases_.size(); ++p_idx) {
      street_matches.emplace_back(
          cos_similarity(ctx.street_name_ngrams_, ctx.phrase_ngrams_[p_idx]),
          street, p_idx);
    }
  }

  auto const result_count = static_cast<std::ptrdiff_t>(
      std::min(std::size_t{200}, street_matches.size()));
  std::nth_element(begin(street_matches), begin(street_matches) + result_count,
                   end(street_matches), [](auto&& a, auto&& b) {
                     return std::get<0>(a) > std::get<0>(b);
                   });
  street_matches.resize(result_count);
  std::sort(begin(street_matches), end(street_matches),
            [](auto&& a, auto&& b) { return std::get<0>(a) > std::get<0>(b); });

  auto l = 0U;
  for (auto const [cos_sim, street, p_idx] : street_matches) {
    std::cout << l++ << ": " << t.strings_[t.street_names_[street]].view()
              << ": " << ctx.phrases_[p_idx].s_ << "  [" << cos_sim << "]\n";
  }

  UTL_STOP_TIMING(t);
  trace("{} suggestions [{} ms]\n", ctx.suggestions_.size(), UTL_TIMING_MS(t));

  if (ctx.suggestions_.empty()) {
    return;
  }

  //  UTL_START_TIMING(sort);
  //  auto const result_count = static_cast<std::ptrdiff_t>(
  //      std::min(std::size_t{n_suggestions}, ctx.suggestions_.size()));
  //  std::nth_element(begin(ctx.suggestions_),
  //                   begin(ctx.suggestions_) + result_count,
  //                   end(ctx.suggestions_));
  //  ctx.suggestions_.resize(result_count);
  //  std::sort(begin(ctx.suggestions_), end(ctx.suggestions_));
  //  UTL_STOP_TIMING(sort);
  //  trace("sort [{} us]\n", UTL_TIMING_US(sort));
}

template void get_suggestions<true>(typeahead const&,
                                    geo::latlng const&,
                                    std::string_view,
                                    unsigned,
                                    guess_context&);
template void get_suggestions<false>(typeahead const&,
                                     geo::latlng const&,
                                     std::string_view,
                                     unsigned,
                                     guess_context&);

}  // namespace adr