#pragma once

#include "drivers/Display.h"
#include "services/Tick.h"
#include "services/Logger.h"
#include "services/SerialConsole.h"
#include "services/FsService.h"
#include "services/WiFiService.h"
#include "services/WebService.h"
#include "services/ImageStore.h"
#include "services/SettingsService.h"
#include "services/ClockService.h"
#include "services/SlideshowService.h"
#include "services/QuoteService.h"
#include "services/WeatherService.h"
#include "ui/ScreenManager.h"
#include "ui/BootScreen.h"
#include "ui/HomeScreen.h"

class App {
public:
  void begin();
  void update();

private:
  Logger           logger_;
  Display          display_;
  FsService        fs_;
  WiFiService      wifi_;
  ImageStore       imageStore_;
  SettingsService  settings_;
  ClockService     clock_;
  SlideshowService slideshow_;
  QuoteService     quotes_;
  WeatherService   weather_;
  WebService       web_;
  Tick             tick_;
  SerialConsole    console_;
  ScreenManager    screens_;
  BootScreen       boot_;
  HomeScreen       home_;

  bool wifiKey_  = false;
  bool ntpDone_  = false;

  // Track previous settings to detect changes and react immediately
  bool prevSlideshowEnabled_ = false;
  bool prevClockEnabled_     = true;
  char prevActiveImage_[20]       = "";
  char prevStatusMode_[12]        = "wifi";
  char prevTopBarColor_[8]        = "121622";
  char prevStatusBarColor_[8]     = "0e121c";
};