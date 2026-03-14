#pragma once
#include <Arduino.h>
#include "services/Logger.h"

class SettingsService {
public:
  bool begin(Logger* logger);

  const char* activeImage()  const { return activeImage_; }
  void setActiveImage(const char* id);

  bool slideshowEnabled()    const { return slideshowEnabled_; }
  void setSlideshowEnabled(bool v) { slideshowEnabled_ = v; }

  int  slideshowIntervalSec() const { return slideshowIntervalSec_; }
  void setSlideshowIntervalSec(int s) { slideshowIntervalSec_ = s < 1 ? 1 : s; }

  bool clockEnabled()        const { return clockEnabled_; }
  void setClockEnabled(bool v)     { clockEnabled_ = v; }

  // "wifi" = show mode+IP in status bar, "quote" = show quote text
  const char* statusMode()   const { return statusMode_; }
  void setStatusMode(const char* m);

  // Weather — coordinates for Open-Meteo fetch (browser fetches, posts to ESP32)
  const char* weatherLat() const { return weatherLat_; }
  const char* weatherLon() const { return weatherLon_; }
  const char* weatherCity() const { return weatherCity_; }
  void setWeatherLat(const char* v);
  void setWeatherLon(const char* v);
  void setWeatherCity(const char* v);

  // Bar background colours as 6-char hex strings, e.g. "121622"
  const char* topBarColor()    const { return topBarColor_; }
  const char* statusBarColor() const { return statusBarColor_; }
  void setTopBarColor(const char* hex);
  void setStatusBarColor(const char* hex);

  // Text colours — 6-char hex strings
  // Top bar
  const char* dateColor()       const { return dateColor_; }
  const char* timeColor()       const { return timeColor_; }
  // Status bar — wifi mode
  const char* modeColor()       const { return modeColor_; }
  const char* ipColor()         const { return ipColor_; }
  // Status bar — quote mode
  const char* quoteColor()      const { return quoteColor_; }
  // Status bar — weather mode
  const char* weatherCityColor()const { return weatherCityColor_; }
  const char* weatherTempColor()const { return weatherTempColor_; }

  void setDateColor(const char* hex);
  void setTimeColor(const char* hex);
  void setModeColor(const char* hex);
  void setIpColor(const char* hex);
  void setQuoteColor(const char* hex);
  void setWeatherCityColor(const char* hex);
  void setWeatherTempColor(const char* hex);

  bool  save();
  bool  load();

  String toJson()               const;
  void   applyJson(const String& json);

private:
  Logger* logger_ = nullptr;

  char activeImage_[20]      = "";
  bool slideshowEnabled_     = false;
  int  slideshowIntervalSec_ = 30;
  bool clockEnabled_         = true;
  char statusMode_[12]       = "wifi";  // "wifi" | "quote" | "weather"
  char weatherLat_[16]       = "42.93";   // default: Grand Rapids MI
  char weatherLon_[16]       = "-85.68";
  char weatherCity_[32]      = "Grand Rapids";
  char topBarColor_[8]       = "121622";
  char statusBarColor_[8]    = "0e121c";
  // Text colours
  char dateColor_[8]         = "82a0dc";  // soft blue
  char timeColor_[8]         = "f0f0c8";  // warm white
  char modeColor_[8]         = "50d2b4";  // teal
  char ipColor_[8]           = "7896c8";  // muted blue
  char quoteColor_[8]        = "c8beA0";  // warm cream
  char weatherCityColor_[8]  = "8cd2ff";  // light blue
  char weatherTempColor_[8]  = "ffdc64";  // warm yellow

  static constexpr const char* kPath = "/config/settings.json";
};