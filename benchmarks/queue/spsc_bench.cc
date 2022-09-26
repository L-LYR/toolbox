#include <benchmark/benchmark.h>

#include "dummy_queue.hh"
#include "queue/mpmc.hh"
#include "queue/spsc.hh"

template <typename Queue>
class SPSCBench : public ::benchmark::Fixture {
  using ValueType = typename Queue::ValueType;
  using ConsumerType = typename Queue::ConsumerType;
  using ProducerType = typename Queue::ProducerType;

 public:
  auto consumer(::benchmark::State& s) -> void {
    while (s.KeepRunning()) {
      ValueType got;
      while (not c_.pop(got)) {
        toolbox::misc::pause();
      }
    }
  }
  auto producer(::benchmark::State& s) -> void {
    while (s.KeepRunning()) {
      ValueType e = 0xF0F0F0F0;
      while (not p_.push(e)) {
        toolbox::misc::pause();
      }
    }
  }

 private:
  Queue q_{};
  ConsumerType c_{q_};
  ProducerType p_{q_};
};

using BoundedSPSC = toolbox::container::BoundedSPSCQueue<uint64_t, 1024>;
using UnboundedSPSC = toolbox::container::UnboundedSPSCQueue<uint64_t>;
using SPSCMode =
    toolbox::container::Queue<uint64_t, 1024, toolbox::container::QueueMode::SPSC>;
using MPMCMode =
    toolbox::container::Queue<uint64_t, 1024, toolbox::container::QueueMode::MPMC>;
using MPMC_HTSMode =
    toolbox::container::Queue<uint64_t, 1024, toolbox::container::QueueMode::MPMC_HTS>;
using MPMC_RTSMode =
    toolbox::container::Queue<uint64_t, 1024, toolbox::container::QueueMode::MPMC_RTS>;
using Dummy = DummyQueue<uint64_t>;

#define BenchName(q) #q "Bench"
#define Bench(q)                         \
  namespace q##Bench {                   \
    using Benchmark = SPSCBench<q>;      \
    BENCHMARK_DEFINE_F(Benchmark, Run)   \
    (::benchmark::State & s) {           \
      if (s.thread_index() == 0) {       \
        consumer(s);                     \
      } else {                           \
        producer(s);                     \
      }                                  \
    }                                    \
    BENCHMARK_REGISTER_F(Benchmark, Run) \
        ->Name(BenchName(q))             \
        ->Iterations(1 << 24)            \
        ->Threads(2)                     \
        ->UseRealTime();                 \
  }

Bench(BoundedSPSC);
Bench(UnboundedSPSC);
Bench(SPSCMode);
Bench(MPMCMode);
Bench(MPMC_HTSMode);
Bench(MPMC_RTSMode);
Bench(Dummy);