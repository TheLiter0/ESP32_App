#pragma once
#include <Arduino.h>

class Logger {
public:
  void begin(uint32_t baud) {
    Serial.begin(baud);
    delay(200);
  }

  void info(const char* msg) {
    Serial.print("[I] ");
    Serial.println(msg);
  }

  void warn(const char* msg) {
    Serial.print("[W] ");
    Serial.println(msg);
  }

  void error(const char* msg) {
    Serial.print("[E] ");
    Serial.println(msg);
  }
};
