#include "WebService.h"
#include "Logger.h"
#include "FsService.h"

#include <WiFi.h>

void WebService::begin(Logger* log, FsService* fs) {
  log_ = log;
  fs_ = fs;

  // TEMP: hardcode WiFi for now
  WiFi.begin("", "");

  if (log_) log_->info("Connecting WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  if (log_) log_->info("WiFi connected");
  if (log_) log_->info(WiFi.localIP().toString().c_str());

  server_.on("/health", HTTP_GET, [this]() { handleHealth(); });
  server_.on("/info",   HTTP_GET, [this]() { handleInfo(); });
  server_.on("/fs",     HTTP_GET, [this]() { handleFs(); });

  server_.begin();

  if (log_) log_->info("Web server started");
}

void WebService::update() {
  server_.handleClient();
}

void WebService::handleHealth() {
  server_.send(200, "text/plain", "ok");
}

void WebService::handleInfo() {
  String msg = "IP: " + WiFi.localIP().toString();
  server_.send(200, "text/plain", msg);
}

void WebService::handleFs() {
  if (!fs_ || !fs_->mounted()) {
    server_.send(500, "text/plain", "FS not mounted");
    return;
  }

  server_.send(200, "text/plain", "Filesystem mounted");
}
