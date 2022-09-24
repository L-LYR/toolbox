#include <benchmark/benchmark.h>

#include "dummy_queue.hh"
#include "queue/mpmc.hh"

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

template <typename Queue>
class MPMCBench : public ::benchmark::Fixture {
  using ValueType = typename Queue::ValueType;
  using ConsumerType = typename Queue::ConsumerType;
  using ProducerType = typename Queue::ProducerType;

 public:
  auto consumer(::benchmark::State& s) -> void {
    auto c = q_.consumer();
    while (s.KeepRunningBatch(s.threads() >> 1)) {
      ValueType src;
      while (not c.pop(src)) {
        toolbox::misc::pause();
      }
    }
  }

  auto producer(::benchmark::State& s) -> void {
    auto p = q_.producer();
    ValueType dst = TestData<ValueType>::generate();
    while (s.KeepRunningBatch(s.threads() >> 1)) {
      while (not p.push(dst)) {
        toolbox::misc::pause();
      }
    }
  }

 public:
  Queue q_{};
  std::atomic_int ready_{0};
};

using MPMCMode =
    toolbox::container::Queue<uint64_t, 1024, toolbox::container::QueueMode::MPMC>;
using MPMC_HTSMode =
    toolbox::container::Queue<uint64_t, 1024, toolbox::container::QueueMode::MPMC_HTS>;
using Dummy = DummyQueue<uint64_t>;

#define BenchName(q) #q "Bench"
#define Bench(q)                                              \
  namespace q##Bench {                                        \
    using Benchmark = MPMCBench<q>;                           \
    BENCHMARK_DEFINE_F(Benchmark, Run)                        \
    (::benchmark::State & s) {                                \
      if (s.thread_index() % 2 == 0) {                        \
        consumer(s);                                          \
      } else {                                                \
        producer(s);                                          \
      }                                                       \
    }                                                         \
    BENCHMARK_REGISTER_F(Benchmark, Run)                      \
        ->Name(BenchName(q))                                  \
        ->Iterations(1 << 24)                                 \
        ->ThreadRange(2, std::thread::hardware_concurrency()) \
        ->UseRealTime();                                      \
  }

Bench(MPMCMode);
Bench(MPMC_HTSMode);