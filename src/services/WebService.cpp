#include "services/WebService.h"
#include <LittleFS.h>
#include <functional>

void WebService::begin(Logger* logger, FsService* fs, WiFiService* wifi,
                       ImageStore* imageStore, Display* display,
                       SettingsService* settings, SlideshowService* slideshow) {
  logger_     = logger;
  fs_         = fs;
  wifi_       = wifi;
  display_    = display;
  started_    = false;
  imageStore_ = imageStore;
  settings_   = settings;
  slideshow_  = slideshow;

  if (!server_)       server_       = new WebServer(80);
  if (!uploadServer_) uploadServer_ = new WiFiServer(81);

  if (logger_) logger_->info("[Web] ready (waiting for WiFi)");
}

// Helper: serve a file from LittleFS with correct content-type
static void serveFile(WebServer* server, const String& path) {
  if (!LittleFS.exists(path)) {
    server->send(404, "text/plain", "not found: " + path);
    return;
  }
  String ct = "text/plain";
  if      (path.endsWith(".html")) ct = "text/html";
  else if (path.endsWith(".js"))   ct = "application/javascript";
  else if (path.endsWith(".css"))  ct = "text/css";
  else if (path.endsWith(".json")) ct = "application/json";
  else if (path.endsWith(".png"))  ct = "image/png";
  File f = LittleFS.open(path, "r");
  server->streamFile(f, ct);
  f.close();
}

