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
// Serve the last uploaded PNG
server_.on("/latest.png", HTTP_GET, [this]() {
  if (!fs_->mounted()) { server_.send(500, "text/plain", "fs not mounted"); return; }
  if (!LittleFS.exists("/latest.png")) { server_.send(404, "text/plain", "no latest.png"); return; }

  File f = LittleFS.open("/latest.png", "r");
  server_.streamFile(f, "image/png");
  f.close();
});
server_.on("/latest_info", HTTP_GET, [this]() {
  if (!fs_->mounted()) { server_.send(500, "text/plain", "fs not mounted"); return; }
  if (!LittleFS.exists("/latest.png")) { server_.send(404, "text/plain", "no latest.png"); return; }

  File f = LittleFS.open("/latest.png", "r");
  size_t sz = f.size();

  uint8_t buf[16] = {0};
  size_t n = f.read(buf, sizeof(buf));
  f.close();

  String out;
  out += "size=" + String((uint32_t)sz) + "\n";
  out += "head16=";
  for (size_t i = 0; i < n; i++) {
    if (buf[i] < 16) out += "0";
    out += String(buf[i], HEX);
  }
  out += "\n";
  out += "expected_png_header=89504e470d0a1a0a\n"; // PNG signature

  server_.send(200, "text/plain", out);
});
// Simple HTML preview page
server_.on("/preview", HTTP_GET, [this]() {
  if (!fs_->mounted()) { server_.send(500, "text/plain", "fs not mounted"); return; }
  if (!LittleFS.exists("/latest.png")) { server_.send(404, "text/plain", "no latest.png"); return; }

  String html;
  html += "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>ESP32 Preview</title>";
  html += "<style>body{font-family:sans-serif;padding:16px}img{border:1px solid #ccc;border-radius:12px;max-width:100%}</style>";
  html += "</head><body>";
  html += "<h2>Last Upload Preview</h2>";
  html += "<p><a href='/latest.png'>Download latest.png</a></p>";
  html += "<img src='/latest.png?cachebust=" + String(millis()) + "' />";
  html += "</body></html>";

  server_.send(200, "text/html", html);
});
  server_.on("/info", HTTP_GET, [this]() {
    String msg = "ip=" + wifi_->ip().toString();
    server_.send(200, "text/plain", msg);
  });

  server_.on("/fs", HTTP_GET, [this]() {
    server_.send(200, "text/plain", fs_->mounted() ? "mounted" : "not mounted");
  });
  // Receive PNG upload and store to LittleFS
  server_.on(
    "/upload",
    HTTP_POST,
    [this]() {
      // This runs after upload finishes
      server_.send(200, "text/plain", "ok");
    },
    [this]() {
      // This runs during upload chunks
      if (!fs_->mounted()) {
        server_.send(500, "text/plain", "fs not mounted");
        return;
      }

      HTTPUpload& up = server_.upload();

      if (up.status == UPLOAD_FILE_START) {
        if (uploadFile_) uploadFile_.close();

        // overwrite each time
        uploadFile_ = LittleFS.open("/latest.png", "w");
        if (!uploadFile_) {
          if (logger_) logger_->error("[Web] failed to open /latest.png for write");
        } else {
          if (logger_) logger_->info("[Web] upload start -> /latest.png");
        }
      }
      else if (up.status == UPLOAD_FILE_WRITE) {
        if (uploadFile_) {
          uploadFile_.write(up.buf, up.currentSize);
          delay(0); // yield to keep WiFi + watchdog happy
        }
      }
      else if (up.status == UPLOAD_FILE_END) {
        if (uploadFile_) {
          uploadFile_.close();
        }
        if (logger_) {
          String msg = String("[Web] upload done bytes=") + up.totalSize;
          logger_->info(msg.c_str());
        }
      }
      else if (up.status == UPLOAD_FILE_ABORTED) {
        if (uploadFile_) uploadFile_.close();
        if (logger_) logger_->warn("[Web] upload aborted");
      }
    }
  );
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
                         