#include "queue/mpmc.hh"

#include "test_util.hh"

template <typename T, uint32_t Size>
using MPMCQueue = toolbox::container::Queue<T, toolbox::container::Handle, Size>;

TEST(MPMCQueue, SPSCCorrectnessTest) {
  MPMCQueue<uint64_t, 65535> iq;
  MPMCQueue<std::string, 65535> sq;
  std::make_shared<SPSCCorrectnessTest<MPMCQueue<uint64_t, 65535>, 1 << 24>>(iq);
  std::make_shared<SPSCCorrectnessTest<MPMCQueue<std::string, 65535>, 1 << 24>>(sq);
  for (uint32_t i = 1; i <= std::thread::hardware_concurrency(); i++) {
    std::make_shared<MPMCCorrectnessTest<MPMCQueue<uint64_t, 65535>>>(iq, i, 1 << 24);
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