void WebService::startServer_() {

  // ── Core API ───────────────────────────────────────────────────────────
  server_->on("/health", HTTP_GET, [this]() {
    server_->send(200, "text/plain", "ok");
  });

  server_->on("/info", HTTP_GET, [this]() {
    server_->send(200, "text/plain", "ip=" + wifi_->ip().toString());
  });

  server_->on("/fs", HTTP_GET, [this]() {
    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();
    String s = "mounted total=" + String(total) + " used=" + String(used) +
               " free=" + String(total - used);
    server_->send(200, "text/plain", s);
  });

  // ── Image API ──────────────────────────────────────────────────────────
  server_->on("/api/images", HTTP_GET, [this]() {
    if (!imageStore_ || !imageStore_->ready()) {
      server_->send(500, "application/json", "[]");
      return;
    }
    server_->send(200, "application/json", imageStore_->indexJson());
  });

  server_->on("/api/images/show", HTTP_POST, [this]() {
    if (!imageStore_ || !imageStore_->ready()) {
      server_->send(500, "application/json", "{\"ok\":false,\"error\":\"store not ready\"}");
      return;
    }
    String body = server_->arg("plain");
    int idStart = body.indexOf("\"id\":");
    if (idStart < 0) {
      server_->send(400, "application/json", "{\"ok\":false,\"error\":\"missing id\"}");
      return;
    }
    idStart = body.indexOf('"', idStart + 5);
    int idEnd = body.indexOf('"', idStart + 1);
    if (idStart < 0 || idEnd < 0) {
      server_->send(400, "application/json", "{\"ok\":false,\"error\":\"bad id\"}");
      return;
    }
    String id = body.substring(idStart + 1, idEnd);
    bool ok = imageStore_->showById(id.c_str(), display_);
    if (ok && settings_) {
      settings_->setActiveImage(id.c_str());
      settings_->save();
    }
    server_->send(ok ? 200 : 404,
                  "application/json",
                  ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"not found\"}");
  });

  // Repair corrupt index - rescans /images/*.rgb565 and rebuilds index.json
  server_->on("/api/images/reindex", HTTP_POST, [this]() {
    if (!imageStore_ || !imageStore_->ready()) {
      server_->send(500, "application/json", "{\"ok\":false}");
      return;
    }
    int count = imageStore_->rebuildIndex();
    String resp = "{\"ok\":true,\"count\":" + String(count) + "}";
    server_->send(200, "application/json", resp);
  });

  // Delete a saved image
  server_->on("/api/images/delete", HTTP_POST, [this]() {
    if (!imageStore_ || !imageStore_->ready()) {
      server_->send(500, "application/json", "{\"ok\":false}");
      return;
    }
    String body = server_->arg("plain");
    int idStart = body.indexOf("\"id\":");
    if (idStart < 0) { server_->send(400, "application/json", "{\"ok\":false}"); return; }
    idStart = body.indexOf('"', idStart + 5);
    int idEnd = body.indexOf('"', idStart + 1);
    if (idStart < 0 || idEnd < 0) { server_->send(400, "application/json", "{\"ok\":false}"); return; }
    String id = body.substring(idStart + 1, idEnd);
    bool ok = imageStore_->deleteById(id.c_str());
    server_->send(ok ? 200 : 404, "application/json",
                  ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"not found\"}");
  });

  // ── Settings API ───────────────────────────────────────────────────────
  server_->on("/api/settings", HTTP_GET, [this]() {
    if (!settings_) { server_->send(500, "application/json", "{}"); return; }
    server_->send(200, "application/json", settings_->toJson());
  });

  server_->on("/api/settings", HTTP_POST, [this]() {
    if (!settings_) { server_->send(500, "application/json", "{}"); return; }
    settings_->applyJson(server_->arg("plain"));
    settings_->save();
    server_->send(200, "application/json", "{\"ok\":true}");
  });

  // Quotes GET
  server_->on("/api/quotes", HTTP_GET, [this]() {
    if (!LittleFS.exists("/config/quotes.json")) {
      server_->send(200, "application/json", "[]"); return;
    }
    File f = LittleFS.open("/config/quotes.json", "r");
    server_->streamFile(f, "application/json");
    f.close();
  });
  // Quotes POST
  server_->on("/api/quotes", HTTP_POST, [this]() {
    String body = server_->arg("plain");
    if (body.length() == 0) { server_->send(400, "application/json", "{\"ok\":false}"); return; }
    File f = LittleFS.open("/config/quotes.json", FILE_WRITE);
    if (!f) { server_->send(500, "application/json", "{\"ok\":false}"); return; }
    f.print(body);
    f.close();
    server_->send(200, "application/json", "{\"ok\":true}");
  });

  // Weather POST — browser fetches from Open-Meteo and saves here
  server_->on("/api/weather", HTTP_POST, [this]() {
    String body = server_->arg("plain");
    if (logger_) logger_->info(("[Weather] POST body len=" + String(body.length())).c_str());
    if (body.length() == 0) {
      server_->send(400, "application/json", "{\"ok\":false,\"error\":\"empty\"}");
      return;
    }
    File f = LittleFS.open("/config/weather.json", FILE_WRITE);
    if (!f) { server_->send(500, "application/json", "{\"ok\":false,\"error\":\"fs\"}"); return; }
    f.print(body);
    f.close();
    if (logger_) logger_->info("[Weather] saved");
    server_->send(200, "application/json", "{\"ok\":true}");
  });
  // Weather GET — return stored data
  server_->on("/api/weather/get", HTTP_GET, [this]() {
    if (!LittleFS.exists("/config/weather.json")) {
      server_->send(200, "application/json", "{}"); return;
    }
    File f = LittleFS.open("/config/weather.json", "r");
    server_->streamFile(f, "application/json");
    f.close();
  });

  // Slideshow control
  server_->on("/api/slideshow/next", HTTP_POST, [this]() {
    if (!slideshow_) { server_->send(500, "application/json", "{\"ok\":false}"); return; }
    slideshow_->next();
    server_->send(200, "application/json", "{\"ok\":true}");
  });

  // ── Debug: list all files on LittleFS ────────────────────────────────────
  server_->on("/api/ls", HTTP_GET, [this]() {
    String out = "";
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) {
      server_->send(500, "text/plain", "cannot open root");
      return;
    }
    // Walk recursively using a simple stack
    std::function<void(File, int)> walk = [&](File dir, int depth) {
      File entry = dir.openNextFile();
      while (entry) {
        for (int i = 0; i < depth; i++) out += "  ";
        out += entry.name();
        if (entry.isDirectory()) {
          out += "/\n";
          walk(entry, depth + 1);
        } else {
          out += "  (" + String(entry.size()) + " bytes)\n";
        }
        entry = dir.openNextFile();
      }
    };
    walk(root, 0);
    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();
    out += "\ntotal=" + String(total) + " used=" + String(used) +
           " free=" + String(total - used);
    server_->send(200, "text/plain", out);
  });

  // ── Static files — explicit routes so onNotFound isn't needed ──────────
  server_->on("/",          HTTP_GET, [this]() { serveFile(server_, "/index.html"); });
  server_->on("/index.html",HTTP_GET, [this]() { serveFile(server_, "/index.html"); });
  server_->on("/style.css", HTTP_GET, [this]() { serveFile(server_, "/style.css"); });
  server_->on("/app.js",    HTTP_GET, [this]() { serveFile(server_, "/app.js"); });

  // Catch-all for anything else on LittleFS
  server_->onNotFound([this]() {
    String path = server_->uri();
    if (fs_->mounted() && LittleFS.exists(path)) {
      serveFile(server_, path);
    } else {
      server_->send(404, "text/plain", "not found: " + path);
    }
  });

  server_->begin();
  uploadServer_->begin();
  started_ = true;

  if (logger_) {
    logger_->info(("ip=" + wifi_->ip().toString()).c_str());
    logger_->info("[Web] port 80=UI  port 81=upload");
  }
}

