#pragma once
#include <Arduino.h>

class Logger {
public:
  void begin(uint32_t baud) {
    Serial.begin(baud);
    delay(200);
  }

  void info(const char* msg) {
    Serial.println(msg);
  }
};
