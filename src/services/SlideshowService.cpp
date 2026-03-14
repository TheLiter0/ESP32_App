#include "services/SlideshowService.h"
#include "services/ImageStore.h"
#include "services/SettingsService.h"
#include "services/Logger.h"
#include "drivers/Display.h"

void SlideshowService::begin(Logger* logger, ImageStore* store,
                              SettingsService* settings, Display* display) {
  logger_   = logger;
  store_    = store;
  settings_ = settings;
  display_  = display;
  count_    = 0;
  current_  = 0;
  advanced_ = false;
  lastAdvanceMs_  = millis();
  lastListLoadMs_ = 0;
}

bool SlideshowService::running() const {
  return settings_ && settings_->slideshowEnabled() && count_ > 1;
}

bool SlideshowService::consumeAdvanced() {
  bool v = advanced_;
  advanced_ = false;
  return v;
}

void SlideshowService::update() {
  if (!settings_ || !settings_->slideshowEnabled()) return;
  if (!store_ || !store_->ready()) return;

  // Reload image list every 10 seconds
  if (millis() - lastListLoadMs_ > 10000 || count_ == 0) {
    loadImageList_();
    lastListLoadMs_ = millis();
  }

  if (count_ <= 1) return;

  unsigned long intervalMs = (unsigned long)settings_->slideshowIntervalSec() * 1000UL;
  if (millis() - lastAdvanceMs_ >= intervalMs) {
    next();
  }
}

void SlideshowService::next() {
  if (count_ == 0) { loadImageList_(); }
  if (count_ == 0) return;

  current_ = (current_ + 1) % count_;
  const char* id = ids_[current_];

  if (logger_) logger_->info(("[Slideshow] -> " + String(id)).c_str());

  // Just update the active image — let HomeScreen do the actual drawing.
  // This prevents the double-draw race where showById draws then HomeScreen
  // immediately overwrites with the old image on the next update() tick.
  if (settings_) settings_->setActiveImage(id);

  advanced_ = true;  // signal to App that image changed
  lastAdvanceMs_ = millis();
}

bool SlideshowService::loadImageList_() {
  count_ = 0;
  if (!store_) return false;

  String index = store_->indexJson();
  int searchFrom = 0;
  while (count_ < kMaxImages) {
    int idStart = index.indexOf("\"id\":\"", searchFrom);
    if (idStart < 0) break;
    idStart += 6;
    int idEnd = index.indexOf('"', idStart);
    if (idEnd < 0) break;

    String id = index.substring(idStart, idEnd);
    strncpy(ids_[count_], id.c_str(), sizeof(ids_[0]) - 1);
    ids_[count_][sizeof(ids_[0]) - 1] = '\0';
    count_++;
    searchFrom = idEnd + 1;
  }

  if (logger_)
    logger_->info(("[Slideshow] " + String(count_) + " images loaded").c_str());

  if (current_ >= count_) current_ = 0;
  return count_ > 0;
}