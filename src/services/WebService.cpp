#include "services/WebService.h"
#include <LittleFS.h>

void WebService::begin(Logger* logger, FsService* fs, WiFiService* wifi) {
  logger_ = logger;
  fs_ = fs;
  wifi_ = wifi;

  started_ = false;

  if (logger_) logger_->info("[Web] ready (waiting for WiFi)");
}

void WebService::startServer_() {
  server_.on("/health", HTTP_GET, [this]() {
    server_.send(200, "text/plain", "ok");
  });

  server_.on("/info", HTTP_GET, [this]() {
    String msg = "ip=" + wifi_->ip().toString();
    server_.send(200, "text/plain", msg);
  });

  server_.on("/fs", HTTP_GET, [this]() {
    server_.send(200, "text/plain", fs_->mounted() ? "mounted" : "not mounted");
  });

  // Serve /index.html
  server_.on("/", HTTP_GET, [this]() {
    if (!fs_->mounted()) {
      server_.send(500, "text/plain", "fs not mounted");
      return;
    }
    if (!LittleFS.exists("/index.html")) {
      server_.send(404, "text/plain", "missing /index.html");
      return;
    }
    File f = LittleFS.open("/index.html", "r");
    server_.streamFile(f, "text/html");
    f.close();
  });

  // Static fallback: /app.js, /style.css, etc.
  server_.onNotFound([this]() {
    if (!fs_->mounted()) {
      server_.send(404, "text/plain", "not found");
      return;
    }

    String path = server_.uri();
    if (path == "/") path = "/index.html";

    if (!LittleFS.exists(path)) {
      server_.send(404, "text/plain", "not found");
      return;
    }

    String contentType = "text/plain";
    if (path.endsWith(".html")) contentType = "text/html";
    else if (path.endsWith(".js")) contentType = "application/javascript";
    else if (path.endsWith(".css")) contentType = "text/css";
    else if (path.endsWith(".png")) contentType = "image/png";

    File f = LittleFS.open(path, "r");
    server_.streamFile(f, contentType);
    f.close();
  });

  server_.begin();
  started_ = true;

  if (logger_) {
    String msg = String("[Web] started: http://") + wifi_->ip().toString();
    logger_->info(msg.c_str());
  }
}

void WebService::update() {
  // Start server once WiFi is connected (non-blocking)
  if (!started_ && wifi_ && wifi_->connected()) {
    startServer_();
  }

  if (started_) {
    server_.handleClient();
  }
}
                         