#pragma once

#include "src/drivers/Display.h"
#include "src/services/Logger.h"
#include "src/services/Tick.h"
#include "src/services/SerialConsole.h"
#include "src/ui/ScreenManager.h"
#include "src/ui/BootScreen.h"
#include "src/services/FsService.h"

class App {
public:
  void begin();
  void update();

private:
  Logger logger_;
  Tick tick_;
  FsService fs_;
  Display display_;
  SerialConsole console_;
  ScreenManager screens_;
  BootScreen boot_;
};
