#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "services/Logger.h"

class WiFiService {
public:
  enum class Mode {
    None,
    Station,
    AccessPoint
  };

  void begin(Logger* logger);
  void update();

  bool connected() const;     // true only for STA connected
  bool ready() const;         // true for STA connected OR AP running
  IPAddress ip() const;
  Mode mode() const { return mode_; }

  void startConnect(const char** ssids, const char** passes, int count);
  void startConnect(const char* ssid, const char* pass);

  void startAccessPoint(const char* ssid, const char* pass);

private:
  void tryNetwork_(int index);

  Logger* logger_ = nullptr;

  bool started_    = false;
  bool connecting_ = false;

  const char** ssids_ = nullptr;
  const char** passes_ = nullptr;
  int networkCount_   = 0;
  int currentNetwork_ = 0;

  unsigned long lastAttemptMs_ = 0;
  unsigned long lastLogMs_     = 0;

  Mode mode_ = Mode::None;

  static constexpr unsigned long RETRY_MS = 8000;
};