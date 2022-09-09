#pragma once

#include <new>
namespace toolbox {
namespace util {

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_constructive_interference_size = 64;
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

constexpr std::size_t cache_line_size = hardware_destructive_interference_size;

}  // namespace util
}  // namespace toolbox