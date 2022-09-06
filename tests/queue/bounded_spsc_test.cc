#include "test_util.hh"

template <typename T>
using BoundedSPSCQueue = toolbox::container::BoundedSPSCQueue<T>;

TEST(BoundedSPSCQueue, CorrectnessTest) {
  BoundedSPSCQueue<uint64_t> iq(65535);
  TestRunner<CorrectnessTest<BoundedSPSCQueue<uint64_t>, 1 << 24>>(iq).Run();
  BoundedSPSCQueue<std::string> sq(65535);
  TestRunner<CorrectnessTest<BoundedSPSCQueue<std::string>, 1 << 24>>(sq).Run();
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