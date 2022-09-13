#pragma once

#include <array>
#include <new>

#include "descriptor.hh"
#include "util/align.hh"
#include "util/asm.hh"
#include "util/math.hh"

namespace toolbox {
namespace container {

/// This Queue is borrowed from DPDK.

enum QueueMode {
  SPSC,
  MPMC,
  MPMC_HTS,
  MPMC_RTS,
};

template <typename T, uint32_t Size, QueueMode Mode>
class Queue {
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

 public:
  using ValueType = T;
  using HandleType =
      std::conditional_t<Mode == MPMC or Mode == SPSC, Handle,
                         std::conditional_t<Mode == MPMC_HTS, HTSHandle, RTSHandle>>;
  using QueueType = Queue<ValueType, Size, Mode>;
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
        counter_(isSPSC() ? 1 : -1, isSPSC() ? 1 : -1),
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
  static constexpr auto isSPSC() -> bool { return Mode == SPSC; }

 public:
  auto producer() -> ProducerType { return ProducerType(*this); }
  auto consumer() -> ConsumerType { return ConsumerType(*this); }

 public:
  template <typename... Args>
  auto push(Args&&... args) -> bool {
    uint32_t producer_new_head;
    uint32_t n_free;

    const uint32_t capacity = capacity_;
    uint32_t producer_old_head = producer_handle_.head_.load(std::memory_order_consume);
    // move producer head
    auto ok = true;
    do {
      n_free = capacity + consumer_handle_.tail_.load(std::memory_order_consume) -
               producer_old_head;
      if (n_free < 1) return false;
      producer_new_head = producer_old_head + 1;
      if constexpr (isSPSC()) {
        producer_handle_.head_ = producer_new_head;
      } else {
        ok = producer_handle_.head_.compare_exchange_strong(
            producer_old_head, producer_new_head, std::memory_order_relaxed,
            std::memory_order_relaxed);
      }
    } while (not ok);
    // place element
    new (&elems_[producer_old_head & mask_]) ValueType(std::forward<Args>(args)...);

    // wait and update producer tail
    if constexpr (not isSPSC()) {
      while (producer_handle_.tail_.load(std::memory_order_relaxed) !=
             producer_old_head) {
        misc::pause();
      }
    }

    producer_handle_.tail_.store(producer_new_head, std::memory_order_release);
    return true;
  }

  auto pop(ValueType& e) -> bool {
    uint32_t consumer_old_head;
    uint32_t consumer_new_head;

    // move consumer head
    consumer_old_head = consumer_handle_.head_.load(std::memory_order_consume);
    auto ok = true;
    do {
      auto n_remain =
          producer_handle_.tail_.load(std::memory_order_consume) - consumer_old_head;
      if (n_remain < 1) return false;
      consumer_new_head = consumer_old_head + 1;
      if constexpr (isSPSC()) {
        consumer_handle_.head_ = consumer_new_head;
      } else {
        ok = consumer_handle_.head_.compare_exchange_strong(
            consumer_old_head, consumer_new_head, std::memory_order_relaxed,
            std::memory_order_relaxed);
      }
    } while (not ok);

    // move element
    uint32_t idx = consumer_old_head & mask_;
    e = std::move(elems_[idx]);
    elems_[idx].~ValueType();
    
    if constexpr (not isSPSC()) {
      // wait and update consumer tail
      while (consumer_handle_.tail_.load(std::memory_order_relaxed) !=
             consumer_old_head) {
        misc::pause();
      }
    }

    consumer_handle_.tail_.store(consumer_new_head, std::memory_order_release);
    return true;
  }

 private:
  alignas(util::cache_line_size) HandleType producer_handle_;
  [[gnu::unused]] char _pad1_[util::cache_line_size - sizeof(HandleType)];

  alignas(util::cache_line_size) HandleType consumer_handle_;
  [[gnu::unused]] char _pad2_[util::cache_line_size - sizeof(HandleType)];

  uint32_t size_;
  uint32_t mask_;
  uint32_t capacity_;
  DescriptorCounter counter_;

  ValueType* const elems_;
};

}  // namespace container
}  // namespace toolbox