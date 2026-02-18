#pragma once
#include <Arduino.h>

class Logger;

class FsService {
public:
  // formatOnFail=true will format the FS if mount fails (useful for first run)
  bool begin(Logger* log, bool formatOnFail = true);
  bool mounted() const { return mounted_; }

private:
  Logger* log_ = nullptr;
  bool mounted_ = false;
};
