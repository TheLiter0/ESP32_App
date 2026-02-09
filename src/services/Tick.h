#pragma once
#include <Arduino.h>

class Tick {
public:
  void begin() { lastMs_ = millis(); }

  void update() {
    uint32_t now = millis();
    dtMs_ = now - lastMs_;
    lastMs_ = now;
  }

private:
  uint32_t lastMs_ = 0;
  uint32_t dtMs_ = 0;
};
