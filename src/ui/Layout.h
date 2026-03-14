#pragma once
#include <stdint.h>

struct RectI {
  int16_t x, y, w, h;
};

namespace Layout {
  // Full display: 240 x 240
  //
  //  ┌──────────────────────┐  y=0
  //  │       TOPBAR         │  h=30
  //  ├──────────────────────┤  y=30
  //  │                      │
  //  │        CANVAS        │  h=180
  //  │                      │
  //  ├──────────────────────┤  y=210
  //  │       STATUS         │  h=30
  //  └──────────────────────┘  y=240

  static constexpr RectI TOPBAR { 0,   0, 240,  30 };
  static constexpr RectI CANVAS { 0,  30, 240, 180 };
  static constexpr RectI STATUS { 0, 210, 240,  30 };
}