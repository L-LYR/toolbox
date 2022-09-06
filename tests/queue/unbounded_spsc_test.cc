#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <array>

#include "queue/spsc.hh"
#include "test_util.hh"
#include "util/timer.hh"
#include "util/type.hh"

template <typename Queue, size_t TestDataSize>
class CorrectnessTest {
 public:
  using T = typename Queue::ValueType;

 public:
  explicit CorrectnessTest() : queue_() {
    for (size_t i = 0; i < TestDataSize; i++) {
      test_data_.at(i) = TestData<T>::generate();
    }
  }
  ~CorrectnessTest() = default;

 public:
  auto operator()() {
    spdlog::info("Queue Value Type: {}, TestDataSize: {}", toolbox::util::typenameOf<T>(),
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
      queue_.push(e);
    }
  }

  auto consume() -> void {
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
using UnboundedSPSCQueue = toolbox::container::UnboundedSPSCQueue<T>;

TEST(UnboundedSPSCQueue, CorrectnessTest) {
  TestRunner<CorrectnessTest<UnboundedSPSCQueue<uint64_t>, 1 << 24>>().Run();
  TestRunner<CorrectnessTest<UnboundedSPSCQueue<std::string>, 1 << 22>>().Run();
}

TEST(UnboundedSPSCQueue, DtorTest) {
  {
    UnboundedSPSCQueue<DtorCounter> q;
    for (int i = 0; i < 10; ++i) {
      q.push(DtorCounter());
    }
    EXPECT_EQ(DtorCounter::get(), 11);
    {
      DtorCounter dummy;
      EXPECT_TRUE(q.pop(dummy));
      EXPECT_TRUE(q.pop(dummy));
    }
    EXPECT_EQ(DtorCounter::get(), 11);
  }
  EXPECT_EQ(DtorCounter::get(), 0);
  {
    UnboundedSPSCQueue<DtorCounter> q;
    for (int i = 0; i < 3; ++i) {
      q.push(DtorCounter());
    }
    EXPECT_EQ(DtorCounter::get(), 4);
    {
      DtorCounter dummy;
      EXPECT_TRUE(q.pop(dummy));
    }
    EXPECT_EQ(DtorCounter::get(), 4);
    q.push(DtorCounter());
    EXPECT_EQ(DtorCounter::get(), 4);
    q.push(DtorCounter());
    EXPECT_EQ(DtorCounter::get(), 5);
  }
  EXPECT_EQ(DtorCounter::get(), 0);
}
