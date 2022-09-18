#pragma once

#if defined(__x86_64__)

#include <emmintrin.h>
#define PAUSE _mm_pause()

#elif defined(__aarch64__)

#define PAUSE asm volatile("yield" ::: "memory")

#endif

namespace toolbox {
namespace misc {

inline auto pause() -> void { PAUSE; }

}  // namespace misc
}  // namespace toolbox

#undef PAUSE