#pragma once

#include <atomic>
#include <stdexcept>

namespace toolbox {
namespace container {

class DescriptorCounter {
 public:
  DescriptorCounter(uint32_t max_producer, uint32_t max_consumer)
      : n_producer_(0), n_consumer_(0), max_producer_(max_producer), max_consumer_(max_consumer) {}
  ~DescriptorCounter() = default;

 public:
  std::atomic_uint32_t n_producer_;
  std::atomic_uint32_t n_consumer_;
  uint32_t max_producer_;
  uint32_t max_consumer_;
};

template <typename Queue>
class QueueProducer {
 public:
  using ValueType = typename Queue::ValueType;

 public:
  explicit QueueProducer(Queue& q) : q_(q) {
    if (q_.counter_.n_producer_ == q.counter_.max_producer_) {
      throw std::runtime_error("too many producers");
    }
    q_.counter_.n_producer_++;
  }
  ~QueueProducer() { q_.counter_.n_producer_--; }

 public:
  template <typename... Args>
  auto push(Args&&... args) -> bool {
    return q_.push(std::forward<Args>(args)...);
  }

 private:
  Queue& q_;
};

template <typename Queue>
class QueueConsumer {
 public:
  using ValueType = typename Queue::ValueType;

 public:
  explicit QueueConsumer(Queue& q) : q_(q) {
    if (q_.counter_.n_producer_ == q.counter_.max_consumer_) {
      throw std::runtime_error("too many consumers");
    }
    q_.counter_.n_consumer_++;
  }
  ~QueueConsumer() { q_.counter_.n_consumer_--; }

 public:
  auto pop(ValueType& e) -> bool { return q_.pop(e); }

 private:
  Queue& q_;
};

}  // namespace container
}  // namespace toolbox