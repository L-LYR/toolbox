#pragma once
#include <mutex>
#include <queue>

#include "queue/descriptor.hh"

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
  toolbox::container::DescriptorCounter counter_{-1, -1};
};