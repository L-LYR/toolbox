#include "test_util.hh"

template <typename T>
using UnboundedSPSC = toolbox::container::UnboundedSPSCQueue<T>;

TEST(UnboundedSPSCQueue, SPSCCorrectnessTest) {
  UnboundedSPSC<uint64_t> iq;
  RunSPSCCorrectnessTest(iq, 1 << 20);
  UnboundedSPSC<std::string> sq;
  RunSPSCCorrectnessTest(sq, 1 << 20);
}

TEST(UnboundedSPSCQueue, DtorTest) {
  {
    UnboundedSPSC<DtorCounter> q;
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
    UnboundedSPSC<DtorCounter> q;
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
