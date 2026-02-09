#pragma once

#include "src/services/Logger.h"
#include "src/drivers/Display.h"

class SerialConsole {
public:
  void begin(Logger* logger, Display* display) {
    logger_ = logger;
    display_ = display;
  }

  void update();

private:
  Logger* logger_ = nullptr;
  Display* display_ = nullptr;
};
