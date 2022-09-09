#include "test_util.hh"

template <typename T>
using UnboundedSPSCQueue = toolbox::container::UnboundedSPSCQueue<T>;

TEST(UnboundedSPSCQueue, SPSCCorrectnessTest) {
  UnboundedSPSCQueue<uint64_t> iq;
  std::make_shared<SPSCCorrectnessTest<UnboundedSPSCQueue<uint64_t>, 1 << 20>>(iq);
  UnboundedSPSCQueue<std::string> sq;
  std::make_shared<SPSCCorrectnessTest<UnboundedSPSCQueue<std::string>, 1 << 20>>(sq);
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
