#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "services/Logger.h"
#include "services/FsService.h"
#include "services/WiFiService.h"

class WebService {
public:
  void begin(Logger* logger, FsService* fs, WiFiService* wifi);
  void update();

  bool started() const { return started_; }

private:
  void startServer_();

  Logger* logger_ = nullptr;
  FsService* fs_ = nullptr;
  WiFiService* wifi_ = nullptr;

  WebServer server_{80};
  bool started_ = false;
};
