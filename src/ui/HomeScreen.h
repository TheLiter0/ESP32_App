#pragma once
#include <cstring>
#include "ui/Screen.h"

class ImageStore;
class SettingsService;
class ClockService;
class QuoteService;
class WeatherService;

class HomeScreen : public Screen {
public:
  void setServices(ImageStore* store, SettingsService* settings,
                   ClockService* clock, QuoteService* quotes = nullptr,
                   WeatherService* weather = nullptr);
  void setStatusText(const char* mode, const char* ip);

  void begin(Display& display)  override;
  void update(Display& display) override;

  void invalidate()       { dirty_ = true; clockDirty_ = true; statusDirty_ = true; }
  void invalidateClock()  { clockDirty_  = true; memset(lastTimeStr_, 0, sizeof(lastTimeStr_)); }
  void invalidateImage()  { dirty_       = true; }
  void invalidateStatus() { statusDirty_ = true; }

private:
  void drawTopBar_(Display& d);
  void drawImage_(Display& d);
  void drawStatusBar_(Display& d);

  ImageStore*      store_    = nullptr;
  SettingsService* settings_ = nullptr;
  ClockService*    clock_    = nullptr;
  QuoteService*    quotes_   = nullptr;
  WeatherService*  weather_  = nullptr;

  char modeText_[40] = "Mode: --";
  char ipText_[40]   = "IP: --";

  bool          dirty_        = true;
  bool          clockDirty_   = true;
  bool          statusDirty_  = true;
  unsigned long lastClockMs_   = 0;
  unsigned long lastQuoteMs_   = 0;
  char          lastTimeStr_[8] = "";
};