#pragma once

#include "ui/Screen.h"

class Display;

class ScreenManager {
public:
  void begin(Display& display);
  void set(Screen* s, Display& display);
  void update(Display& display);

private:
  Screen* current_ = nullptr;
};
