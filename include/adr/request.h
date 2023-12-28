#pragma once

#include <string>

#include "cista/reflection/comparable.h"
#include "cista/reflection/printable.h"

#include "adr/crypto.h"

namespace adr {

struct http {
  CISTA_FRIEND_COMPARABLE(http);
  CISTA_PRINTABLE(http);
  std::string path_;
  std::string body_;
};

struct encoded {
  std::string iv_;
  std::span<std::uint8_t const> encrypted_;
};

void decode(crypto& c,
            std::span<std::uint8_t const> iv,
            std::span<std::uint8_t const> encoded,
            http& req);

encoded encode(crypto& c, http const& res);

}  // namespace adr