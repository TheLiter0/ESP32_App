#include "services/WeatherService.h"
#include <LittleFS.h>

bool WeatherService::begin(Logger* logger) {
  logger_ = logger;
  // Don't error if the file doesn't exist yet — weather hasn't been fetched
  if (LittleFS.exists(kPath)) {
    reload();
  } else {
    if (logger_) logger_->info("[Weather] no data yet");
  }
  return true;
}

bool WeatherService::reload() {
  hasData_ = false;
  if (!LittleFS.exists(kPath)) return false;

  File f = LittleFS.open(kPath, FILE_READ);
  if (!f) return false;
  String json = f.readString();
  f.close();

  if (json.length() < 5) return false;

  auto extract = [&](const char* key, char* buf, size_t len) {
    String search = String("\"") + key + "\":\"";
    int s = json.indexOf(search);
    if (s < 0) return;
    s += search.length();
    int e = json.indexOf('"', s);
    if (e < 0) return;
    String val = json.substring(s, e);
    strncpy(buf, val.c_str(), len - 1);
    buf[len - 1] = '\0';
  };

  extract("temp",      temp_,      sizeof(temp_));
  extract("condition", condition_, sizeof(condition_));
  extract("humidity",  humidity_,  sizeof(humidity_));
  extract("city",      city_,      sizeof(city_));

  if (temp_[0]) {
    hasData_ = true;
    buildStrings_();
    if (logger_) logger_->info(("[Weather] " + String(line_)).c_str());
  }
  return hasData_;
}

void WeatherService::buildStrings_() {
  // "72F Sunny" — clean, fits in 30px status bar at medium text pitch
  snprintf(line_, sizeof(line_), "%sF %s", temp_, condition_);
  strncpy(full_, line_, sizeof(full_) - 1);
}