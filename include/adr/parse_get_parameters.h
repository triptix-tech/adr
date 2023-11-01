#pragma once

#include <string_view>

#include "adr/score.h"
#include "utl/parser/arg_parser.h"
#include "utl/verify.h"

namespace adr {

template <typename Fn>
void parse_get_parameters(std::string_view decoded, Fn&& fn) {
  enum state { kStart, kKey, kValue } s = kStart;
  auto key = std::string_view{};
  while (!decoded.empty()) {
    switch (s) {
      case kStart:
        if (decoded[0] != '?' && decoded[0] != '&') {
          return;
        }
        decoded = decoded.substr(1);
        s = kKey;
        break;

      case kKey: {
        key = adr::get_until(decoded, '&', '=');
        decoded = decoded.substr(key.size());
        s = kValue;
        break;
      }

      case kValue: {
        if (decoded[0] != '=') {
          fn(key, std::string_view{});
          continue;
        }
        decoded = decoded.substr(1);

        auto const value = adr::get_until(decoded, '&');
        decoded = decoded.substr(value.size());

        fn(key, value);

        s = kStart;

        break;
      }
    }
  }
}

std::optional<geo::latlng> parse_latlng(std::string_view s) {
  auto const lat = utl::get_until(utl::cstr{s}, ',');
  if (!lat || lat[lat.len] != ',') {
    return std::nullopt;
  }
  auto const lng = s.substr(lat.len + 1U);
  if (lng.empty()) {
    return std::nullopt;
  }
  return geo::latlng{utl::parse<double>(lat), utl::parse<double>(lng)};
}

}  // namespace adr