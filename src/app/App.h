#pragma once

#include "src/drivers/Display.h"
#include "src/services/Logger.h"
#include "src/services/Tick.h"
#include "src/services/SerialConsole.h"

class App {
public:
  void begin();
  void update();

private:
  Logger logger_;
  Tick tick_;
  Display display_;
  SerialConsole console_;
};
