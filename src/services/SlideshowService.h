#pragma once
#include <Arduino.h>

class ImageStore;
class SettingsService;
class Display;
class Logger;

class SlideshowService {
public:
  void begin(Logger* logger, ImageStore* store,
             SettingsService* settings, Display* display);
  void update();   // call every loop
  void next();     // advance to next image immediately

  bool running() const;

  // Returns true (once) when the active image was just changed.
  // App calls this to know when to tell HomeScreen to redraw.
  bool consumeAdvanced();

private:
  bool loadImageList_();

  Logger*          logger_   = nullptr;
  ImageStore*      store_    = nullptr;
  SettingsService* settings_ = nullptr;
  Display*         display_  = nullptr;

  static constexpr int kMaxImages = 50;
  char     ids_[kMaxImages][20];
  int      count_    = 0;
  int      current_  = 0;
  bool     advanced_ = false;

  unsigned long lastAdvanceMs_  = 0;
  unsigned long lastListLoadMs_ = 0;
};
