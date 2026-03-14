#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <FS.h>

#include "services/ImageStore.h"
#include "services/Logger.h"
#include "services/FsService.h"
#include "services/WiFiService.h"
#include "services/SettingsService.h"
#include "services/SlideshowService.h"
#include "drivers/Display.h"
#include "ui/Layout.h"

class WebService {
public:
  void begin(Logger* logger, FsService* fs, WiFiService* wifi,
             ImageStore* imageStore, Display* display,
             SettingsService* settings = nullptr,
             SlideshowService* slideshow = nullptr);
  void update();
  bool started() const { return started_; }

private:
  void startServer_();
  void handleUpload_();

  Logger*          logger_       = nullptr;
  FsService*       fs_           = nullptr;
  WiFiService*     wifi_         = nullptr;
  Display*         display_      = nullptr;
  ImageStore*      imageStore_   = nullptr;
  SettingsService*  settings_    = nullptr;
  SlideshowService* slideshow_   = nullptr;
  WebServer*       server_       = nullptr;
  WiFiServer*      uploadServer_ = nullptr;
  bool             started_      = false;
};
