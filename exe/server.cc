#include <filesystem>

#include "boost/program_options.hpp"
#include "boost/thread/tss.hpp"

#include "fmt/core.h"

#include <openssl/ssl.h>

#include "App.h"

#include "utl/timing.h"

#include "blockingconcurrentqueue.h"

#include "adr/adr.h"
#include "adr/crypto.h"
#include "adr/json.h"
#include "adr/parse_get_parameters.h"
#include "adr/request.h"
#include "adr/typeahead.h"
#include "adr/url_decode.h"

namespace fs = std::filesystem;

int main(int ac, char** av) {
  auto in = fs::path{"adr.cista"};
  auto mapped = false;
  auto host = std::string{"0.0.0.0"};
  auto key_path = std::string{"./key.txt"};
  auto iv_path = std::string{"./iv.txt"};
  auto port = 8080;
  auto num_threads = std::thread::hardware_concurrency();
  auto unsafe = false;

  try {
    namespace bpo = boost::program_options;

    bpo::options_description desc{"Options"};
    desc.add_options()  //
        ("help,h", "Help screen")  //
        ("in,i", bpo::value(&in)->default_value(in),
         "OSM input file")  //
        ("host,h", bpo::value(&host)->default_value(host), "host")  //
        ("port,p", bpo::value(&port)->default_value(port), "port")  //
        ("key_path", bpo::value(&key_path)->default_value(key_path),
         "key_path")  //
        ("iv_path", bpo::value(&iv_path)->default_value(iv_path), "iv_path")  //
        ("threads,t", bpo::value(&num_threads)->default_value(num_threads))  //
        ("unsafe,u", "unsafe")  //
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
    if (vm.count("unsafe")) {
      unsafe = true;
    }
  } catch (std::exception const& ex) {
    std::cerr << ex.what() << '\n';
    return 1;
  }

  auto const key = adr::crypto::read_base64_file(key_path, 32);
  auto const iv = adr::crypto::read_base64_file(iv_path, 16);
  auto const t = adr::read(in, mapped);
  auto cache = adr::cache{.n_strings_ = t->strings_.size(), .max_size_ = 1000U};

  auto workers = std::vector<std::thread>();
  workers.resize(std::thread::hardware_concurrency());
  for (auto& w : workers) {
    w = std::thread{[&]() {
      auto in = std::string{};
      auto ctx = adr::guess_context{cache};
      auto lang_list =
          std::basic_string<adr::language_idx_t>{adr::kDefaultLang};
      auto c = adr::crypto{iv, key};
      auto h = adr::http{};

      ctx.resize(*t);

      auto server = uWS::App();

      if (unsafe) {
        server.get("/g/:req", [&](uWS::HttpResponse<false>* res,
                                  uWS::HttpRequest* req) {
          adr::url_decode(req->getParameter(0), in);
          res->end(request(in, *t, ctx, adr::api::kGPlaces));
          res->writeHeader("Content-Type", "application/json;charset=UTF-8");
        });
      }

      server.post("/", [&](uWS::HttpResponse<false>* res,
                           uWS::HttpRequest* req) {
        auto aborted = std::make_shared<bool>(false);
        res->onData([&c, &h, res, req_body = std::make_shared<std::string>(),
                     aborted](std::string_view chunk, bool const fin) mutable {
          (*req_body) += chunk;
          if (fin && !*aborted) {
            adr::decode(c,
                        std::span{reinterpret_cast<std::uint8_t const*>(
                                      req_body->data()),
                                  req_body->size()},
                        h);
            std::cout << h << "\n";
            auto const response = adr::encode(c, {.body_ = "Hello World!"});
            res->end({reinterpret_cast<char const*>(response.data()),
                      response.size()});
          }
        });
        res->onAborted([aborted]() { *aborted = true; });
      });
      server.listen(port, [&](auto* listen_socket) {
        if (listen_socket) {
          std::cout << "Thread " << std::this_thread::get_id()
                    << " listening on port " << port << std::endl;
        } else {
          std::cout << "Thread " << std::this_thread::get_id()
                    << " failed to listen on port port" << std::endl;
        }
      });
      server.run();
    }};
  }

  for (auto& w : workers) {
    w.join();
  }

  return 1;
}