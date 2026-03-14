#include "services/WiFiService.h"

void WiFiService::begin(Logger* logger) {
  logger_ = logger;
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);
  started_ = true;
  mode_ = Mode::None;
}

void WiFiService::tryNetwork_(int index) {
  if (logger_) {
    String msg = String("[WiFi] trying: ") + ssids_[index];
    logger_->info(msg.c_str());
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(50);
  WiFi.begin(ssids_[index], passes_[index]);

  currentNetwork_ = index;
  lastAttemptMs_  = millis();
  lastLogMs_      = 0;
  mode_ = Mode::Station;
}

void WiFiService::startConnect(const char** ssids, const char** passes, int count) {
  if (!started_ || count <= 0) return;

  ssids_        = ssids;
  passes_       = passes;
  networkCount_ = count;
  connecting_   = true;

  tryNetwork_(0);
}

void WiFiService::startConnect(const char* ssid, const char* pass) {
  static const char* s[1];
  static const char* p[1];
  s[0] = ssid;
  p[0] = pass;
  startConnect(s, p, 1);
}

void WiFiService::startAccessPoint(const char* ssid, const char* pass) {
  if (!started_) return;

  connecting_ = false;
  WiFi.mode(WIFI_AP);
  WiFi.softAPdisconnect(true);
  delay(100);

  bool ok = WiFi.softAP(ssid, pass);
  if (ok) {
    mode_ = Mode::AccessPoint;

    if (logger_) {
      String msg = String("[WiFi] AP started: ") + ssid + " ip=" + WiFi.softAPIP().toString();
      logger_->info(msg.c_str());
    }
  } else {
    mode_ = Mode::None;
    if (logger_) logger_->error("[WiFi] AP start failed");
  }
}

void WiFiService::update() {
  if (!started_) return;

  if (mode_ != Mode::Station || !connecting_) return;

  if (connected()) {
    connecting_ = false;

    String msg = String("[WiFi] connected: ") + ip().toString()
               + " (" + ssids_[currentNetwork_] + ")";
    if (logger_) logger_->info(msg.c_str());
    return;
  }

  const unsigned long now = millis();

  if (now - lastAttemptMs_ > RETRY_MS) {
    int next = (currentNetwork_ + 1) % networkCount_;
    tryNetwork_(next);
    return;
  }

  if (logger_ && now - lastLogMs_ > 2000) {
    String msg = String("[WiFi] status=") + (int)WiFi.status()
               + " ssid=" + ssids_[currentNetwork_];
    logger_->info(msg.c_str());
    lastLogMs_ = now;
  }
}

bool WiFiService::connected() const {
  return mode_ == Mode::Station && WiFi.status() == WL_CONNECTED;
}

bool WiFiService::ready() const {
  if (mode_ == Mode::Station) {
    return WiFi.status() == WL_CONNECTED;
  }

  if (mode_ == Mode::AccessPoint) {
    IPAddress ap = WiFi.softAPIP();
    return ap[0] != 0;
  }

  return false;
}

IPAddress WiFiService::ip() const {
  if (mode_ == Mode::AccessPoint) {
    return WiFi.softAPIP();
  }
  return WiFi.localIP();
}