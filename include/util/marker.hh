#pragma once

namespace toolbox {
namespace util {

class Noncopyable {
 protected:
  Noncopyable() noexcept = default;
  ~Noncopyable() noexcept = default;

 private:
  Noncopyable(const Noncopyable&) = delete;
  auto operator=(const Noncopyable&) -> Noncopyable& = delete;
};

class Nonmovable {
 protected:
  Nonmovable() noexcept = default;
  ~Nonmovable() noexcept = default;

 private:
  Nonmovable(Noncopyable&&) = delete;
  auto operator=(Noncopyable&&) -> Noncopyable& = delete;
};

}  // namespace util
}  // namespace toolbox