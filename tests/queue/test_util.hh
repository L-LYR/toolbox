#pragma once
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <array>
#include <memory>
#include <random>

#include "queue/spsc.hh"
#include "util/timer.hh"
#include "util/type.hh"

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
  template <typename... Args>
  TestRunner(Args &&...args) : t_(new T(std::forward<Args>(args)...)) {}
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

template <typename Queue, size_t TestDataSize>
class CorrectnessTest {
 public:
  using T = typename Queue::ValueType;

 public:
  explicit CorrectnessTest(Queue &q) : consumer_(q), producer_(q) {
    for (size_t i = 0; i < TestDataSize; i++) {
      test_data_.at(i) = TestData<T>::generate();
    }
  }
  ~CorrectnessTest() = default;

 public:
  auto operator()() {
    spdlog::info("Queue Type: {}, TestDataSize: {}", toolbox::util::typenameOf<Queue>(),
                 TestDataSize);

    timer_.begin();

    std::thread producer([this] { produce(); });
    std::thread consumer([this] { consume(); });

    producer.join();
    consumer.join();

    timer_.end();
    spdlog::info("done : {} ms", timer_.elapsed<std::chrono::milliseconds>());
  }

 public:
  auto produce() -> void {
    for (auto &e : test_data_) {
      while (not producer_.push(e))
        ;
    }
  }

  auto consume() -> void {
    for (auto &expect : test_data_) {
      T got;
      while (not consumer_.pop(got))
        ;
      EXPECT_EQ(got, expect);
    }
  }

 private:
  toolbox::container::QueueConsumer<Queue> consumer_;
  toolbox::container::QueueProducer<Queue> producer_;
  toolbox::util::Timer timer_;
  std::array<T, TestDataSize> test_data_;
};