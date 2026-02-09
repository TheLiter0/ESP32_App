#pragma once
#include <stdint.h>

struct RectI {
  int16_t x, y, w, h;
};

namespace Layout {
  // Active coordinate space confirmed by your tests: 160(w) x 128(h)
  // Top bar: 0..19
  // Canvas: 20..107
  // Status: 108..127

  static constexpr RectI TOPBAR { 0,   0, 160, 20 };
  static constexpr RectI CANVAS { 0,  20, 160, 88 };
  static constexpr RectI STATUS { 0, 108, 160, 20 };
}
