#pragma once

#include <memory>
#include <random>

template <typename T>
class TestData {
 public:
  static auto generate() -> T { return rand() % (1000000007UL); }
};

template <>
class TestData<std::string> {
 public:
  static auto generate() -> std::string { return std::string(12, 'F'); }
};

template <typename T>
class TestRunner {
 public:
  TestRunner() : t_(new T) {}
  ~TestRunner() = default;

 public:
  auto Run() -> void { (*t_)(); }

 private:
  std::unique_ptr<T> t_;
};

class DtorCounter {
 private:
  inline static uint32_t n = 0;

 public:
  static auto get() -> uint32_t { return n; }

 public:
  DtorCounter() { ++n; }
  DtorCounter(const DtorCounter &) { ++n; }
  ~DtorCounter() { --n; }
};