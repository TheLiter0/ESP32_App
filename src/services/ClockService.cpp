#include "services/ClockService.h"
#include <time.h>

void ClockService::begin() {
  synced_ = false;
}

void ClockService::update() {
  // SNTP updates automatically in the background after syncNtp().
}

bool ClockService::syncNtp(const char* server, long gmtOffsetSec, int daylightOffsetSec) {
  configTime(gmtOffsetSec, daylightOffsetSec, server);

  struct tm ti;
  const uint32_t deadline = millis() + 8000;
  while (millis() < deadline) {
    if (getLocalTime(&ti) && ti.tm_year > 120) {
      synced_ = true;
      return true;
    }
    delay(200);
  }
  return false;
}

void ClockService::getTimeStr(char* buf, size_t len) const {
  struct tm ti;
  if (synced_ && getLocalTime(&ti) && ti.tm_year > 120) {
    // 12-hour clock with AM/PM, e.g. "3:47 PM"
    int hour12 = ti.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    const char* ampm = ti.tm_hour < 12 ? "AM" : "PM";
    snprintf(buf, len, "%d:%02d %s", hour12, ti.tm_min, ampm);
  } else {
    // Fallback: elapsed time since boot in 12h format
    unsigned long sec = millis() / 1000;
    int h = (int)((sec / 3600) % 24);
    int m = (int)((sec % 3600) / 60);
    int h12 = h % 12; if (h12 == 0) h12 = 12;
    snprintf(buf, len, "%d:%02d %s", h12, m, h < 12 ? "AM" : "PM");
  }
}

void ClockService::getDateStr(char* buf, size_t len) const {
  struct tm ti;
  if (synced_ && getLocalTime(&ti) && ti.tm_year > 120) {
    strftime(buf, len, "%a %b %d", &ti);
  } else {
    strncpy(buf, "", len);
  }
}