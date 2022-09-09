#include "test_util.hh"

template <typename T>
using BoundedSPSCQueue = toolbox::container::BoundedSPSCQueue<T>;

template <typename T, uint32_t Size>
using AnotherBoundedSPSCQueue =
    toolbox::container::Queue<T, Size, toolbox::container::QueueMode::SPSC>;

TEST(BoundedSPSCQueue, SPSCCorrectnessTest) {
  BoundedSPSCQueue<uint64_t> iq(1024);
  RunSPSCCorrectnessTest(iq, 1 << 20);
  BoundedSPSCQueue<std::string> sq(1024);
  RunSPSCCorrectnessTest(sq, 1 << 20);
  AnotherBoundedSPSCQueue<uint64_t, 1024> aiq;
  RunSPSCCorrectnessTest(aiq, 1 << 20);
  AnotherBoundedSPSCQueue<std::string, 1024> asq;
  RunSPSCCorrectnessTest(asq, 1 << 20);
}

TEST(BoundedSPSCQueue, DtorTest) {
  {
    BoundedSPSCQueue<DtorCounter> q(1024);
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
    BoundedSPSCQueue<DtorCounter> q(4);
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

TEST(BoundedSPSCQueue, MiscTest) {
  BoundedSPSCQueue<int> queue(2);
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