#pragma once

#include <cstdint>

namespace toolbox {
namespace misc {

[[gnu::unused]] constexpr static inline auto alignUpPowerOf2(uint32_t x) -> uint32_t {
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return x + 1;
}

[[gnu::unused]] constexpr static inline auto alignUpPowerOf2(uint64_t x) -> uint64_t {
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  return x + 1;
}

}  // namespace misc
}  // namespace toolbox