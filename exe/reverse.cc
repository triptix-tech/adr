#include <filesystem>
#include <iostream>

#include "rtree.h"

#include "boost/program_options.hpp"

#include "utl/timer.h"

#include "adr/adr.h"
#include "adr/area_database.h"
#include "adr/area_set.h"
#include "adr/cista_read.h"
#include "adr/guess_context.h"
#include "adr/reverse.h"
#include "adr/typeahead.h"

namespace bpo = boost::program_options;
namespace fs = std::filesystem;

int main(int ac, char** av) {
  auto in = fs::path{"adr"};
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

  auto const r = adr::cista_read<adr::reverse>(in / "r.bin", true);
  auto const t = adr::read(in / "t.bin", false);
  adr::print_stats(*t);
  auto const area_db = adr::area_database{in, cista::mmap::protection::READ};

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

  auto cache = adr::cache{t->strings_.size(), 1000U};
  auto ctx = adr::guess_context{cache};
  auto areas = std::basic_string<adr::area_idx_t>{};

  for (auto i = 0U; i != guess.size(); i += 2U) {
    auto const timer = utl::scoped_timer{"lookup"};
    auto const query = geo::latlng{guess[i], guess[i + 1U]};

    r->lookup(*t, ctx, rt, query, 10U);

    area_db.lookup(*t, adr::coordinates::from_latlng(query), areas);

    std::cout << "results for " << query << "\n";
    std::cout << "areas: " << adr::area_set{*t, lang_indices, areas, 0U, {}}
              << "\n";
    for (auto const& [j, s] : utl::enumerate(ctx.suggestions_)) {
      std::cout << "[" << j << "]\t";
      s.print(std::cout, *t, lang_indices);
    }
  }

  rtree_free(rt);
}