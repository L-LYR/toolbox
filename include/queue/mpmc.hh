#pragma once

#include <array>
#include <new>

#include "descriptor.hh"
#include "util/align.hh"
#include "util/asm.hh"
#include "util/math.hh"

namespace toolbox {
namespace container {

class Handle {
 public:
  Handle() : head_(0), tail_(0) {}
  ~Handle() = default;

 public:
  std::atomic_uint32_t head_;
  std::atomic_uint32_t tail_;
};

/// TODO: waiting for dev(copy)
class RTSHandle {};
class HTSHandle {};

template <typename T, typename Handle, uint32_t Size>
class Queue {
 public:
  using ValueType = T;
  using HandleType = Handle;
  using QueueType = Queue<ValueType, HandleType, Size>;
  using ProducerType = QueueProducer<QueueType>;
  using ConsumerType = QueueConsumer<QueueType>;

 public:
  friend class QueueProducer<QueueType>;
  friend class QueueConsumer<QueueType>;

 public:
  Queue()
      : size_(misc::alignUpPowerOf2(Size)),
        mask_(size_ - 1),
        capacity_(Size),
        counter_(-1, -1),
        elems_(static_cast<ValueType*>(std::malloc(sizeof(ValueType) * size_))) {
    if (elems_ == nullptr) {
      throw std::bad_alloc();
    }
  }
  ~Queue() {
    if (not std::is_trivially_destructible<ValueType>()) {
      size_t idx = consumer_handle_.head_;
      size_t end = producer_handle_.head_;
      while (idx != end) {
        elems_[idx].~ValueType();
        if (++idx == size_) {
          idx = 0;
        }
      }
    }
    std::free(elems_);
  }

 public:
  auto producer() -> ProducerType { return ProducerType(*this); }
  auto consumer() -> ConsumerType { return ConsumerType(*this); }

 public:
  template <typename... Args>
  auto push(Args&&... args) -> bool {
    uint32_t producer_new_head;
    uint32_t consumer_tail;
    uint32_t n_free;

    const uint32_t capacity = capacity_;
    uint32_t producer_old_head = producer_handle_.head_.load(std::memory_order_relaxed);
    // move producer head
    do {
      std::atomic_thread_fence(std::memory_order_acquire);

      consumer_tail = consumer_handle_.tail_.load(std::memory_order_acquire);
      n_free = capacity + consumer_tail - producer_old_head;
      if (n_free < 1) return false;
      producer_new_head = producer_old_head + 1;

    } while (not producer_handle_.head_.compare_exchange_strong(
        producer_old_head, producer_new_head, std::memory_order_relaxed,
        std::memory_order_relaxed));
    // place element
    new (&elems_[producer_old_head & mask_]) ValueType(std::forward<Args>(args)...);

    // wait and update producer tail
    while (producer_handle_.tail_.load(std::memory_order_relaxed) != producer_old_head) {
      misc::pause();
    }

    producer_handle_.tail_.store(producer_new_head, std::memory_order_release);
    return true;
  }

  auto pop(ValueType& e) -> bool {
    uint32_t consumer_old_head;
    uint32_t consumer_new_head;

    // move consumer head
    consumer_old_head = consumer_handle_.head_.load(std::memory_order_relaxed);
    do {
      std::atomic_thread_fence(std::memory_order_acquire);

      auto n_remain =
          producer_handle_.tail_.load(std::memory_order_acquire) - consumer_old_head;
      if (n_remain < 1) return false;
      consumer_new_head = consumer_old_head + 1;

    } while (not consumer_handle_.head_.compare_exchange_strong(
        consumer_old_head, consumer_new_head, std::memory_order_relaxed,
        std::memory_order_relaxed));

    // move element
    uint32_t idx = consumer_old_head & mask_;
    e = std::move(elems_[idx]);
    elems_[idx].~ValueType();

    // wait and update consumer tail
    while (consumer_handle_.tail_.load(std::memory_order_relaxed) != consumer_old_head) {
      misc::pause();
    }
    consumer_handle_.tail_.store(consumer_new_head, std::memory_order_release);
    return true;
  }

 private:
  uint32_t size_;
  uint32_t mask_;
  uint32_t capacity_;
  DescriptorCounter counter_;

  alignas(util::cache_line_size) [[gnu::unused]] char _pad1_[util::cache_line_size];

  alignas(util::cache_line_size) HandleType producer_handle_;

  alignas(util::cache_line_size) [[gnu::unused]] char _pad2_[util::cache_line_size];

  alignas(util::cache_line_size) HandleType consumer_handle_;

  alignas(util::cache_line_size) [[gnu::unused]] char _pad3_[util::cache_line_size];

  ValueType* const elems_;
};

}  // namespace container
}  // namespace toolbox