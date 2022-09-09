#include <benchmark/benchmark.h>

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
      while (not c_.pop(got))
        ;
    }
  }
  auto producer(::benchmark::State& s) -> void {
    while (s.KeepRunning()) {
      ValueType e = 0xF0F0F0F0;
      while (not p_.push(e))
        ;
    }
  }

 private:
  Queue q_{};
  ConsumerType c_{q_};
  ProducerType p_{q_};
};

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
        ->Iterations(1 << 24)            \
        ->Threads(2)                     \
        ->UseRealTime();                 \
  }

using Queue1 = toolbox::container::BoundedSPSCQueue<uint64_t, 1024>;
Bench(Queue1);
using Queue2 = toolbox::container::UnboundedSPSCQueue<uint64_t>;
Bench(Queue2);
using Queue3 =
    toolbox::container::Queue<uint64_t, 1024, toolbox::container::QueueMode::SPSC>;
Bench(Queue3);