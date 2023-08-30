#include "adr/adr.h"

#include "cista/containers/flat_matrix.h"

#include "adr/typeahead.h"

namespace adr {

struct area {
  friend bool operator==(area const& a, area const& b) {
    return a.area_ == b.area_;
  }
  area_idx_t area_;
  coordinates coordinates_;
  float cos_sim_{0.0};
};

std::uint8_t soundex(char const c) {
  switch (c) {
    case 'b':
    case 'f':
    case 'p':
    case 'v': return 1;
    case 'c':
    case 'g':
    case 'j':
    case 'k':
    case 'q':
    case 's':
    case 'x':
    case 'z': return 2;
    case 'd':
    case 't': return 3;
    case 'l': return 4;
    case 'm':
    case 'n': return 5;
    case 'r': return 6;
    default: return c;
  }
}

std::uint8_t soundex_diff(char const a, char const b) {
  return soundex(a) == soundex(b) ? 1U : 1U;
}

template <typename T1, typename T2>
unsigned levenshtein_distance(T1 const& source,
                              T2 const& target,
                              std::vector<unsigned>& lev_dist) {
  using size_type = unsigned;

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
                      soundex_diff(source[i - 1], target[j - 1]);
      }
      previous_diagonal = previous_diagonal_save;
    }
  }

  return lev_dist[min_size];
}

