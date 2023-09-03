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

edit_dist_t levenshtein_distance(std::string_view source,
                                 std::string_view target,
                                 std::vector<edit_dist_t>& lev_dist) {
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
      return kMaxEditDist;
    }
  }

  return lev_dist[min_size];
}

float levenshtein_distance_normalize(std::string_view s,
                                     std::string_view p,
                                     std::vector<edit_dist_t>& lev_dist,
                                     std::string& tmp) {
  auto const normalized_str = std::string_view{normalize(s, tmp)};
  auto const cut_normalized_str =
      normalized_str.substr(0U, std::min(normalized_str.size(), p.size()));
  auto const dist = levenshtein_distance(cut_normalized_str, p, lev_dist);
  if (dist >= cut_normalized_str.size()) {
    return std::numeric_limits<float>::max();
  }
  return dist +
         (static_cast<float>(normalized_str.size() -
                             cut_normalized_str.size()) /
          4.0F) +
         2.0F * (static_cast<float>(dist) /
                 static_cast<float>(cut_normalized_str.size()));
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
  utl::for_each_token(utl::cstr{in}, ' ', [&, i = 0U](utl::cstr token) mutable {
    if (token.empty()) {
      return;
    }
    tokens.emplace_back(normalize_alloc(token.view()));
    all_tokens_mask |= 1U << (i++);
  });
  ctx.phrases_ = get_phrases(tokens);

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
      cista::raw::vector_map<area_idx_t, std::array<float, kMaxInputPhrases>>{};
  auto area_active = std::vector<bool>{};
  area_active.resize(t.area_names_.size());
  area_edit_dist.resize(t.area_names_.size(), inf_edit_dist<float>());
  for (auto const m : ctx.area_matches_) {
    area_edit_dist[m.idx_] = m.edit_dist_;
    area_active[to_idx(m.idx_)] = true;
    trace("  AREA {} [edit_dist={}]\n",
          t.strings_[t.area_names_[m.idx_]].view(),
          std::span{begin(m.edit_dist_), ctx.phrases_.size()});
  }

  constexpr auto const kMaxMatches = std::size_t{1000U};

  auto street_matches =
      std::vector<std::tuple<float, std::uint8_t, street_idx_t>>{};
  for (auto const& street_match : ctx.street_matches_) {
    auto const street = street_match.idx_;
    auto const street_name = t.strings_[t.street_names_[street]].view();
    auto const& normalized_street_name = normalize(street_name, ctx.tmp_);
    for (auto p_idx = 0U; p_idx != ctx.phrases_.size(); ++p_idx) {
      auto const edit_dist = street_match.edit_dist_[p_idx];
      if (edit_dist != std::numeric_limits<std::uint8_t>::max() &&
          (street_matches.size() != kMaxMatches ||
           std::get<0>(street_matches.back()) >= edit_dist)) {
        utl::insert_sorted(
            street_matches, {edit_dist, p_idx, street},
            [](auto&& a, auto&& b) { return std::get<0>(a) < std::get<0>(b); });
        street_matches.resize(std::min(kMaxMatches, street_matches.size()));
      }
    }
  }

  ctx.areas_.clear();
  for (auto const [edit_dist, street_p_idx, street] : street_matches) {
    auto const hn_areas_base = t.street_areas_[street].size();

    std::cout << t.strings_[t.street_names_[street]].view()
              << ", edit_dist=" << edit_dist
              << ", phrase=" << ctx.phrases_[street_p_idx].s_ << "\n";

    for (auto const [i, area_set] : utl::enumerate(t.street_areas_[street])) {
      ctx.areas_[area_set].emplace_back(i, area_src::kStreet);
    }

    auto i = 0U;
    auto best_hn_dist = std::numeric_limits<float>::max();
    auto best_hn = 0U;
    for (auto const [hn, pos, areas_idx] :
         utl::zip(t.house_numbers_[street], t.house_coordinates_[street],
                  t.house_areas_[street])) {
      for (auto const& [hn_p_idx, p] : utl::enumerate(ctx.phrases_)) {
        auto const overlap =
            (p.input_token_bits_ &
             ctx.phrases_[street_p_idx].input_token_bits_) != 0U;
        if (overlap) {
          continue;
        }

        auto const hn_edit_dist = levenshtein_distance_normalize(
            t.strings_[hn].view(), p.s_, ctx.lev_dist_, ctx.tmp_);
        if (hn_edit_dist < best_hn_dist) {
          ctx.areas_[areas_idx].emplace_back(i, area_src::kHouseNumber);
          std::cout << "  HOUSENUMBER MATCH: pos=" << pos
                    << ", name=" << t.strings_[hn].view() << ", phrase=\""
                    << ctx.phrases_[hn_p_idx].s_
                    << "\", edit_dist=" << hn_edit_dist
                    << " , areas: " << area_set{t, areas_idx} << "\n";
        }
      }
    }
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