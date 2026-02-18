#include "ui/BootScreen.h"
#include "drivers/Display.h"
#include "ui/Layout.h"


void BootScreen::begin(Display& d) {
  d.clear();
  d.textInBox(Layout::TOPBAR.x, Layout::TOPBAR.y,
              Layout::TOPBAR.w, Layout::TOPBAR.h,
              1, 255, 255, 255, "BOOT");

  d.textInBox(Layout::STATUS.x, Layout::STATUS.y,
              Layout::STATUS.w, Layout::STATUS.h,
              1, 255, 255, 255, "READY");
}

void BootScreen::update(Display&) {}