void WebService::handleUpload_() {
  WiFiClient client = uploadServer_->accept();
  if (!client) return;

  if (logger_) logger_->info("[Upload] client connected");

  // ── Parse request line + headers ────────────────────────────────────────
  uint32_t timeout = millis() + 5000;
  String   currentLine = "";
  String   method = "";
  int      contentLength = 0;
  bool     firstLine = true;
  bool     headersComplete = false;

  while (client.connected() && millis() < timeout && !headersComplete) {
    if (!client.available()) { delay(1); continue; }
    char c = client.read();
    if (c == '\n') {
      currentLine.trim();
      if (firstLine) {
        int sp = currentLine.indexOf(' ');
        if (sp > 0) method = currentLine.substring(0, sp);
        firstLine = false;
      } else if (currentLine.length() == 0) {
        headersComplete = true;
      } else if (currentLine.startsWith("Content-Length:")) {
        contentLength = currentLine.substring(15).toInt();
      }
      currentLine = "";
    } else if (c != '\r') {
      currentLine += c;
    }
  }

  if (logger_) {
    String msg = "[Upload] method=" + method +
                 " len=" + contentLength +
                 " heap=" + ESP.getFreeHeap() +
                 " fs_free=" + String(LittleFS.totalBytes() - LittleFS.usedBytes());
    logger_->info(msg.c_str());
  }

  // CORS preflight
  if (method == "OPTIONS") {
    client.print(
      "HTTP/1.1 204 No Content\r\n"
      "Access-Control-Allow-Origin: *\r\n"
      "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
      "Access-Control-Allow-Headers: Content-Type, Content-Length\r\n"
      "Access-Control-Max-Age: 86400\r\n"
      "Content-Length: 0\r\n"
      "Connection: close\r\n\r\n"
    );
    client.stop();
    return;
  }

  if (contentLength <= 0) {
    client.print("HTTP/1.1 400 Bad Request\r\nAccess-Control-Allow-Origin: *\r\n"
                 "Content-Length: 12\r\nConnection: close\r\n\r\nno body sent");
    client.stop();
    return;
  }

  // ── Check filesystem has room ────────────────────────────────────────────
  size_t fsFree = LittleFS.totalBytes() - LittleFS.usedBytes();
  if (fsFree < (size_t)contentLength + 4096) {  // 4KB margin for metadata
    if (logger_) {
      String msg = "[Upload] FS full! free=" + String(fsFree) +
                   " need=" + String(contentLength);
      logger_->error(msg.c_str());
    }
    client.print("HTTP/1.1 507 Insufficient Storage\r\nAccess-Control-Allow-Origin: *\r\n"
                 "Content-Length: 21\r\nConnection: close\r\n\r\nfilesystem is full :(");
    client.stop();
    return;
  }

  // ── Heap guard ───────────────────────────────────────────────────────────
  if (ESP.getFreeHeap() < (uint32_t)contentLength + 50000) {
    if (logger_) logger_->error("[Upload] low heap, refusing");
    client.print("HTTP/1.1 503 Service Unavailable\r\nAccess-Control-Allow-Origin: *\r\n"
                 "Content-Length: 10\r\nConnection: close\r\n\r\nlow memory");
    client.stop();
    return;
  }

  // ── Allocate buffer ──────────────────────────────────────────────────────
  uint8_t* buf = (uint8_t*)malloc(contentLength);
  if (!buf) {
    if (logger_) logger_->error("[Upload] malloc failed");
    client.print("HTTP/1.1 500 Internal Server Error\r\nAccess-Control-Allow-Origin: *\r\n"
                 "Content-Length: 3\r\nConnection: close\r\n\r\noom");
    client.stop();
    return;
  }

  // ── Receive body — patient loop with backoff ─────────────────────────────
  // The key fix: when available()==0, we wait up to 2s for more data before
  // giving up. TCP delivers in bursts; a momentary gap is normal.
  size_t   received = 0;
  uint32_t lastDataMs = millis();
  timeout = millis() + 30000;

  while (received < (size_t)contentLength && millis() < timeout) {
    if (!client.connected() && !client.available()) break;

    int avail = client.available();
    if (avail <= 0) {
      // Wait up to 2 seconds for the next TCP burst before giving up
      if (millis() - lastDataMs > 2000) {
        if (logger_) logger_->warn("[Upload] stalled, giving up");
        break;
      }
      delay(2);
      continue;
    }

    int chunk = client.read(buf + received,
                            min(avail, (int)(contentLength - received)));
    if (chunk > 0) {
      received    += chunk;
      lastDataMs   = millis();
    }
  }

  if (logger_) {
    String msg = "[Upload] received=" + String(received) + "/" + contentLength;
    logger_->info(msg.c_str());
  }

  // ── Must have received the full frame ───────────────────────────────────
  if (received != (size_t)contentLength) {
    free(buf);
    if (logger_) logger_->error("[Upload] incomplete, aborting");
    client.print("HTTP/1.1 400 Bad Request\r\nAccess-Control-Allow-Origin: *\r\n"
                 "Content-Length: 16\r\nConnection: close\r\n\r\nincomplete upload");
    client.stop();
    return;
  }

  // ── Draw ─────────────────────────────────────────────────────────────────
  const uint16_t w = (uint16_t)Layout::CANVAS.w;
  const uint16_t h = (uint16_t)(received / 2 / w);

  if (display_) {
    display_->drawRGB565(Layout::CANVAS.x, Layout::CANVAS.y,
                         w, h, (const uint16_t*)buf);
    if (logger_) logger_->info(("[Upload] drew " + String(w) + "x" + String(h)).c_str());
  }

  // ── Save ─────────────────────────────────────────────────────────────────
  String responseBody = "{\"ok\":true,\"saved\":false}";
  int    responseCode = 200;

  if (imageStore_ && imageStore_->ready()) {
    ImageInfo saved;
    bool ok = imageStore_->saveRgb565(buf, received, w, h, nullptr, &saved);
    if (ok) {
      if (settings_) {
        settings_->setActiveImage(saved.id.c_str());
        settings_->save();
      }
      responseBody = "{\"ok\":true,\"id\":\"" + saved.id +
                     "\",\"file\":\"" + saved.file + "\"}";
      responseCode = 200;
    } else {
      responseBody = "{\"ok\":false,\"error\":\"save failed\"}";
      responseCode = 500;
    }
  }

  free(buf);

  // ── Respond ───────────────────────────────────────────────────────────────
  client.print("HTTP/1.1 " + String(responseCode) +
               (responseCode == 200 ? " OK\r\n" : " Internal Server Error\r\n"));
  client.print("Access-Control-Allow-Origin: *\r\n"
               "Content-Type: application/json\r\nContent-Length: ");
  client.print(responseBody.length());
  client.print("\r\nConnection: close\r\n\r\n");
  client.print(responseBody);
  client.stop();
}

void WebService::update() {
  if (!started_ && wifi_ && wifi_->ready()) {
    startServer_();
  }
  if (started_) {
    server_->handleClient();
    handleUpload_();
  }
}