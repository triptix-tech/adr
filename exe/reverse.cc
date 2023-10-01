#include <filesystem>
#include <iostream>

#include "rtree.h"

#include "boost/program_options.hpp"

#include "utl/pairwise.h"

#include "adr/adr.h"
#include "adr/cista_read.h"
#include "adr/guess_context.h"
#include "adr/reverse.h"
#include "adr/typeahead.h"

namespace bpo = boost::program_options;
namespace fs = std::filesystem;

template <typename PolyLine>
std::pair<double, geo::latlng> distance_to_way(geo::latlng const& x,
                                               PolyLine&& c) {
  auto min = std::numeric_limits<double>::max();
  auto best = geo::latlng{};
  for (auto const [a, b] : utl::pairwise(c)) {
    auto const candidate = geo::closest_on_segment(x, a, b);
    auto const dist = geo::distance(x, candidate);
    if (dist < min) {
      min = dist;
      best = candidate;
    }
  }
  return {min, best};
}

int main(int ac, char** av) {
  auto in = fs::path{"adr.cista"};
  auto guess = std::vector<double>{0.0, 0.0};
  auto languages = std::vector<std::string>{"en"};
  auto n = 15U;
  auto runs = 1U;

  try {
    bpo::options_description desc{"Options"};
    desc.add_options()  //
        ("help,h", "Help screen")  //
        ("in,i", bpo::value(&in)->default_value(in),
         "input file")  //
        ("guess,g", bpo::value(&guess)->multitoken(),
         "guess coordinates")  //
        (",n", bpo::value<unsigned>(&n)->default_value(n),
         "number of suggestions")  //
        ("runs,r", bpo::value<unsigned>(&runs)->default_value(runs),
         "number of runs (for benchmarking)")  //
        ("languages,l", bpo::value(&languages)->multitoken(),
         R"(IANA language tags such as "en", "de", "it")");

    auto const pos_desc =
        bpo::positional_options_description{}.add("in", 1).add("guess", -1);

    auto const parsed_options = bpo::command_line_parser{ac, av}
                                    .options(desc)
                                    .positional(pos_desc)
                                    .allow_unregistered()
                                    .run();
    auto vm = bpo::variables_map{};
    bpo::store(parsed_options, vm);
    bpo::notify(vm);

    if (guess.size() % 2U != 0U) {
      std::cout << "even number of coordinates expected, got: " << guess.size()
                << "\n";
      return 1;
    }

    if (guess.empty()) {
      std::cout << "no coordinates, expected even number of coordinates > 0\n";
      return 1;
    }

    if (vm.count("help")) {
      std::cout << desc << '\n';
      return 0;
    }
  } catch (bpo::error const& ex) {
    std::cerr << ex.what() << '\n';
    return 1;
  }

  auto const r =
      adr::cista_read<adr::reverse>(in.generic_string() + ".r.adr", true);
  auto const t = adr::read(in.generic_string() + ".t.adr", false);

  auto rt = r->build_rtree(*t);

  auto lang_indices =
      std::basic_string<adr::language_idx_t>{{adr::kDefaultLang}};
  for (auto const& l_str : languages) {
    auto const l_idx = t->resolve_language(l_str);
    if (l_idx == adr::language_idx_t::invalid()) {
      std::cout << "could not resolve language " << l_str << "\n";
    }
    lang_indices.push_back(l_idx);
  }

  for (auto i = 0U; i != guess.size(); i += 2U) {
    auto const query = geo::latlng{guess[i], guess[i + 1U]};
    auto const min =
        std::array<double, 2U>{query.lng_ - 0.01, query.lat_ - 0.01};
    auto const max =
        std::array<double, 2U>{query.lng_ + 0.01, query.lat_ + 0.01};
    using udata_t = std::tuple<adr::typeahead const*, adr::reverse const*,
                               std::vector<adr::suggestion>*, geo::latlng>;
    auto results = std::vector<adr::suggestion>{};
    auto udata = udata_t{t.get(), r.get(), &results, query};
    rtree_search(
        rt, min.data(), max.data(),
        [](double const* min, double const* max, void const* item,
           void* udata) {
          auto const [t, r, results, query] =
              *reinterpret_cast<udata_t*>(udata);
          auto const e = adr::rtree_entity::from_data(item);

          auto const min_latlng = geo::latlng{min[1], min[0]};

          switch (e.type_) {
            case adr::entity_type::kHouseNumber: {
              auto const& hn = e.hn_;
              auto const c = t->house_coordinates_[hn.street_][hn.idx_];
              results->emplace_back(adr::suggestion{
                  .str_ = t->street_names_[hn.street_][adr::kDefaultLangIdx],
                  .location_ = adr::address{.street_ = hn.street_,
                                            .house_number_ = hn.idx_},
                  .coordinates_ = c,
                  .area_set_ = t->house_areas_[hn.street_][hn.idx_],
                  .matched_area_lang_ = {adr::kDefaultLangIdx} /* TODO */,
                  .matched_areas_ = std::numeric_limits<
                      decltype(std::declval<adr::suggestion>()
                                   .matched_areas_)>::max(),
                  .matched_tokens_ = 0U,
                  .score_ =
                      static_cast<float>(geo::distance(query, c)) - 10.F});
            } break;

            case adr::entity_type::kPlace: {
              auto const& p = e.place_;
              auto const c = t->place_coordinates_[p.place_];
              results->emplace_back(adr::suggestion{
                  .str_ = t->place_names_[p.place_][adr::kDefaultLangIdx],
                  .location_ = p.place_,
                  .coordinates_ = c,
                  .area_set_ = t->place_areas_[p.place_],
                  .matched_area_lang_ = {adr::kDefaultLangIdx} /* TODO */,
                  .matched_areas_ = std::numeric_limits<
                      decltype(std::declval<adr::suggestion>()
                                   .matched_areas_)>::max(),
                  .matched_tokens_ = 0U,
                  .score_ =
                      static_cast<float>(geo::distance(query, c)) - 10.F});
            } break;

            case adr::entity_type::kStreet:
              auto const& s = e.street_segment_;
              auto const [dist, closest] = distance_to_way(
                  query, r->street_segments_[s.street_][s.segment_]);
              results->emplace_back(adr::suggestion{
                  .str_ = t->street_names_[s.street_][adr::kDefaultLangIdx],
                  .location_ = adr::address{.street_ = s.street_,
                                            .house_number_ =
                                                adr::address::kNoHouseNumber},
                  .coordinates_ = adr::coordinates::from_latlng(closest),
                  .area_set_ = t->street_areas_[s.street_][0U] /* TODO */,
                  .matched_area_lang_ = {adr::kDefaultLangIdx} /* TODO */,
                  .matched_areas_ = std::numeric_limits<
                      decltype(std::declval<adr::suggestion>()
                                   .matched_areas_)>::max(),
                  .matched_tokens_ = 0U,
                  .score_ = static_cast<float>(dist)});
              break;
          }

          return true;
        },
        &udata);

    utl::nth_element(results, 10U);
    results.resize(10U);
    utl::sort(results);

    std::cout << "results for " << guess[i] << ", " << guess[i + 1] << "\n";
    for (auto const& [j, s] : utl::enumerate(results)) {
      std::cout << "[" << j << "]\t";
      s.print(std::cout, *t, lang_indices);
    }
  }
}