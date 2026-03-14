#include "ui/BootScreen.h"
#include "drivers/Display.h"
#include "ui/Layout.h"
#include <cstring>

void BootScreen::begin(Display& d) {
  d.clear();
  
  // Draw the main title / header area once
  d.textInBox(Layout::TOPBAR.x, Layout::TOPBAR.y,
              Layout::TOPBAR.w, Layout::TOPBAR.h,
              1, 0, 200, 255, "ESP32 Screen Control System");

  d.textInBox(Layout::TOPBAR.x, Layout::TOPBAR.y + 10,
              Layout::TOPBAR.w, Layout::TOPBAR.h,
              1, 0, 200, 255, "Home Screen");

  // Draw the bottom status area for the first time
  drawStatusArea_(d);
}

void BootScreen::update(Display& d) {
  // Only redraw the bottom area when one of the text values changed
  if (statusDirty_) {
    drawStatusArea_(d);
  }
}

void BootScreen::setModeText(const char* text) {
  if (!text) return;

  std::strncpy(modeText_, text, sizeof(modeText_) - 1);
  modeText_[sizeof(modeText_) - 1] = '\0';
  statusDirty_ = true;
}

void BootScreen::setIpText(const char* text) {
  if (!text) return;

  std::strncpy(ipText_, text, sizeof(ipText_) - 1);
  ipText_[sizeof(ipText_) - 1] = '\0';
  statusDirty_ = true;
}

void BootScreen::setShowApInfo(bool show) {
  showApInfo_ = show;
  statusDirty_ = true;
}

void BootScreen::setApSsidText(const char* text) {
  if (!text) return;

  std::strncpy(apSsidText_, text, sizeof(apSsidText_) - 1);
  apSsidText_[sizeof(apSsidText_) - 1] = '\0';
  statusDirty_ = true;
}

void BootScreen::setApPasswordText(const char* text) {
  if (!text) return;

  std::strncpy(apPassText_, text, sizeof(apPassText_) - 1);
  apPassText_[sizeof(apPassText_) - 1] = '\0';
  statusDirty_ = true;
}

void BootScreen::drawStatusArea_(Display& d) {
  // Split the bottom status area into 2 columns and 2 rows:
  //
  // Left column:
  //   top    -> mode text
  //   bottom -> IP / AP address
  //
  // Right column:
  //   top    -> SSID
  //   bottom -> PASS
  //
  // In normal WiFi mode, the right column stays blank.

  const int gap = 4;
  const int rowH = Layout::STATUS.h / 2;
  const int colW = (Layout::STATUS.w - gap) / 2;

  const int leftX  = Layout::STATUS.x;
  const int rightX = Layout::STATUS.x + colW + gap;

  const int topY    = Layout::STATUS.y;
  const int bottomY = Layout::STATUS.y + rowH;

  // Clear the full bottom status area before redrawing it
  d.clearRect(Layout::STATUS.x, Layout::STATUS.y,
              Layout::STATUS.w, Layout::STATUS.h);

  // Draw the bottom-left top row: current mode
  d.textInBox(leftX, topY, colW, rowH,
              1, 200, 255, 255, modeText_);

  // Draw the bottom-left bottom row: IP or AP address
  d.textInBox(leftX, bottomY, colW, rowH,
              1, 180, 220, 255, ipText_);

  // Only draw the right-side hotspot details if AP mode is active
  if (showApInfo_) {
    d.textInBox(rightX, topY, colW, rowH,
                1, 255, 220, 120, apSsidText_);

    d.textInBox(rightX, bottomY, colW, rowH,
                1, 255, 220, 120, apPassText_);
  }

  statusDirty_ = false;
}