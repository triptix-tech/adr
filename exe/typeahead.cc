#include <filesystem>
#include <iostream>

#include "boost/program_options.hpp"

#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"  // for Input, Renderer, Vertical
#include "ftxui/component/component_base.hpp"  // for ComponentBase
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

  try {
    bpo::options_description desc{"Options"};
    desc.add_options()  //
        ("help,h", "Help screen")  //
        ("in,i", bpo::value<fs::path>(&in)->default_value(in),
         "OSM input file")  //
        ("guess,g", bpo::value<std::string>(&guess)->default_value(guess),
         "guess input string");

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
  } catch (bpo::error const& ex) {
    std::cerr << ex.what() << '\n';
    return 1;
  }

  auto const t = adr::read(in);

  if (!guess.empty()) {
    auto ctx = adr::guess_context{};
    adr::get_suggestions(*t, geo::latlng{0, 0}, guess, 10, ctx);
    for (auto const& s : ctx.suggestions_) {
      s.print(std::cout, *t);
    }
    return 0;
  } else {
    using namespace ftxui;

    std::string line;
    auto ctx = adr::guess_context{};

    std::string first_name;
    auto const guesses = [&]() {
      adr::get_suggestions(*t, geo::latlng{0, 0}, first_name, 10, ctx);

      Elements list;
      for (auto const& s : ctx.suggestions_) {
        auto ss = std::stringstream{};
        s.print(ss, *t);
        list.push_back(text(ss.str()));
      }
      return vbox(std::move(list));
    };

    InputOption options;
    Component input_first_name = Input(&first_name, "", options);

    auto component = Container::Vertical({input_first_name});

    auto renderer = Renderer(component, [&] {
      return bgcolor(Color::DeepPink1,
                     vbox({input_first_name->Render(), hbox(guesses())}));
    });

    auto screen = ScreenInteractive::TerminalOutput();
    screen.Loop(renderer);
  }
}