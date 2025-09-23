#pragma once

#include "utl/for_each_bit_set.h"

namespace adr {

template <typename T>
struct bitmask {
  friend std::ostream& operator<<(std::ostream& out, bitmask const x) {
    for (auto bit = 0U; bit != sizeof(T) * 8U; ++bit) {
      out << (utl::has_bit_set(x.t_, bit) ? '1' : '0');
    }
    return out;
  }
  T t_;
};

template <typename T>
bitmask(T) -> bitmask<T>;

}  // namespace adr

template <typename T>
struct fmt::formatter<adr::bitmask<T>> : fmt::ostream_formatter {};