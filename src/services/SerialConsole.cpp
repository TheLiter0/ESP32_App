#include "src/services/SerialConsole.h"
#include <Arduino.h>
#include <string.h>

void SerialConsole::update() {
  if (!Serial.available()) return;

  static char buf[80];
  size_t n = Serial.readBytesUntil('\n', buf, sizeof(buf) - 1);
  buf[n] = 0;

  if (logger_) logger_->info(buf);

  if (strcmp(buf, "cls") == 0) {
    display_->clear();
    return;
  }

  if (strcmp(buf, "ready") == 0) {
    display_->setText(10, 10, 2, 255, 255, 255);
    display_->print("READY");
    return;
  }
}
