#pragma once

#include <fmt/core.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

namespace toolbox {
namespace util {

/// Suggestion:
///     (2 ^ N - 1) * scale >= upper bound of the data
/// Notice:
///     The results are always smaller or equal to the actual statistics.
template <uint32_t N, uint32_t scale>
class Statistics {
  static_assert(N >= 1, "N is too small");
  static_assert(scale > 1, "scale is too small");

 public:
  Statistics() { reset(); }
  ~Statistics() = default;

 public:
  auto reset() -> void { memset(this, 0, sizeof(Statistics)); }

 public:
  auto record(uint64_t x) -> void {
    uint32_t n = 0;
    uint32_t y = 0;
    for (uint32_t i = 0; i < N; i++) {
      n = (1 << i);
      y = x / n;
      if (y < scale) {
        s[i][y]++;
        return;
      }
      x -= scale * n;
    }
    s_inf_++;
  }

 public:
  // sample number
  auto count() -> uint64_t {
    uint64_t cnt = s_inf_;
    for (uint32_t i = 0; i < N; i++) {
      for (uint32_t j = 0; j < scale; j++) {
        cnt += s[i][j];
      }
    }
    return cnt;
  }

  // approximate sum
  auto sum() -> double {
    double sum = 0;
    uint32_t base = 0;
    uint32_t n = 0;
    for (uint32_t i = 0; i < N; i++) {
      n = (1 << i);
      for (uint32_t j = 0; j < scale; j++) {
        sum += s[i][j] * (double)(n * j + base);
      }
      base += n * scale;
    }
    sum += (double)s_inf_ * base;
    return sum;
  }

  // approximate avg
  auto avg() -> double { return sum() / std::max(1UL, count()); }

  // approximate max
  auto max() -> uint64_t {
    uint64_t base = ((1 << N) - 1) * scale;
    if (s_inf_ > 0) {
      return base;
    }
    uint32_t n = 0;
    for (int32_t i = N - 1; i >= 0; i--) {
      n = (1 << i);
      base -= n * scale;
      for (int32_t j = scale - 1; j >= 0; j--) {
        if (s[i][j] > 0) {
          return base + n * j;
        }
      }
    }
    return 0;
  }

  // approximate min
  auto min() -> uint64_t {
    uint64_t base = 0;
    uint32_t n = 0;
    for (uint32_t i = 0; i < N; i++) {
      n = (1 << i);
      for (uint32_t j = 0; j < scale; j++) {
        if (s[i][j] > 0) {
          return base + n * j;
        }
      }
      base += n * scale;
    }
    return base;
  }

  // approximate p-th percentile sample
  auto percentile(double p) -> uint64_t {
    int64_t threshold = (int64_t)(p * (double)count());
    uint64_t base = 0;
    uint32_t n = 0;
    for (uint32_t i = 0; i < N; i++) {
      n = (1 << i);
      for (uint32_t j = 0; j < scale; j++) {
        threshold -= s[i][j];
        if (threshold < 0) return base + j * n;
      }
      base += n * scale;
    }
    return base;
  }

 public:
  auto dump() -> std::string {
    return fmt::format(
        "count: {}\n"
        "sum:   {}\n"
        "avg:   {}\n"
        "min:   {}\n"
        "max:   {}\n"
        "p50:   {}\n"
        "p90:   {}\n"
        "p95:   {}\n"
        "p99:   {}\n"
        "p999:  {}\n",
        count(), sum(), avg(), min(), max(), percentile(0.5), percentile(0.9),
        percentile(0.95), percentile(0.99), percentile(0.999));
  }

 public:
  auto operator+=(const Statistics<N, scale>& r) -> Statistics<N, scale>& {
    for (uint32_t i = 0; i < N; i++) {
      for (uint32_t j = 0; j < scale; j++) {
        s[i][j] += r.s[i][j];
      }
    }
    s_inf_ += r.s_inf_;
    return *this;
  }

 private:
  uint64_t s[N][scale];
  uint64_t s_inf_;
};

template <uint32_t upperbound>
using AccStatistics = Statistics<1, upperbound>;

}  // namespace util
}  // namespace toolbox