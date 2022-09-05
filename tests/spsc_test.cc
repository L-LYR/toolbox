#include "spsc.hh"

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <array>

#include "util/timer.hh"
#include "util/type.hh"

enum ConsumerOp {
  CheckAndPop,
  Pop,
};

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

template <typename Queue, size_t QueueSize, size_t TestDataSize, ConsumerOp Op>
class CorrectnessTest {
 public:
  using T = typename Queue::ValueType;

 public:
  explicit CorrectnessTest() : queue_(QueueSize) {
    for (size_t i = 0; i < TestDataSize; i++) {
      test_data_.at(i) = TestData<T>::generate();
    }
  }
  ~CorrectnessTest() = default;

 public:
  auto operator()() {
    spdlog::info("Queue Value Type: {}, Queue Size: {}, TestDataSize: {}, Consumer Op: {}",
                 toolbox::util::typenameOf<T>(), QueueSize, TestDataSize,
                 (Op == Pop ? "pop" : "check and pop"));

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
      while (not queue_.push(e))
        ;
    }
  }

  auto consume() -> void {
    switch (Op) {
      case CheckAndPop: {
        return checkAndPop();
      }
      case Pop: {
        return pop();
      }
    }
  }
  auto checkAndPop() -> void {
    for (auto &expect : test_data_) {
      T *got = nullptr;
      while (got == nullptr) {
        got = queue_.front();
      }
      EXPECT_EQ(*got, expect);
      queue_.pop();
    }
  }
  auto pop() -> void {
    for (auto &expect : test_data_) {
      T got;
      while (not queue_.pop(got))
        ;
      EXPECT_EQ(got, expect);
    }
  }

 private:
  Queue queue_;
  toolbox::util::Timer timer_;
  std::array<T, TestDataSize> test_data_;
};

template <typename T>
using Queue = toolbox::container::SPSCQueue<T>;

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

TEST(SPSCQueue, CorrectnessTest) {
  TestRunner<CorrectnessTest<Queue<uint64_t>, 65535, 1 << 24, CheckAndPop>>().Run();
  TestRunner<CorrectnessTest<Queue<uint64_t>, 65535, 1 << 24, Pop>>().Run();
  TestRunner<CorrectnessTest<Queue<std::string>, 65535, 1 << 24, CheckAndPop>>().Run();
  TestRunner<CorrectnessTest<Queue<std::string>, 65535, 1 << 24, Pop>>().Run();
}

class DtorCounter {
 private:
  static uint32_t n;

 public:
  static auto get() -> uint32_t { return n; }

 public:
  DtorCounter() { ++n; }
  DtorCounter(const DtorCounter &) { ++n; }
  ~DtorCounter() { --n; }
};

uint32_t DtorCounter::n = 0;

TEST(SPSCQueue, DtorTest) {
  {
    Queue<DtorCounter> q(1024);
    for (int i = 0; i < 10; ++i) {
      EXPECT_TRUE(q.push(DtorCounter()));
    }
    EXPECT_EQ(DtorCounter::get(), 10);
    {
      DtorCounter dummy;
      EXPECT_TRUE(q.pop(dummy));
      EXPECT_TRUE(q.pop(dummy));
    }
    EXPECT_EQ(DtorCounter::get(), 8);
  }
  EXPECT_EQ(DtorCounter::get(), 0);
  {
    Queue<DtorCounter> q(4);
    for (int i = 0; i < 3; ++i) {
      EXPECT_TRUE(q.push(DtorCounter()));
    }
    EXPECT_EQ(DtorCounter::get(), 3);
    {
      DtorCounter dummy;
      EXPECT_TRUE(q.pop(dummy));
    }
    EXPECT_EQ(DtorCounter::get(), 2);
    EXPECT_TRUE(q.push(DtorCounter()));
    EXPECT_EQ(DtorCounter::get(), 3);
  }
  EXPECT_EQ(DtorCounter::get(), 0);
}

TEST(SPSCQueue, MiscTest) {
  Queue<int> queue(2);
  EXPECT_EQ(queue.capacity(), 2);

  EXPECT_TRUE(queue.isEmpty());
  EXPECT_FALSE(queue.isFull());

  EXPECT_TRUE(queue.push(1));
  EXPECT_FALSE(queue.isEmpty());
  EXPECT_FALSE(queue.isFull());

  EXPECT_TRUE(queue.push(2));
  EXPECT_FALSE(queue.isEmpty());
  EXPECT_TRUE(queue.isFull());

  EXPECT_FALSE(queue.push(3));
  EXPECT_EQ(queue.approximateSize(), 2);
}