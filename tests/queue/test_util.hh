#pragma once
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <array>
#include <memory>
#include <random>

#include "queue/mpmc.hh"
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
class SPSCCorrectnessTest {
 public:
  using ValueType = typename Queue::ValueType;
  using ConsumerType = typename Queue::ConsumerType;
  using ProducerType = typename Queue::ProducerType;

 public:
  explicit SPSCCorrectnessTest(Queue &q) : consumer_(q), producer_(q) {
    for (size_t i = 0; i < TestDataSize; i++) {
      test_data_.at(i) = TestData<ValueType>::generate();
    }
    spdlog::info("Queue Type: {}, TestDataSize: {}", toolbox::util::TypenameOf<Queue>(),
                 TestDataSize);
    timer_.begin();

    std::thread producer([this] { produce(); });
    std::thread consumer([this] { consume(); });

    producer.join();
    consumer.join();

    timer_.end();
    spdlog::info("done: {} ms", timer_.elapsed<std::chrono::milliseconds>());
  }
  ~SPSCCorrectnessTest() = default;

 public:
  auto produce() -> void {
    for (auto &e : test_data_) {
      while (not producer_.push(e))
        ;
    }
  }

  auto consume() -> void {
    for (auto &expect : test_data_) {
      ValueType got;
      while (not consumer_.pop(got))
        ;
      EXPECT_EQ(got, expect);
    }
  }

 private:
  ConsumerType consumer_;
  ProducerType producer_;
  toolbox::util::Timer timer_;
  std::array<ValueType, TestDataSize> test_data_;
};

template <typename Queue>
class MPMCCorrectnessTest {
 public:
  using ValueType = typename Queue::ValueType;
  using ConsumerType = typename Queue::ConsumerType;
  using ProducerType = typename Queue::ProducerType;

  static_assert(std::is_same<uint64_t, ValueType>(), "we need uint64_t");

 public:
  static auto pushPop(uint32_t n_thread, uint32_t n_ops, Queue &q,
                      std::atomic_uint64_t &sum, uint32_t x) -> void {
    auto producer = q.producer();
    auto consumer = q.consumer();
    uint64_t local_sum = 0;
    uint64_t src = x;
    uint64_t received = x;
    while (src < n_ops || received < n_ops) {
      if (src < n_ops && producer.push(src)) {
        src += n_thread;
      }
      uint64_t dst = 0;
      if (received < n_ops && consumer.pop(dst)) {
        received += n_thread;
        local_sum += dst;
      }
    }
    sum += local_sum;
  }

 public:
  explicit MPMCCorrectnessTest(Queue &q, uint32_t n_threads, uint32_t n_ops) {
    spdlog::info("Queue Type: {}, N thread: {}, N Ops: {}",
                 toolbox::util::TypenameOf<Queue>(), n_threads, n_ops);
    timer_.begin();

    std::atomic_uint64_t sum(0);
    for (uint32_t i = 0; i < n_threads; i++) {
      ts_.emplace_back(&MPMCCorrectnessTest::pushPop, n_threads, n_ops, std::ref(q),
                       std::ref(sum), i);
    }
    for (auto &t : ts_) {
      t.join();
    }
    uint64_t expect = (uint64_t)n_ops * (n_ops - 1) / 2 - sum;
    EXPECT_EQ(expect, 0);

    timer_.end();
    spdlog::info("done: {} ms", timer_.elapsed<std::chrono::milliseconds>());
  }

  ~MPMCCorrectnessTest() = default;

 private:
  std::vector<std::thread> ts_;
  toolbox::util::Timer timer_;
};

#define RunSPSCCorrectnessTest(q, size) \
  std::make_shared<SPSCCorrectnessTest<decltype(q), size>>(q)
#define RunMPMCCorrectnessTest(q, n_threads, n_ops) \
  std::make_shared<MPMCCorrectnessTest<decltype(q)>>(q, n_threads, n_ops)
