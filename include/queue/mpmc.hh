#pragma once

#include <spdlog/spdlog.h>

#include <array>
#include <atomic>
#include <new>

#include "descriptor.hh"
#include "util/align.hh"
#include "util/math.hh"
#include "util/misc.hh"

namespace toolbox::container {

/// This Queue is borrowed from DPDK.

enum QueueMode {
  SPSC,
  MPMC,
  MPMC_HTS,
  MPMC_RTS,
};

template <typename T, uint32_t Size, QueueMode Mode>
class Queue {
  class [[gnu::packed]] Handle {
   public:
    Handle() : head_(0), tail_(0) {}
    ~Handle() = default;

   public:
    auto head() -> uint32_t { return head_; }
    auto tail() -> uint32_t { return tail_; }

   public:
    std::atomic_uint32_t head_;
    std::atomic_uint32_t tail_;
  };

  class [[gnu::packed]] RawHandle {
   public:
    RawHandle() : head_(0), tail_(0) {}
    ~RawHandle() = default;
    RawHandle(uint64_t x) : head_(x >> 32), tail_(x) {}

   public:
    auto sync() -> bool { return head_ == tail_; }
    auto asUint64() -> uint64_t { return *raw(); }
    auto asUint64Ref() -> uint64_t& { return *raw(); }

   private:
    auto raw() -> uint64_t* { return (uint64_t*)(this); }

   public:
    uint32_t head_;
    uint32_t tail_;
  };

  class [[gnu::packed]] alignas(sizeof(uint64_t)) HTSHandle {
   public:
    HTSHandle() : head_(0), tail_(0) {}
    ~HTSHandle() = default;

   public:
    auto head() -> uint32_t { return head_; }
    auto tail() -> uint32_t { return tail_; }

   public:
    auto load(std::memory_order order = std::memory_order_seq_cst) -> RawHandle {
      return rawAtomic()->load(order);
    }

    auto compareExchangeStrong(uint64_t& expected, uint64_t desired,
                               std::memory_order success = std::memory_order_seq_cst,
                               std::memory_order failure = std::memory_order_seq_cst)
        -> bool {
      return rawAtomic()->compare_exchange_strong(expected, desired, success, failure);
    }

   private:
    auto rawAtomic() -> std::atomic_uint64_t* { return (std::atomic_uint64_t*)(this); }

   public:
    std::atomic_uint32_t head_;
    std::atomic_uint32_t tail_;
  };

  class [[gnu::packed]] alignas(sizeof(uint64_t)) PosRef {
   public:
    PosRef() : pos_(0), ref_(0) {}
    PosRef(uint64_t x) : pos_(x >> 32), ref_(x) {}
    ~PosRef() = default;

   public:
    auto asUint64() -> uint64_t { return *raw(); }
    auto asUint64Ref() -> uint64_t& { return *raw(); }

   public:
    auto load(std::memory_order order = std::memory_order_seq_cst) -> PosRef {
      return rawAtomic()->load(order);
    }

    auto compareExchangeStrong(uint64_t& expected, uint64_t desired,
                               std::memory_order success = std::memory_order_seq_cst,
                               std::memory_order failure = std::memory_order_seq_cst)
        -> bool {
      return rawAtomic()->compare_exchange_strong(expected, desired, success, failure);
    }

   private:
    auto rawAtomic() -> std::atomic_uint64_t* { return (std::atomic_uint64_t*)(this); }
    auto raw() -> uint64_t* { return (uint64_t*)(this); }

   public:
    uint32_t pos_;
    uint32_t ref_;
  };

  constexpr static uint32_t head_tail_dis_max = Size / 8;
  class [[gnu::packed]] RTSHandle {
   public:
    RTSHandle() : head_(), tail_(), dis_max_(head_tail_dis_max) {}
    ~RTSHandle() = default;

   public:
    auto head() -> uint32_t { return head_.pos_; }
    auto tail() -> uint32_t { return tail_.pos_; }

   public:
    PosRef head_;
    PosRef tail_;
    uint32_t dis_max_;
  };

