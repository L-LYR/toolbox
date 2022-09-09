#include "test_util.hh"

template <typename T, uint32_t Size>
using MPMCQueue = toolbox::container::Queue<T, Size, toolbox::container::QueueMode::MPMC>;

TEST(MPMCQueue, SPSCCorrectnessTest) {
  MPMCQueue<uint64_t, 1024> iq;
  for (uint32_t i = 1; i <= std::thread::hardware_concurrency(); i++) {
    RunMPMCCorrectnessTest(iq, i, 1 << 20);
  }
}

TEST(MPMCQueue, DtorTest) {
  {
    MPMCQueue<DtorCounter, 1023> q;
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
    MPMCQueue<DtorCounter, 1023> q;
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
