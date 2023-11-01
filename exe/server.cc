#include <filesystem>

#include "boost/program_options.hpp"
#include "boost/thread/tss.hpp"

#include "fmt/core.h"

#include "App.h"

#include "utl/timing.h"

#include "blockingconcurrentqueue.h"

#include "adr/adr.h"
#include "adr/json.h"
#include "adr/parse_get_parameters.h"
#include "adr/typeahead.h"
#include "adr/url_decode.h"
#include "utl/parser/arg_parser.h"

namespace fs = std::filesystem;

int main(int ac, char** av) {
  auto in = fs::path{"adr.cista"};
  auto mapped = false;
  auto host = std::string{"0.0.0.0"};
  auto port = 8080;
  auto num_threads = std::thread::hardware_concurrency();

  try {
    namespace bpo = boost::program_options;

    bpo::options_description desc{"Options"};
    desc.add_options()  //
        ("help,h", "Help screen")  //
        ("in,i", bpo::value(&in)->default_value(in),
         "OSM input file")  //
        ("host,h", bpo::value(&host)->default_value(host), "host")  //
        ("port,p", bpo::value(&port)->default_value(port), "port")  //
        ("threads,t", bpo::value(&num_threads)->default_value(num_threads))  //
        ("mmap,m", "use memory mapping");

    auto const pos_desc =
        bpo::positional_options_description{}.add("in", 1).add("port", -1);

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
    if (vm.count("mmap")) {
      mapped = true;
    }
  } catch (std::exception const& ex) {
    std::cerr << ex.what() << '\n';
    return 1;
  }

  auto const t = adr::read(in, mapped);
  auto cache = adr::cache{.n_strings_ = t->strings_.size(), .max_size_ = 1000U};

  auto workers = std::vector<std::thread>();
  workers.resize(std::thread::hardware_concurrency());
  for (auto& w : workers) {
    w = std::thread{[&]() {
      auto in = std::string{};
      auto ctx = adr::guess_context{cache};
      auto ss = std::stringstream{};
      auto lang_list =
          std::basic_string<adr::language_idx_t>{adr::kDefaultLang};
      ctx.resize(*t);
      uWS::App()
          .get("/v1/autocomplete/g:req",
               [&](uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
                 res->writeHeader("Content-Type",
                                  "application/json;charset=UTF-8");
                 adr::url_decode(req->getParameter(0), in);

                 auto parser_error = false;
                 auto language = std::string_view{};
                 auto input = std::string_view{};
                 auto location = std::optional<geo::latlng>{};
                 auto radius = std::numeric_limits<float>::max();
                 auto strictbounds = false;
                 adr::parse_get_parameters(
                     in, [&](std::string_view key, std::string_view value) {
                       switch (cista::hash(key)) {
                         case cista::hash("input"): input = value; break;

                         case cista::hash("location"):
                           location = adr::parse_latlng(value);
                           if (!location.has_value()) {
                             parser_error = true;
                           }
                           break;

                         case cista::hash("strictbounds"):
                           strictbounds = utl::parse<bool>(value);
                           break;

                         case cista::hash("language"): language = value; break;
                       }
                     });
                 adr::get_suggestions<false>(*t, {}, in, 10U, lang_list, ctx);
                 res->end(adr::to_json(ctx.suggestions_,
                                       adr::output_format::kGPlaces));
               })
          .listen(port,
                  [&](auto* listen_socket) {
                    if (listen_socket) {
                      std::cout << "Thread " << std::this_thread::get_id()
                                << " listening on port " << port << std::endl;
                    } else {
                      std::cout << "Thread " << std::this_thread::get_id()
                                << " failed to listen on port port"
                                << std::endl;
                    }
                  })
          .run();
    }};
  }

  for (auto& w : workers) {
    w.join();
  }

  std::cout << "Failed to listen on port " << port << std::endl;
  return 1;
}