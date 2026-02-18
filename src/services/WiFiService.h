#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "services/Logger.h"

class WiFiService {
public:
  void begin(Logger* logger);
  void update();

  bool connected() const;
  IPAddress ip() const;

  void startConnect(const char* ssid, const char* pass);

private:
  Logger* logger_ = nullptr;

  bool started_ = false;
  bool connecting_ = false;
  unsigned long lastAttemptMs_ = 0;
  unsigned long lastLogMs_ = 0;

  const char* ssid_ = nullptr;
  const char* pass_ = nullptr;
};
