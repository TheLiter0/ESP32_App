#include "services/FsService.h"
#include "services/Logger.h"

#include <FS.h>
#include <LittleFS.h>

bool FsService::begin(Logger* log, bool formatOnFail) {
  log_ = log;

  // Mount LittleFS (optionally format if mount fails)
  mounted_ = LittleFS.begin(formatOnFail);

  if (log_) {
    if (mounted_) log_->info("LittleFS mounted");
    else          log_->error("LittleFS mount failed");
  }

  return mounted_;
}
