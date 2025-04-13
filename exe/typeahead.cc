#include <filesystem>
#include <iostream>

#include "boost/program_options.hpp"

#include "fmt/format.h"

#include "utl/enumerate.h"
#include "utl/parser/cstr.h"
#include "utl/read_file.h"
#include "utl/timing.h"

#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"  // for Input, Renderer, Vertical
#include "ftxui/component/component_options.hpp"  // for InputOption
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for text, hbox, separator, Element, operator|, vbox, border
#include "ftxui/util/ref.hpp"  // for Ref

#include "adr/adr.h"
#include "adr/typeahead.h"

namespace bpo = boost::program_options;
namespace fs = std::filesystem;

constexpr auto const kAddresses = std::array<std::string_view, 16>{
    "Oldesloer Strasse 74",
    "Pohlstrasse 73 Niedersachsen",
    "10532 S Redwood Rd",
    "Chausseestr. 27 23423 Berlin",
    "South Jordan",
    "2104 John F Kennedy Blvd W",
    "1B Chandos Rd",
    "United Kingdom",
    "Market St",
    "Neustadt An Der Weinstraße Königsbach",
    "24 Rue de Madrid Bouches-du-Rhône",
    "53 Rue de la Scellerie Indre-et-Loire",
    "49 Rue Etienne Marcel",
    "An der Hauptwache 15 Frankfurt am Main",
    "4500 Kingsway #1001 Burnaby British Columbia",
    "3833 Princeton Pkwy SW"};

int main(int ac, char** av) {
  auto in = fs::path{"adr"};
  auto guess = std::string{""};
  auto file = std::string{""};
  auto languages = std::vector<std::string>{};
  auto verbose = false;
  auto n = 15U;
  auto runs = 1U;
  auto warmup = false;
  auto benchmark = false;
  auto dark = false;

  try {
    bpo::options_description desc{"Options"};
    desc.add_options()  //
        ("help,h", "Help screen")  //
        ("verbose,v", "Print debug output")  //
        ("benchmark,b", "parallel benchmark on all threads")  //
        ("dark,d", "dark mode")  //
        ("file,f", bpo::value<std::string>(&file)->default_value(file),
         "read inputs from file")  //
        ("warmup,w", "warm up with test query")  //
        ("in,i", bpo::value<fs::path>(&in)->default_value(in),
         "OSM input file")  //
        ("guess,g", bpo::value<std::string>(&guess)->default_value(guess),
         "guess input string")  //
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

    if (vm.count("help")) {
      std::cout << desc << '\n';
      return 0;
    }

    if (vm.count("verbose")) {
      verbose = true;
    }
    if (vm.count("warmup")) {
      warmup = true;
    }
    if (vm.count("benchmark")) {
      benchmark = true;
    }
    if (vm.count("dark")) {
      dark = true;
    }
  } catch (bpo::error const& ex) {
    std::cerr << ex.what() << '\n';
    return 1;
  }

  auto const t = adr::read(in);
  adr::print_stats(*t);

  auto lang_indices =
      std::basic_string<adr::language_idx_t>{{adr::kDefaultLang}};
  for (auto const& l_str : languages) {
    auto const l_idx = t->resolve_language(l_str);
    if (l_idx == adr::language_idx_t::invalid()) {
      std::cout << "could not resolve language " << l_str << "\n";
    }
    lang_indices.push_back(l_idx);
  }

  auto const coord =
      std::optional{geo::latlng{49.8731001322536, 8.647738878714677}};

  auto cache = adr::cache{t->strings_.size(), 1000U};
  auto ctx = adr::guess_context{cache};
  ctx.resize(*t);

  if (warmup) {
    adr::get_suggestions<false>(
        *t, "Willy Brandt Platz 64289 Darmstadt Deutschland", n, lang_indices,
        ctx, coord, 1.0);
  }

  if (!file.empty()) {
    std::cout << "reading file " << file << "\n";
    auto const content = utl::read_file(file.c_str());
    if (!content.has_value()) {
      std::cout << "unable to read file " << file << "\n";
    }
    utl::for_each_line(utl::cstr{*content}, [&](utl::cstr const line) {
      UTL_START_TIMING(timer);
      adr::get_suggestions<false>(*t, line.to_str(), n, lang_indices, ctx,
                                  coord, 1.0);
      UTL_STOP_TIMING(timer);
      std::cout << UTL_TIMING_MS(timer) << " ms\n";
    });
    return 0;
  }

  if (benchmark) {
    auto count = std::atomic_uint32_t{0U};
    auto threads = std::vector<std::thread>{};
    auto const num_threads = std::thread::hardware_concurrency();

    UTL_START_TIMING(timer);
    for (auto i = 0U; i != num_threads; ++i) {
      threads.emplace_back([&]() {
        auto ctx = adr::guess_context{cache};
        ctx.resize(*t);

        while (true) {
          adr::get_suggestions<false>(
              *t, std::string{kAddresses[count % kAddresses.size()]}, n,
              lang_indices, ctx, coord, 1.0);
          ++count;
          if (count > 1'000) {
            break;
          }
        }
      });
    }
    for (auto& thread : threads) {
      thread.join();
    }
    UTL_STOP_TIMING(timer);
    std::cout << UTL_TIMING_MS(timer) << " ms\n";
    return 0;
  }

  if (!guess.empty()) {
    UTL_START_TIMING(timer);
    for (auto i = 0U; i != runs; ++i) {
      if (verbose) {
        adr::get_suggestions<true>(*t, guess, n, lang_indices, ctx, coord, 1.0);
      } else {
        adr::get_suggestions<false>(*t, guess, n, lang_indices, ctx, coord,
                                    1.0);
      }
    }
    UTL_STOP_TIMING(timer);
    std::cout << UTL_TIMING_MS(timer) << " ms";
    if (runs != 1U) {
      std::cout << ", average=" << (UTL_TIMING_MS(timer) / runs) << " ms";
    }
    std::cout << "\n";

    for (auto const& [i, s] : utl::enumerate(ctx.suggestions_)) {
      std::cout << "[" << i << "]\t";
      s.print(std::cout, *t, lang_indices);
    }
    return 0;
  } else {
    using namespace ftxui;

    auto screen = ScreenInteractive::TerminalOutput();

    std::string line;
    std::string first_name;
    auto const guesses = [&]() {
      if (first_name == "quit!") {
        screen.Exit();
      }

      UTL_START_TIMING(timer);
      adr::get_suggestions<false>(*t, first_name, n, lang_indices, ctx, coord,
                                  1.0);
      UTL_STOP_TIMING(timer);

      Elements list;
      list.emplace_back(text(fmt::format("{} ms", UTL_TIMING_MS(timer))));
      for (auto const& s : ctx.suggestions_) {
        auto ss = std::stringstream{};
        s.print(ss, *t, lang_indices);
        list.push_back(text(ss.str()));
      }
      return vbox(std::move(list)) |
             bgcolor(dark ? Color::Black : Color::White);
    };

    InputOption options;
    Component input_first_name = Input(&first_name, "", options);

    auto component = Container::Vertical({input_first_name});

    auto renderer = Renderer(component, [&] {
      return vbox(
          {input_first_name->Render() | bgcolor(Color::Black), guesses()});
    });

    screen.Loop(renderer);
  }
}