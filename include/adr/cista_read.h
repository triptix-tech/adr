#pragma once

#include "cista/memory_holder.h"
#include "cista/mmap.h"
#include "cista/serialization.h"

namespace adr {

template <typename T>
cista::wrapped<T> cista_read(std::filesystem::path const& path_in,
                             bool const mapped) {
  constexpr auto const kMode =
      cista::mode::UNCHECKED | cista::mode::WITH_STATIC_VERSION;
  if (mapped) {
    auto b = cista::buf{
        cista::mmap{path_in.string().c_str(), cista::mmap::protection::READ}};
    auto const p = cista::deserialize<T, kMode>(b);
    return cista::wrapped{std::move(b), p};
  } else {
    auto b = cista::file{path_in.c_str(), "r"}.content();
    auto const p = cista::deserialize<T, kMode>(b);
    return cista::wrapped{std::move(b), p};
  }
}

}  // namespace adr