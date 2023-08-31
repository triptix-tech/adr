#pragma once

#include "fmt/core.h"

#ifndef trace
#define trace(...)           \
  if constexpr (Debug) {     \
    fmt::print(__VA_ARGS__); \
  }                          \
  (void)0
#endif