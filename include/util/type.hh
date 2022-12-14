#pragma once

#include <cxxabi.h>
#include <fmt/core.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>

namespace toolbox {
namespace util {

template <typename T>
auto TypenameOf() -> std::string {
  int status = 0;
  std::unique_ptr<char, decltype(free)*> demangled_name(
      abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status), free);
  if (status != 0) {
    throw std::runtime_error(
        fmt::format("fail to demangle type name, status: {}", status));
  }
  return demangled_name.get();
}

}  // namespace util
}  // namespace toolbox