#include "util/statistics.hh"

#include <gtest/gtest.h>

#include <random>

constexpr static uint32_t N = 256;
std::array<uint64_t, N> test_data{
    103, 70,  105, 115, 81, 127, 74,  108, 41,  77,  58,  43,  114, 123, 99,  70,  124, 66,  84,
    120, 27,  104, 103, 13, 118, 90,  46,  99,  51,  31,  73,  26,  102, 50,  13,  55,  49,  88,
    35,  90,  37,  93,  5,  23,  88,  105, 94,  84,  43,  50,  77,  70,  27,  52,  84,  17,  14,
    2,   116, 65,  33,  61, 92,  7,   112, 105, 62,  33,  65,  97,  124, 103, 62,  1,   126, 23,
    106, 92,  107, 22,  15, 56,  92,  42,  108, 48,  59,  123, 50,  47,  60,  84,  108, 24,  91,
    92,  2,   26,  126, 67, 123, 122, 42,  58,  123, 41,  81,  102, 5,   60,  124, 20,  117, 88,
    62,  97,  9,   121, 92, 59,  40,  25,  15,  21,  49,  107, 113, 51,  5,   111, 119, 0,   105,
    33,  58,  101, 74,  11, 75,  80,  72,  71,  100, 61,  31,  35,  30,  40,  28,  123, 100, 69,
    20,  115, 90,  69,  94, 75,  121, 99,  59,  112, 100, 36,  17,  30,  9,   92,  42,  84,  44,
    114, 27,  16,  47,  59, 51,  77,  99,  80,  72,  71,  21,  92,  59,  111, 34,  25,  58,  27,
    125, 117, 11,  97,  26, 28,  127, 35,  120, 41,  120, 36,  27,  19,  53,  74,  78,  104, 24,
    50,  56,  96,  121, 77, 61,  52,  60,  95,  78,  119, 122, 75,  108, 5,   44,  6,   33,  43,
    42,  26,  85,  34,  62, 112, 53,  115, 59,  4,   92,  83,  54,  20,  51,  47,  98,  112, 100,
    30,  79,  50,  21,  73, 125, 2,   78,  41,
};

TEST(LatencyStatistic, AllMethodTest) {
  toolbox::util::AccStatistics<128> statistics;

  std::sort(test_data.begin(), test_data.end());

  std::cout << "test data: " << std::endl;
  auto fn = [i = 0, &statistics](uint64_t n) mutable -> void {
    statistics.record(n);
    std::cout << " " << n;
    if (++i >= 16) {
      std::cout << std::endl;
      i = 0;
    }
  };
  std::for_each(test_data.begin(), test_data.end(), fn);

  std::cout << statistics.dump();

  double expected_sum = std::accumulate(test_data.begin(), test_data.end(), 0UL);
  auto [expected_min, expected_max] = std::minmax_element(test_data.begin(), test_data.end());

  EXPECT_EQ(statistics.count(), N);
  EXPECT_EQ(statistics.sum(), expected_sum);
  EXPECT_EQ(statistics.avg(), expected_sum / N);
  EXPECT_EQ(statistics.min(), *expected_min);
  EXPECT_EQ(statistics.max(), *expected_max);
  EXPECT_EQ(statistics.percentile(0.5), 67);
  EXPECT_EQ(statistics.percentile(0.9), 117);
  EXPECT_EQ(statistics.percentile(0.95), 123);
  EXPECT_EQ(statistics.percentile(0.99), 126);
  EXPECT_EQ(statistics.percentile(0.999), 127);
}