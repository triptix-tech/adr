#include <filesystem>
#include <iostream>

#include "boost/program_options.hpp"

#include "utl/enumerate.h"
#include "utl/timing.h"

#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"  // for Input, Renderer, Vertical
#include "ftxui/component/component_options.hpp"  // for InputOption
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for text, hbox, separator, Element, operator|, vbox, border
#include "ftxui/util/ref.hpp"  // for Ref

#include "adr/adr.h"

namespace bpo = boost::program_options;
namespace fs = std::filesystem;

int main(int ac, char** av) {
  auto in = fs::path{"adr.cista"};
  auto guess = std::string{""};
  auto verbose = false;
  auto n = 15U;
  auto mapped = false;
  auto runs = 1U;

  try {
    bpo::options_description desc{"Options"};
    desc.add_options()  //
        ("help,h", "Help screen")  //
        ("verbose,v", "Print debug output")  //
        ("in,i", bpo::value<fs::path>(&in)->default_value(in),
         "OSM input file")  //
        ("guess,g", bpo::value<std::string>(&guess)->default_value(guess),
         "guess input string")  //
        (",n", bpo::value<unsigned>(&n)->default_value(n),
         "number of suggestions")  //
        ("runs,r", bpo::value<unsigned>(&runs)->default_value(runs),
         "number of runs (for benchmarking)")  //
        ("mmap,m", "use memory mapping");

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
    if (vm.count("mmap")) {
      mapped = true;
    }
  } catch (bpo::error const& ex) {
    std::cerr << ex.what() << '\n';
    return 1;
  }

  auto const t = adr::read(in, mapped);

  if (!guess.empty()) {
    auto ctx = adr::guess_context{};

    UTL_START_TIMING(timer);
    for (auto i = 0U; i != runs; ++i) {
      if (verbose) {
        adr::get_suggestions<true>(*t, geo::latlng{0, 0}, guess, n, ctx);
      } else {
        adr::get_suggestions<false>(*t, geo::latlng{0, 0}, guess, n, ctx);
      }
    }
    UTL_STOP_TIMING(timer);
    std::cout << "\n" << UTL_TIMING_MS(timer) << " ms";
    if (runs != 1U) {
      std::cout << ", average=" << (UTL_TIMING_MS(timer) / runs) << " ms";
    }
    std::cout << "\n";

    for (auto const& s : ctx.suggestions_) {
      s.print(std::cout, *t, ctx.phrases_);
    }
    return 0;
  } else {
    using namespace ftxui;

    auto screen = ScreenInteractive::TerminalOutput();

    std::string line;
    auto ctx = adr::guess_context{};

    std::string first_name;
    auto const guesses = [&]() {
      if (first_name == "quit!") {
        screen.Exit();
      }

      adr::get_suggestions<false>(*t, geo::latlng{0, 0}, first_name, n, ctx);

      Elements list;
      for (auto const& s : ctx.suggestions_) {
        auto ss = std::stringstream{};
        s.print(ss, *t, ctx.phrases_);
        list.push_back(text(ss.str()));
      }
      return vbox(std::move(list)) | bgcolor(Color::White);
    };

    InputOption options;
    Component input_first_name = Input(&first_name, "", options);

    auto component = Container::Vertical({input_first_name});

    auto renderer = Renderer(component, [&] {
      return vbox({input_first_name->Render(), guesses()}) |
             bgcolor(Color::Black);
    });

    screen.Loop(renderer);
  }
}