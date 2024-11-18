#include <filesystem>
#include <iostream>

#include "boost/program_options.hpp"

#include "utl/progress_tracker.h"

#include "adr/adr.h"

namespace bpo = boost::program_options;
namespace fs = std::filesystem;

int main(int ac, char** av) {
  auto const progress_bars = utl::global_progress_bars{false};

  auto in = std::string{"osm.pbf"};
  auto out = std::string{"adr"};
  auto tmp = std::string{"."};

  try {
    bpo::options_description desc{"Options"};
    desc.add_options()  //
        ("help,h", "Help screen")  //
        ("in,i", bpo::value(&in)->default_value(in),
         "OSM input file")  //
        ("out,o", bpo::value(&out)->default_value(out),
         "output file")  //
        ("tmp_dir,t", bpo::value(&tmp)->default_value(tmp),
         "directory for temporary files");

    auto const pos_desc =
        bpo::positional_options_description{}.add("in", 1).add("out", -1);

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

  std::cout << "IN: " << in << "\n"
            << "OUT: " << out << "\n"
            << "TMP: " << tmp << "\n";
  adr::extract(in, out, tmp);
}