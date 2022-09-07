#pragma once

#include <emmintrin.h>

namespace toolbox {
namespace misc {

inline auto pause() -> void { _mm_pause(); }

}  // namespace misc
}  // namespace toolbox