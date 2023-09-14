#include <filesystem>

#include "boost/program_options.hpp"
#include "boost/thread/tss.hpp"

#include "fmt/core.h"

#include "App.h"

#include "utl/timing.h"

#include "blockingconcurrentqueue.h"

#include "adr/adr.h"
#include "adr/typeahead.h"

namespace fs = std::filesystem;

bool url_decode(std::string_view in, std::string& out) {
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i) {
    if (in[i] == '%') {
      if (i + 3 <= in.size()) {
        int value = 0;
        std::istringstream is(std::string{in.substr(i + 1, 2)});
        if (is >> std::hex >> value) {
          out += static_cast<char>(value);
          i += 2;
        } else {
          return false;
        }
      } else {
        return false;
      }
    } else if (in[i] == '+') {
      out += ' ';
    } else {
      out += in[i];
    }
  }
  return true;
}

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

  //  using queue_item_t =
  //      std::pair<std::string /* request string */,
  //                std::function<void(std::string /* response */)>>;

  //  auto q = moodycamel::BlockingConcurrentQueue<queue_item_t>{};
  //  auto workers = std::vector<std::thread>{};
  auto cache = adr::cache{.n_strings_ = t->strings_.size(), .max_size_ = 1000U};
  //  for (auto i = 0U; i != num_threads; ++i) {
  //    workers.emplace_back([&]() {
  //      auto req = std::string{};
  //
  //      auto ctx = adr::guess_context{cache};
  //      auto ss = std::stringstream{};
  //      ctx.resize(*t);
  //
  //      auto item = queue_item_t{};
  //
  //      while (true) {
  //        q.wait_dequeue(item);
  //        url_decode(item.first, req);
  //        std::cout << "REQ: " << req << "\n";
  //        adr::get_suggestions<false>(*t, {}, req, 10U, ctx);
  //
  //        ss.str("");
  //        for (auto const& s : ctx.suggestions_) {
  //          s.print(ss, *t);
  //        }
  //        item.second(ss.str());
  //      }
  //    });
  //  }
  //
  //  uWS::App()
  //      .get("/geocode/:req",
  //           [&](uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
  //             UTL_START_TIMING(timer);
  //             res->writeHeader("Content-Type", "text/plain;charset=UTF-8");
  //             auto const loop = uWS::Loop::get();
  //             q.enqueue(queue_item_t{
  //                 req->getParameter(0),
  //                 [res, timer_start, &loop](std::string const& x) {
  //                   loop->defer([&]() {
  //                     res->cork([x, res, timer_start]() {
  //                       UTL_STOP_TIMING(timer);
  //                       fmt::print("request timing: {}\n",
  //                       UTL_TIMING_MS(timer)); res->end(x);
  //                     });
  //                   });
  //                 }});
  //             res->onAborted([]() {});
  //           })
  //      .listen(host, port,
  //              [&](auto* listen_socket) {
  //                if (listen_socket) {
  //                  std::cout << "Listening on port " << port << std::endl;
  //                }
  //              })
  //      .run();

  std::vector<std::thread*> threads(std::thread::hardware_concurrency());

  std::transform(
      threads.begin(), threads.end(), threads.begin(), [&](std::thread* /*t*/) {
        return new std::thread([&]() {
          auto in = std::string{};
          auto ctx = adr::guess_context{cache};
          auto ss = std::stringstream{};
          ctx.resize(*t);

          /* Very simple WebSocket echo server */
          uWS::App()
              .get("/geocode/:req",
                   [&](uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
                     UTL_START_TIMING(timer);
                     res->writeHeader("Content-Type",
                                      "text/plain;charset=UTF-8");
                     url_decode(req->getParameter(0), in);
                     adr::get_suggestions<false>(*t, {}, in, 10U, ctx);
                     ss.str("");
                     for (auto const& s : ctx.suggestions_) {
                       s.print(ss, *t);
                     }
                     res->end(ss.str());
                   })
              .listen(port,
                      [&](auto* listen_socket) {
                        if (listen_socket) {
                          std::cout << "Thread " << std::this_thread::get_id()
                                    << " listening on port " << port
                                    << std::endl;
                        } else {
                          std::cout << "Thread " << std::this_thread::get_id()
                                    << " failed to listen on port port"
                                    << std::endl;
                        }
                      })
              .run();
        });
      });

  std::for_each(threads.begin(), threads.end(),
                [](std::thread* t) { t->join(); });

  std::cout << "Failed to listen on port " << port << std::endl;
  return 1;
}