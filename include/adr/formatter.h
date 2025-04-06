#pragma once

#include <memory>

namespace adr {

struct formatter {
  struct impl;
  std::unique_ptr<impl> impl_;
};

}  // namespace adr