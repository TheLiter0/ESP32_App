#include "services/SettingsService.h"
#include <functional>
#include <LittleFS.h>

bool SettingsService::begin(Logger* logger) {
  logger_ = logger;
  bool ok = load();
  if (logger_) logger_->info(ok ? "[Settings] loaded" : "[Settings] using defaults");
  return true; // always succeeds (defaults are fine)
}

void SettingsService::setStatusMode(const char* m) {
  if (!m) return;
  strncpy(statusMode_, m, sizeof(statusMode_) - 1);
  statusMode_[sizeof(statusMode_) - 1] = '\0';
}

void SettingsService::setWeatherLat(const char* v) {
  if (!v) return; strncpy(weatherLat_, v, sizeof(weatherLat_)-1); weatherLat_[sizeof(weatherLat_)-1]='\0';
}
void SettingsService::setWeatherLon(const char* v) {
  if (!v) return; strncpy(weatherLon_, v, sizeof(weatherLon_)-1); weatherLon_[sizeof(weatherLon_)-1]='\0';
}
void SettingsService::setWeatherCity(const char* v) {
  if (!v) return; strncpy(weatherCity_, v, sizeof(weatherCity_)-1); weatherCity_[sizeof(weatherCity_)-1]='\0';
}

void SettingsService::setDateColor(const char* hex)        { if(hex){strncpy(dateColor_,hex,7);dateColor_[7]=0;} }
void SettingsService::setTimeColor(const char* hex)        { if(hex){strncpy(timeColor_,hex,7);timeColor_[7]=0;} }
void SettingsService::setModeColor(const char* hex)        { if(hex){strncpy(modeColor_,hex,7);modeColor_[7]=0;} }
void SettingsService::setIpColor(const char* hex)          { if(hex){strncpy(ipColor_,hex,7);ipColor_[7]=0;} }
void SettingsService::setQuoteColor(const char* hex)       { if(hex){strncpy(quoteColor_,hex,7);quoteColor_[7]=0;} }
void SettingsService::setWeatherCityColor(const char* hex) { if(hex){strncpy(weatherCityColor_,hex,7);weatherCityColor_[7]=0;} }
void SettingsService::setWeatherTempColor(const char* hex) { if(hex){strncpy(weatherTempColor_,hex,7);weatherTempColor_[7]=0;} }

void SettingsService::setTopBarColor(const char* hex) {
  if (!hex) return;
  strncpy(topBarColor_, hex, sizeof(topBarColor_) - 1);
  topBarColor_[sizeof(topBarColor_) - 1] = '\0';
}

void SettingsService::setStatusBarColor(const char* hex) {
  if (!hex) return;
  strncpy(statusBarColor_, hex, sizeof(statusBarColor_) - 1);
  statusBarColor_[sizeof(statusBarColor_) - 1] = '\0';
}

void SettingsService::setActiveImage(const char* id) {
  if (!id) { activeImage_[0] = '\0'; return; }
  strncpy(activeImage_, id, sizeof(activeImage_) - 1);
  activeImage_[sizeof(activeImage_) - 1] = '\0';
}

bool SettingsService::save() {
  File f = LittleFS.open(kPath, FILE_WRITE);
  if (!f) {
    if (logger_) logger_->error("[Settings] save failed");
    return false;
  }
  f.print(toJson());
  f.close();
  if (logger_) logger_->info("[Settings] saved");
  return true;
}

bool SettingsService::load() {
  if (!LittleFS.exists(kPath)) return false;
  File f = LittleFS.open(kPath, FILE_READ);
  if (!f) return false;
  String json = f.readString();
  f.close();
  if (json.length() < 2) return false;
  applyJson(json);
  return true;
}

