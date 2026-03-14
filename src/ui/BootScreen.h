#pragma once

#include "ui/Screen.h"

class Display;

class BootScreen : public Screen {
public:
  void begin(Display& display) override;
  void update(Display& display) override;

  // Left side, top row
  void setModeText(const char* text);

  // Left side, bottom row
  void setIpText(const char* text);

  // Controls whether the right-side hotspot info is shown
  void setShowApInfo(bool show);

  // Right side, top row
  void setApSsidText(const char* text);

  // Right side, bottom row
  void setApPasswordText(const char* text);

private:
  // Draws the full bottom status area
  void drawStatusArea_(Display& d);

  // Stored text for the bottom-left column
  char modeText_[40] = "Mode: Startup";
  char ipText_[40]   = "IP: waiting...";

  // Stored text for the bottom-right column
  char apSsidText_[40] = "SSID: ---";
  char apPassText_[40] = "PASS: ---";

  // If true, show the right-side hotspot info
  bool showApInfo_ = false;

  // Redraw the status area only when something changes
  bool statusDirty_ = true;
};