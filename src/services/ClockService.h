#pragma once
#include <Arduino.h>

class ClockService {
public:
  void begin();
  void update();

  // Sync with NTP. gmtOffsetSec = your UTC offset in seconds.
  // e.g. UTC-5 (EST) = -18000,  UTC-4 (EDT) = -14400
  // daylightOffsetSec = 3600 if your region uses DST, else 0
  bool syncNtp(const char* server = "pool.ntp.org",
               long gmtOffsetSec = -18000,
               int  daylightOffsetSec = 3600);

  bool synced() const { return synced_; }

  void getTimeStr(char* buf, size_t len) const;  // "HH:MM"
  void getDateStr(char* buf, size_t len) const;  // "Mon Mar 13"

private:
  bool synced_ = false;
};
