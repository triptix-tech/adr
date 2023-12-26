#include <filesystem>

#include "boost/program_options.hpp"
#include "boost/thread/tss.hpp"

#include "fmt/core.h"

#include <openssl/ssl.h>

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
      return new std::thread([&]() {
        auto in = std::string{};
        auto ctx = adr::guess_context{cache};
        auto lang_list =
            std::basic_string<adr::language_idx_t>{adr::kDefaultLang};
        ctx.resize(*t);
        auto server =
            uWS::SSLApp(uWS::SocketContextOptions{
                            .key_file_name = "deps/uWebSockets/misc/key.pem",
                            .cert_file_name = "deps/uWebSockets/misc/cert.pem",
                            .passphrase = "1234",
                            .ca_file_name = "cert.pem"})
                .get("/g/:req",
                     [&](uWS::HttpResponse<true>* res, uWS::HttpRequest* req) {
                       adr::url_decode(req->getParameter(0), in);
                       res->end(request(in, *t, ctx, adr::api::kGPlaces));
                       res->writeHeader("Content-Type",
                                        "application/json;charset=UTF-8");
                     })
                .get("/mb/:req",
                     [&](uWS::HttpResponse<true>* res, uWS::HttpRequest* req) {
                       adr::url_decode(req->getParameter(0), in);
                       res->end(request(in, *t, ctx, adr::api::kMB));
                       res->writeHeader("Content-Type",
                                        "application/json;charset=UTF-8");
                     })
                .get("/pelias/:req",
                     [&](uWS::HttpResponse<true>* res, uWS::HttpRequest* req) {
                       adr::url_decode(req->getParameter(0), in);
                       res->end(request(in, *t, ctx, adr::api::kPelias));
                       res->writeHeader("Content-Type",
                                        "application/json;charset=UTF-8");
                     })
                .listen(port, [&](auto* listen_socket) {
                  if (listen_socket) {
                    std::cout << "Thread " << std::this_thread::get_id()
                              << " listening on port " << port << std::endl;
                  } else {
                    std::cout << "Thread " << std::this_thread::get_id()
                              << " failed to listen on port port" << std::endl;
                  }
                });
        auto const ssl_context = server.getNativeHandle();
        SSL_CTX_set_verify((SSL_CTX*)ssl_context,
                           SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                           NULL);
        server.run();
      });
    }};
  }

  for (auto& w : workers) {
    w.join();
  }

  std::cout << "Failed to listen on port " << port << std::endl;
  return 1;
}