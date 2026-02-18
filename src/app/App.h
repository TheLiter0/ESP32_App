#pragma once

#include "drivers/Display.h"
#include "services/Tick.h"
#include "services/Logger.h"
#include "services/SerialConsole.h"
#include "ui/ScreenManager.h"
#include "ui/BootScreen.h"
#include "services/WebService.h"
#include "services/FsService.h"
#include "services/WiFiService.h"
#include "services/WebService.h"

class App {
public:
  void begin();
  void update();

private:
  Logger logger_;
  Display display_;

  FsService fs_;
  WiFiService wifi_;
  WebService web_;

  Tick tick_;
  SerialConsole console_;
  ScreenManager screens_;
  BootScreen boot_;
};