#include "queue/mpmc.hh"

#include "test_util.hh"

#define MPMCQueue(Mode) Mode##Queue

#define MPMCTest(Mode)                                                         \
  template <typename T, uint32_t Size>                                         \
  using MPMCQueue(Mode) =                                                      \
      toolbox::container::Queue<T, Size, toolbox::container::QueueMode::Mode>; \
  TEST(MPMCQueue(Mode), MPMCCorrectnessTest) {                                 \
    MPMCQueue(Mode)<uint64_t, 1024> iq;                                        \
    for (uint32_t i = 1; i <= std::thread::hardware_concurrency(); i++) {      \
      RunMPMCCorrectnessTest(iq, i, 1 << 20);                                  \
    }                                                                          \
  }                                                                            \
  TEST(MPMCQueue(Mode), DtorTest) {                                            \
    {                                                                          \
      MPMCQueue(Mode)<DtorCounter, 1023> q;                                    \
      for (int i = 0; i < 10; ++i) {                                           \
        EXPECT_TRUE(q.push(DtorCounter()));                                    \
      }                                                                        \
      EXPECT_EQ(DtorCounter::get(), 10);                                       \
      {                                                                        \
        DtorCounter dummy;                                                     \
        EXPECT_TRUE(q.pop(dummy));                                             \
        EXPECT_TRUE(q.pop(dummy));                                             \
      }                                                                        \
      EXPECT_EQ(DtorCounter::get(), 8);                                        \
    }                                                                          \
    EXPECT_EQ(DtorCounter::get(), 0);                                          \
    {                                                                          \
      MPMCQueue(Mode)<DtorCounter, 1023> q;                                    \
      for (int i = 0; i < 3; ++i) {                                            \
        EXPECT_TRUE(q.push(DtorCounter()));                                    \
      }                                                                        \
      EXPECT_EQ(DtorCounter::get(), 3);                                        \
      {                                                                        \
        DtorCounter dummy;                                                     \
        EXPECT_TRUE(q.pop(dummy));                                             \
      }                                                                        \
      EXPECT_EQ(DtorCounter::get(), 2);                                        \
      EXPECT_TRUE(q.push(DtorCounter()));                                      \
      EXPECT_EQ(DtorCounter::get(), 3);                                        \
    }                                                                          \
    EXPECT_EQ(DtorCounter::get(), 0);                                          \
  }

MPMCTest(MPMC);
MPMCTest(MPMC_HTS);
MPMCTest(MPMC_RTS);
