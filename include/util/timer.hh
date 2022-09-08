#pragma once

#include <chrono>

namespace toolbox {
namespace util {

using seconds = std::chrono::seconds;
using milliseconds = std::chrono::milliseconds;
using microseconds = std::chrono::microseconds;
using nanoseconds = std::chrono::nanoseconds;
using clock = std::chrono::steady_clock;  // monotonic clock
using time_point = std::chrono::time_point<clock>;
template <typename T>
using duration = std::chrono::duration<T>;

class Timer {
 public:
  Timer() = default;
  ~Timer() = default;

 public:
  auto begin() -> void { begin_ = clock::now(); }
  auto end() -> void { end_ = clock::now(); }
  auto reset() -> void { begin_ = end_ = {}; }

  template <typename T>
  auto elapsed() -> uint64_t {
    return std::chrono::duration_cast<T>(end_ - begin_).count();
  }

 private:
  time_point begin_{};
  time_point end_{};
};

inline auto tsc() -> uint64_t {
  uint64_t a, d;
  asm volatile("rdtsc" : "=a"(a), "=d"(d));
  return (d << 32) | a;
}

inline auto tscp() -> uint64_t {
  uint64_t a, d;
  asm volatile("rdtscp" : "=a"(a), "=d"(d));
  return (d << 32) | a;
}

}  // namespace util
}  // namespace toolbox