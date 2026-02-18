#pragma once

#include "ui/Screen.h"

class Display;

class BootScreen : public Screen {
public:
  void begin(Display& display) override;
  void update(Display& display) override;
};
