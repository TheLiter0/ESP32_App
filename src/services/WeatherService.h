#pragma once
#include <Arduino.h>
#include "services/Logger.h"

// Stores weather data that the browser fetches and POSTs to /api/weather.
// Provides formatted strings ready to display in the 30px status bar.
class WeatherService {
public:
  bool begin(Logger* logger);

  // Reload from /config/weather.json (call after a POST arrives)
  bool reload();

  // "72°F  Sunny" — fits in the status bar
  const char* line() const { return line_; }

  // "Grand Rapids · 72°F · Sunny · 45% humidity"
  const char* full() const { return full_; }

  bool hasData() const { return hasData_; }

private:
  void buildStrings_();

  Logger* logger_  = nullptr;
  bool    hasData_ = false;

  char temp_[8]      = "";
  char condition_[24]= "";
  char humidity_[8]  = "";
  char city_[32]     = "";
  char line_[48]     = "";
  char full_[80]     = "";

  static constexpr const char* kPath = "/config/weather.json";
};