String SettingsService::toJson() const {
  String j = "{";
  j += "\"activeImage\":\"" + String(activeImage_) + "\",";
  j += "\"slideshowEnabled\":" + String(slideshowEnabled_ ? "true" : "false") + ",";
  j += "\"slideshowIntervalSec\":" + String(slideshowIntervalSec_) + ",";
  j += "\"clockEnabled\":" + String(clockEnabled_ ? "true" : "false") + ",";
  j += "\"statusMode\":\"" + String(statusMode_) + "\",";
  j += "\"topBarColor\":\"" + String(topBarColor_) + "\",";
  j += "\"statusBarColor\":\"" + String(statusBarColor_) + "\",";
  j += "\"dateColor\":\"" + String(dateColor_) + "\",";
  j += "\"timeColor\":\"" + String(timeColor_) + "\",";
  j += "\"modeColor\":\"" + String(modeColor_) + "\",";
  j += "\"ipColor\":\"" + String(ipColor_) + "\",";
  j += "\"quoteColor\":\"" + String(quoteColor_) + "\",";
  j += "\"weatherCityColor\":\"" + String(weatherCityColor_) + "\",";
  j += "\"weatherTempColor\":\"" + String(weatherTempColor_) + "\"";
  j += "}";
  return j;
}

// Minimal JSON parser — no external lib needed on the ESP32
static String jsonStr(const String& json, const char* key) {
  String search = String("\"") + key + "\":\"";
  int s = json.indexOf(search);
  if (s < 0) return "";
  s += search.length();
  int e = json.indexOf('"', s);
  if (e < 0) return "";
  return json.substring(s, e);
}

static int jsonInt(const String& json, const char* key, int def) {
  String search = String("\"") + key + "\":";
  int s = json.indexOf(search);
  if (s < 0) return def;
  s += search.length();
  return json.substring(s).toInt();
}

static bool jsonBool(const String& json, const char* key, bool def) {
  String search = String("\"") + key + "\":";
  int s = json.indexOf(search);
  if (s < 0) return def;
  s += search.length();
  String val = json.substring(s, s + 5);
  val.trim();
  if (val.startsWith("true"))  return true;
  if (val.startsWith("false")) return false;
  return def;
}

void SettingsService::applyJson(const String& json) {
  if (json.indexOf("\"activeImage\"") >= 0) {
    String ai = jsonStr(json, "activeImage");
    setActiveImage(ai.c_str());
  }
  slideshowEnabled_     = jsonBool(json, "slideshowEnabled",     slideshowEnabled_);
  slideshowIntervalSec_ = jsonInt (json, "slideshowIntervalSec", slideshowIntervalSec_);
  clockEnabled_         = jsonBool(json, "clockEnabled",         clockEnabled_);
  if (json.indexOf("\"weatherLat\"") >= 0) {
    String v = jsonStr(json, "weatherLat"); if (v.length()) setWeatherLat(v.c_str());
  }
  if (json.indexOf("\"weatherLon\"") >= 0) {
    String v = jsonStr(json, "weatherLon"); if (v.length()) setWeatherLon(v.c_str());
  }
  if (json.indexOf("\"weatherCity\"") >= 0) {
    String v = jsonStr(json, "weatherCity"); if (v.length()) setWeatherCity(v.c_str());
  }
  if (json.indexOf("\"statusMode\"") >= 0) {
    String sm = jsonStr(json, "statusMode");
    if (sm.length() > 0) setStatusMode(sm.c_str());
  }
  if (json.indexOf("\"topBarColor\"") >= 0) {
    String v = jsonStr(json, "topBarColor");
    if (v.length() == 6) setTopBarColor(v.c_str());
  }
  if (json.indexOf("\"statusBarColor\"") >= 0) {
    String v = jsonStr(json, "statusBarColor");
    if (v.length() == 6) setStatusBarColor(v.c_str());
  }
  auto applyCol = [&](const char* key, std::function<void(const char*)> setter) {
    if (json.indexOf(String("\"") + key + "\"") >= 0) {
      String v = jsonStr(json, key);
      if (v.length() == 6) setter(v.c_str());
    }
  };
  applyCol("dateColor",        [this](const char* v){ setDateColor(v); });
  applyCol("timeColor",        [this](const char* v){ setTimeColor(v); });
  applyCol("modeColor",        [this](const char* v){ setModeColor(v); });
  applyCol("ipColor",          [this](const char* v){ setIpColor(v); });
  applyCol("quoteColor",       [this](const char* v){ setQuoteColor(v); });
  applyCol("weatherCityColor", [this](const char* v){ setWeatherCityColor(v); });
  applyCol("weatherTempColor", [this](const char* v){ setWeatherTempColor(v); });
}