void get_suggestions(typeahead const& t,
                     geo::latlng const& /* coord */,
                     std::string_view in,
                     unsigned n_suggestions,
                     guess_context& ctx) {
  auto tokens = std::vector<std::string>{};
  auto all_tokens_mask = std::uint8_t{0U};
  auto i = 0U;
  utl::for_each_token(utl::cstr{in}, ' ', [&](utl::cstr token) {
    tokens.emplace_back(normalize_alloc(token.view()));
    all_tokens_mask |= 1U << (i++);
  });
  //  std::cout << "all tokens mask: " << bit_mask_to_str(all_tokens_mask) <<
  //  "\n";
  auto const in_phrases = get_phrases(tokens);
  //  for (auto const [i, p] : utl::enumerate(in_phrases)) {
  //    std::cout << "  " << i << ": " << p.s_ << "\n";
  //  }

  t.guess(in, ctx);

  auto input_hashes = cista::raw::ankerl_set<cista::hash_t>{};
  auto input_vec = std::vector<std::string>{};
  utl::for_each_token(utl::cstr{ctx.normalized_}, ' ', [&](utl::cstr s) {
    input_hashes.emplace(cista::hash(s.view()));
    auto const start = &s.view()[0];
    auto const size = ctx.normalized_.size() - (start - &ctx.normalized_[0]);
    input_vec.emplace_back(start, size);
  });

  auto const calc_edit_distances =
      [&]<typename T>(std::string_view type,
                      cista::raw::vector_map<T, std::uint8_t>& match_counts,
                      std::vector<match<T>>& matches,
                      data::vector_map<T, string_idx_t> const& names) {
        for (auto [i, m] : utl::enumerate(matches)) {
          //          std::cout << "  IN " << type << " [" << i
          //                    << "]: " << t.strings_[names[m.idx_]].view()
          //                    << " [match_count="
          //                    << static_cast<unsigned>(match_counts[m.idx_])
          //                    << ", sqrt_len_vec_in=" << ctx.sqrt_len_vec_in_
          //                    << ", sqrt_name_len=" <<
          //                    t.match_sqrts_[names[m.idx_]]
          //                    << ", cos_sim = " << m.cos_sim_ << " = "
          //                    << static_cast<unsigned>(match_counts[m.idx_])
          //                    << " / ("
          //                    << ctx.sqrt_len_vec_in_ << " * "
          //                    << t.match_sqrts_[names[m.idx_]] << ") = "
          //                    << (static_cast<float>(match_counts[m.idx_]) /
          //                        (ctx.sqrt_len_vec_in_ *
          //                        t.match_sqrts_[names[m.idx_]]))
          //                    << ", edit_distance=[";
          auto first = true;
          for (auto const& [j, p] : utl::enumerate(in_phrases)) {
            //            if (!first) {
            //              std::cout << ", ";
            //            }
            first = false;
            auto const normalized_str =
                normalize_alloc(t.strings_[names[m.idx_]].view());
            auto const cut_normalized_str = normalized_str.substr(
                0U, std::min(normalized_str.size(), p.s_.size()));
            auto const edit_dist =
                levenshtein_distance(cut_normalized_str, p.s_, ctx.lev_dist_);
            //            std::cout << "\"" << p.s_ << "\" vs \"" <<
            //            cut_normalized_str
            //                      << "\": " << edit_dist << "|"
            //                      << (static_cast<float>(edit_dist) /
            //                          static_cast<float>(p.s_.size()));
            m.edit_dist_[j] = static_cast<float>(edit_dist);
            // (static_cast<float>(edit_dist) /
            // static_cast<float>(p.s_.size()));
          }
          //          std::cout << "]\n";
        }
      };

  //  std::cout << "STREETS\n";
  calc_edit_distances("STREETS", ctx.street_match_counts_, ctx.street_matches_,
                      t.street_names_);

  //  std::cout << "PLACES\n";
  calc_edit_distances("PLACES", ctx.place_match_counts_, ctx.place_matches_,
                      t.place_names_);

  //  std::cout << "AREAS\n";
  calc_edit_distances("AREAS", ctx.area_match_counts_, ctx.area_matches_,
                      t.area_names_);

  auto const get_best_token_match =
      [&](std::array<float, kMaxInputPhrases> const& edit_dist,
          std::uint8_t const exclude_bitmask)
      -> std::pair<float, std::uint8_t> {
    auto min_idx = 0U;
    auto min_dist = std::numeric_limits<float>::max();
    for (auto i = 0U; i != in_phrases.size(); ++i) {
      if ((in_phrases[i].input_token_bits_ & exclude_bitmask) == 0U &&
          edit_dist[i] < min_dist) {
        min_idx = i;
        min_dist = edit_dist[i];
      }
    }
    return {min_dist, min_idx};
  };

  auto area_edit_dist =
      cista::raw::vector_map<area_idx_t, std::array<float, kMaxInputPhrases>>{};
  auto area_active = std::vector<bool>{};
  area_active.resize(t.area_names_.size());
  area_edit_dist.resize(t.area_names_.size(), default_edit_dist());
  for (auto const m : ctx.area_matches_) {
    area_edit_dist[m.idx_] = m.edit_dist_;
    area_active[to_idx(m.idx_)] = true;
  }

  for (auto const& street_match : ctx.street_matches_) {
    //    fmt::print("BOOST: {}",
    //               t.strings_[t.street_names_[street_match.idx_]].view());

    std::set<area_idx_t> areas;
    for (auto const [area_sets, coord] :
         utl::zip(t.street_areas_[street_match.idx_],
                  t.street_coordinates_[street_match.idx_])) {
      for (auto const a : t.area_sets_[area_sets]) {
        if (area_active[to_idx(a)] && t.area_admin_level_[a] >= 6 &&
            t.area_admin_level_[a] <= 8) {
          areas.emplace(a);
        }
      }
    }

    for (auto const a : areas) {
      if (!area_active[to_idx(a)]) {
        continue;
      }

      auto min_dist = std::numeric_limits<std::uint8_t>::max();
      auto min_area_phrase_idx = 0U;
      auto min_street_phrase_idx = 0U;

      if (in_phrases.size() == 1U) {
        std::tie(min_dist, min_street_phrase_idx) =
            get_best_token_match(street_match.edit_dist_, 0U);
      } else {
        for (auto street_phrase_idx = 0U;
             street_phrase_idx != in_phrases.size(); ++street_phrase_idx) {
          for (auto area_phrase_idx = 0U; area_phrase_idx != in_phrases.size();
               ++area_phrase_idx) {
            if ((in_phrases[street_phrase_idx].input_token_bits_ &
                 in_phrases[area_phrase_idx].input_token_bits_) != 0U) {
              continue;
            }

            auto const total = in_phrases[street_phrase_idx].input_token_bits_ |
                               in_phrases[area_phrase_idx].input_token_bits_;
            if (total != all_tokens_mask) {
              continue;
            }

            auto const x = static_cast<std::uint8_t>(
                street_match.edit_dist_[street_phrase_idx] +
                area_edit_dist[a][area_phrase_idx]);

            auto const total_edit_dist =
                std::max(street_match.edit_dist_[street_phrase_idx],
                         area_edit_dist[a][area_phrase_idx]) +
                x;

            if (total_edit_dist < min_dist) {
              min_dist = total_edit_dist;
              min_street_phrase_idx = street_phrase_idx;
              min_area_phrase_idx = area_phrase_idx;
            }
          }
        }
      }

      ctx.suggestions_.emplace_back(suggestion{
          .location_ = address{.street_ = street_match.idx_,
                               .house_number_ =
                                   std::numeric_limits<std::uint16_t>::max()},
          .area_ = a,
          .score_ = min_dist,
          .street_token_ = in_phrases[min_street_phrase_idx].s_,
          .area_token_ = in_phrases[min_area_phrase_idx].s_});

      //      suggestions.back().print(std::cout, t);
    }
  }

  std::sort(begin(ctx.suggestions_), end(ctx.suggestions_),
            [](auto&& a, auto&& b) { return a.score_ < b.score_; });
  ctx.suggestions_.resize(n_suggestions);

  //  std::cout << "SUGGESTIONS\n";
  //  for (auto const& s : suggestions) {
  //    s.print(std::cout, t);
  //  }
}

}  // namespace adr