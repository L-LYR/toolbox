#pragma once

#include <chrono>
#include <vector>

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

/// Notice:
///    Before you use this timer, you shall check your cpu have constant_tsc flag
///    by using command: `cat /proc/cpuinfo | grep constant_tsc`.
class ClockTimer {
 public:
  ClockTimer() = default;
  ~ClockTimer() = default;

 public:
  auto begin() -> void { begin_ = tscp(); }
  auto end() -> void { end_ = tscp(); }
  auto reset() -> void { begin_ = end_ = 0; }
  auto elapsed() -> uint64_t { return end_ - begin_; }

 private:
  uint64_t begin_{};
  uint64_t end_{};
};

class TraceTimer {
 public:
  TraceTimer() = default;
  ~TraceTimer() = default;

 private:
  auto tick() -> void { trace_.emplace_back(clock::now()); }

  auto reset() -> void { trace_.resize(0); }

  auto nPeriod() -> size_t { return trace_.size(); }

  template <typename T>
  auto period(uint32_t i) -> uint64_t {
    return std::chrono::duration_cast<T>(trace_.at(i + 1) - trace_.at(i)).count();
  }

  template <typename T>
  auto elapsed() -> uint64_t {
    return std::chrono::duration_cast<T>(trace_.back() - trace_.front()).count();
  }

 private:
  std::vector<time_point> trace_;
};

}  // namespace util
}  // namespace toolbox