#pragma once

#include <atomic>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <utility>

#include "util/align.hh"
#include "util/marker.hh"

namespace toolbox {
namespace container {

template <typename T>
class BoundedSPSCQueue : public util::Noncopyable, public util::Nonmovable {
 public:
  using ValueType = T;

 public:
  explicit BoundedSPSCQueue(uint32_t capacity)
      : size_(capacity + 1),
        elems_(static_cast<T*>(std::malloc(sizeof(T) * size_))),
        read_idx_(0),
        write_idx_(0) {
    if (size_ < 2) {
      throw std::runtime_error("size of queue is too small");
    }
    if (elems_ == nullptr) {
      throw std::bad_alloc();
    }
  }
  ~BoundedSPSCQueue() {
    if (not std::is_trivially_destructible<T>()) {
      size_t idx = read_idx_;
      size_t end = write_idx_;
      while (idx != end) {
        elems_[idx].~T();
        if (++idx == size_) {
          idx = 0;
        }
      }
    }
    std::free(elems_);
  }

 public:
  template <typename... Args>
  auto push(Args&&... args) -> bool {
    auto const cur_write = write_idx_.load(std::memory_order_relaxed);
    auto next = cur_write + 1;
    if (next == size_) {
      next = 0;
    }
    if (next != read_idx_.load(std::memory_order_acquire)) {
      new (&elems_[cur_write]) T(std::forward<Args>(args)...);
      write_idx_.store(next, std::memory_order_release);
      return true;
    }
    return false;
  }

  auto pop(T& e) -> bool {
    auto const cur_read = read_idx_.load(std::memory_order_relaxed);
    if (cur_read == write_idx_.load(std::memory_order_acquire)) {
      return false;
    }
    auto next = cur_read + 1;
    if (next == size_) {
      next = 0;
    }
    e = std::move(elems_[cur_read]);
    elems_[next].~T();
    read_idx_.store(next, std::memory_order_release);
    return true;
  }

  auto front() -> T* {
    auto const cur_read = read_idx_.load(std::memory_order_relaxed);
    if (cur_read == write_idx_.load(std::memory_order_acquire)) {
      return nullptr;
    }
    return &elems_[cur_read];
  }

  auto pop() -> void {
    auto const cur_read = read_idx_.load(std::memory_order_relaxed);
    if (cur_read == write_idx_.load(std::memory_order_acquire)) {
      throw std::runtime_error("meet empty queue");
    }
    auto next = cur_read + 1;
    if (next == size_) {
      next = 0;
    }
    elems_[cur_read].~T();
    read_idx_.store(next, std::memory_order_release);
  }

 public:
  auto isEmpty() const -> bool {
    return read_idx_.load(std::memory_order_acquire) == write_idx_.load(std::memory_order_acquire);
  }

  auto isFull() const -> bool {
    auto next = write_idx_.load(std::memory_order_acquire) + 1;
    if (next == size_) {
      next = 0;
    }
    return next == read_idx_.load(std::memory_order_acquire);
  }

  auto approximateSize() const -> size_t {
    int ret =
        write_idx_.load(std::memory_order_acquire) - read_idx_.load(std::memory_order_acquire);
    if (ret < 0) {
      ret += size_;
    }
    return ret;
  }

  auto capacity() const -> size_t { return size_ - 1; }

 private:
  [[gnu::unused]] char _head_pad_[util::cache_line_size];

  const uint32_t size_;
  T* const elems_;
  alignas(util::cache_line_size) std::atomic_uint64_t read_idx_;
  alignas(util::cache_line_size) std::atomic_uint64_t write_idx_;

  [[gnu::unused]] char _tail_pad_[util::cache_line_size - sizeof(std::atomic_uint64_t)];
};

template <typename T>
class UnboundedSPSCQueue : public util::Noncopyable, public util::Nonmovable {
 public:
  using ValueType = T;

 private:
  class Node {
   public:
    Node() : next_(nullptr) {}

    template <typename... Args>
    Node(Args&&... args) : next_(nullptr), data_(std::forward<Args>(args)...) {}

    ~Node() = default;

   public:
    std::atomic<Node*> next_;
    T data_;
  };

 public:
  UnboundedSPSCQueue() : head_(new Node), tail_(head_), unused_(head_), head_copy_(head_) {}
  ~UnboundedSPSCQueue() {
    Node* p = unused_;
    while (p != nullptr) {
      Node* next = p->next_;
      delete p;
      p = next;
    }
  };

 public:
  template <typename... Args>
  auto push(Args&&... args) -> bool {
    auto n = newNode(std::forward<Args>(args)...);
    tail_->next_.store(n, std::memory_order_release);
    tail_ = n;
    size_++;
    return true;
  }

  auto pop(T& e) -> bool {
    auto head_next = head_.load(std::memory_order_relaxed)->next_.load(std::memory_order_acquire);
    if (head_next != nullptr) {
      e = std::move(head_next->data_);
      head_.store(head_next, std::memory_order_release);
      size_--;
      return true;
    }
    return false;
  }

 private:
  template <typename... Args>
  auto newNode(Args&&... args) -> Node* {
    Node* n;
    if (unused_ != head_copy_) {
      n = unused_;
      unused_ = unused_->next_.load(std::memory_order_relaxed);
      n->data_.~T();
      new (n) Node(std::forward<Args>(args)...);
      return n;
    }
    head_copy_ = head_.load(std::memory_order_acquire);
    if (unused_ != head_copy_) {
      n = unused_;
      unused_ = unused_->next_.load(std::memory_order_relaxed);
      n->data_.~T();
      new (n) Node(std::forward<Args>(args)...);
      return n;
    }
    return new Node(std::forward<Args>(args)...);
  }

 public:
  auto approximateSize() const -> size_t { return size_; }

 private:
  std::atomic<Node*> head_;
  [[gnu::unused]] char _pad_[util::cache_line_size];
  Node* tail_;
  Node* unused_;
  Node* head_copy_;  // between tail_ and unused_tail_

  std::atomic_uint32_t size_;
};

}  // namespace container
}  // namespace toolbox