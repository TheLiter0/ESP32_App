#include "services/WiFiService.h"

void WiFiService::begin(Logger* logger) {
  logger_ = logger;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  started_ = true;
}

void WiFiService::startConnect(const char* ssid, const char* pass) {
  if (!started_ || !logger_) return;

  ssid_ = ssid;
  pass_ = pass;

  logger_->info("[WiFi] begin connect...");
  WiFi.begin(ssid_, pass_);

  connecting_ = true;
  lastAttemptMs_ = millis();
  lastLogMs_ = 0;
}

void WiFiService::update() {
  if (!started_ || !connecting_ || !logger_) return;

  if (connected()) {
    connecting_ = false;

    String msg = String("[WiFi] connected: ") + ip().toString();
    logger_->info(msg.c_str());
    return;
  }

  const unsigned long now = millis();

  // Retry every 10 seconds if not connected
  if (now - lastAttemptMs_ > 10000) {
    logger_->warn("[WiFi] retry...");
    WiFi.disconnect(true);
    delay(50);
    WiFi.begin(ssid_, pass_);
    lastAttemptMs_ = now;
  }

  // Periodic status log every 2 seconds
  if (now - lastLogMs_ > 2000) {
    String msg = String("[WiFi] status=") + (int)WiFi.status();
    logger_->info(msg.c_str());
    lastLogMs_ = now;
  }
}

bool WiFiService::connected() const {
  return WiFi.status() == WL_CONNECTED;
}

IPAddress WiFiService::ip() const {
  return WiFi.localIP();
}