 public:
  using ValueType = T;
  using HandleType = std::conditional_t<
      Mode == MPMC or Mode == SPSC, Handle,
      std::conditional_t<Mode == MPMC_HTS, HTSHandle,
                         std::conditional_t<Mode == MPMC_RTS, RTSHandle, void>>>;
  static_assert(not std::is_same_v<Handle, void>, "unknown queue mode");
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
        elems_(static_cast<ValueType*>(::operator new[](sizeof(ValueType) * size_))) {
    if (elems_ == nullptr) {
      throw std::bad_alloc();
    }
  }
  ~Queue() {
    if (not std::is_trivially_destructible<ValueType>()) {
      size_t idx = consumer_handle_.head();
      size_t end = producer_handle_.head();
      while (idx != end) {
        elems_[idx].~ValueType();
        if (++idx == size_) {
          idx = 0;
        }
      }
    }
    ::operator delete[](elems_);
  }

 public:
  static constexpr auto isSPSC() -> bool { return Mode == SPSC; }

 public:
  auto producer() -> ProducerType { return ProducerType(*this); }
  auto consumer() -> ConsumerType { return ConsumerType(*this); }

 public:
  template <typename... Args, QueueMode mode = Mode>
  auto push(Args&&... args) -> std::enable_if_t<mode == MPMC or mode == SPSC, bool> {
    uint32_t producer_new_head = 0;
    uint32_t n_free = 0;

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

  template <QueueMode mode = Mode>
  auto pop(ValueType& e) -> std::enable_if_t<mode == MPMC or mode == SPSC, bool> {
    uint32_t consumer_new_head = 0;
    uint32_t n_remain = 0;

    // move consumer head
    uint32_t consumer_old_head = consumer_handle_.head_.load(std::memory_order_consume);
    auto ok = true;
    do {
      n_remain =
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

  template <typename... Args, QueueMode mode = Mode>
  auto push(Args&&... args) -> std::enable_if_t<mode == MPMC_HTS, bool> {
    RawHandle np;
    uint32_t n_free = 0;

    auto ok = false;
    const uint32_t capacity = capacity_;
    RawHandle cp = producer_handle_.load(std::memory_order_acquire);
    do {
      while (not cp.sync()) {
        misc::pause();
        cp = producer_handle_.load(std::memory_order_acquire);
      }
      n_free =
          capacity + consumer_handle_.tail_.load(std::memory_order_relaxed) - cp.head_;
      if (n_free < 1) return false;
      np.tail_ = cp.tail_;
      np.head_ = cp.head_ + 1;
      ok = producer_handle_.compareExchangeStrong(cp.asUint64Ref(), np.asUint64(),
                                                  std::memory_order_acquire,
                                                  std::memory_order_acquire);
    } while (not ok);

    new (&elems_[cp.head_ & mask_]) ValueType(std::forward<Args>(args)...);

    producer_handle_.tail_.store(np.head_, std::memory_order_release);
    return true;
  }

  template <QueueMode mode = Mode>
  auto pop(ValueType& e) -> std::enable_if_t<mode == MPMC_HTS, bool> {
    RawHandle np;
    uint32_t n_remain = 0;

    auto ok = false;
    RawHandle cp = consumer_handle_.load(std::memory_order_acquire);
    do {
      while (not cp.sync()) {
        misc::pause();
        cp = consumer_handle_.load(std::memory_order_acquire);
      }
      n_remain = producer_handle_.tail_.load(std::memory_order_relaxed) - cp.head_;
      if (n_remain < 1) return false;
      np.tail_ = cp.tail_;
      np.head_ = cp.head_ + 1;
      ok = consumer_handle_.compareExchangeStrong(cp.asUint64Ref(), np.asUint64(),
                                                  std::memory_order_acquire,
                                                  std::memory_order_acquire);
    } while (not ok);

    uint32_t idx = cp.head_ & mask_;
    e = std::move(elems_[idx]);
    elems_[idx].~ValueType();

    consumer_handle_.tail_.store(np.head_, std::memory_order_release);
    return true;
  }

  template <typename... Args, QueueMode mode = Mode>
  auto push(Args... args) -> std::enable_if_t<mode == MPMC_RTS, bool> {
    PosRef nh;
    uint32_t n_free = 0;

    auto ok = false;
    const uint32_t capacity = capacity_;
    PosRef ch = producer_handle_.head_.load(std::memory_order_acquire);
    do {
      while (ch.pos_ - producer_handle_.tail_.pos_ > producer_handle_.dis_max_) {
        misc::pause();
        ch = producer_handle_.head_.load(std::memory_order_acquire);
      }
      n_free = capacity + consumer_handle_.tail_.pos_ - ch.pos_;
      if (n_free < 1) return false;
      nh.pos_ = ch.pos_ + 1;
      nh.ref_ = ch.ref_ + 1;
      ok = producer_handle_.head_.compareExchangeStrong(ch.asUint64Ref(), nh.asUint64(),
                                                        std::memory_order_acquire,
                                                        std::memory_order_acquire);
    } while (not ok);

    new (&elems_[ch.pos_ & mask_]) ValueType(std::forward<Args>(args)...);

    PosRef nt;
    PosRef h;
    PosRef ct = producer_handle_.tail_.load(std::memory_order_acquire);
    do {
      h = producer_handle_.head_.load(std::memory_order_relaxed);
      nt = ct;
      if (++nt.ref_ == h.ref_) {
        nt.pos_ = h.pos_;
      }
      ok = producer_handle_.tail_.compareExchangeStrong(ct.asUint64Ref(), nt.asUint64(),
                                                        std::memory_order_release,
                                                        std::memory_order_acquire);
    } while (not ok);

    return true;
  }

  template <QueueMode mode = Mode>
  auto pop(ValueType& e) -> std::enable_if_t<mode == MPMC_RTS, bool> {
    PosRef nh;
    uint32_t n_remain = 0;

    auto ok = false;
    PosRef ch = consumer_handle_.head_.load(std::memory_order_acquire);
    do {
      while (ch.pos_ - consumer_handle_.tail_.pos_ > consumer_handle_.dis_max_) {
        misc::pause();
        ch = consumer_handle_.head_.load(std::memory_order_acquire);
      }
      n_remain = producer_handle_.tail_.pos_ - ch.pos_;
      if (n_remain < 1) return false;
      nh.pos_ = ch.pos_ + 1;
      nh.ref_ = ch.ref_ + 1;
      ok = consumer_handle_.head_.compareExchangeStrong(ch.asUint64Ref(), nh.asUint64(),
                                                        std::memory_order_acquire,
                                                        std::memory_order_acquire);
    } while (not ok);

    uint32_t idx = ch.pos_ & mask_;
    e = std::move(elems_[idx]);
    elems_[idx].~ValueType();

    PosRef nt;
    PosRef h;
    PosRef ct = consumer_handle_.tail_.load(std::memory_order_acquire);
    do {
      h = consumer_handle_.head_.load(std::memory_order_relaxed);
      nt = ct;
      if (++nt.ref_ == h.ref_) {
        nt.pos_ = h.pos_;
      }
      ok = consumer_handle_.tail_.compareExchangeStrong(ct.asUint64Ref(), nt.asUint64(),
                                                        std::memory_order_release,
                                                        std::memory_order_acquire);
    } while (not ok);

    return true;
  }

 private:
  alignas(util::cache_line_size) HandleType producer_handle_{};
  alignas(util::cache_line_size) [[gnu::unused]] char _pad1_[util::cache_line_size]{};

  alignas(util::cache_line_size) HandleType consumer_handle_{};
  alignas(util::cache_line_size) [[gnu::unused]] char _pad2_[util::cache_line_size]{};

  uint32_t size_{};
  uint32_t mask_{};
  uint32_t capacity_{};
  DescriptorCounter counter_{-1, -1};

  ValueType* const elems_{nullptr};
};

}  // namespace toolbox::container