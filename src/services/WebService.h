#pragma once
#include <Arduino.h>
#include <WebServer.h>

class Logger;
class FsService;

class WebService {
public:
  void begin(Logger* log, FsService* fs);
  void update();

private:
  Logger* log_ = nullptr;
  FsService* fs_ = nullptr;

  WebServer server_{80};

  void handleHealth();
  void handleInfo();
  void handleFs();
};
