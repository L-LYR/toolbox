#include <benchmark/benchmark.h>

#include <mutex>
#include <queue>

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

template <typename T>
class DummyQueue {
 public:
  using ValueType = T;
  using QueueType = DummyQueue<ValueType>;
  using ProducerType = toolbox::container::QueueProducer<QueueType>;
  using ConsumerType = toolbox::container::QueueConsumer<QueueType>;

 public:
  friend class toolbox::container::QueueProducer<QueueType>;
  friend class toolbox::container::QueueConsumer<QueueType>;

 public:
  auto producer() -> ProducerType { return ProducerType(*this); }
  auto consumer() -> ConsumerType { return ConsumerType(*this); }

 public:
  template <typename... Args>
  auto push(Args... args) -> bool {
    if (not mu_.try_lock()) {
      return false;
    }
    q_.emplace(args...);
    mu_.unlock();
    return true;
  }

  auto pop(ValueType& t) -> bool {
    if (not mu_.try_lock()) {
      return false;
    }
    if (q_.empty()) {
      mu_.unlock();
      return false;
    }
    t = std::move(q_.front());
    q_.pop();
    mu_.unlock();
    return true;
  }

 public:
  DummyQueue() = default;
  ~DummyQueue() = default;

 public:
 private:
  std::mutex mu_;
  std::queue<T> q_;
  toolbox::container::DescriptorCounter counter_{1, 1};
};

using BoundedSPSC = toolbox::container::BoundedSPSCQueue<uint64_t, 1024>;
using UnboundedSPSC = toolbox::container::UnboundedSPSCQueue<uint64_t>;
using SPSCMode =
    toolbox::container::Queue<uint64_t, 1024, toolbox::container::QueueMode::SPSC>;
using MPMCMode =
    toolbox::container::Queue<uint64_t, 1024, toolbox::container::QueueMode::MPMC>;
using MPMC_HTSMode =
    toolbox::container::Queue<uint64_t, 1024, toolbox::container::QueueMode::MPMC_HTS>;
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
Bench(Dummy);