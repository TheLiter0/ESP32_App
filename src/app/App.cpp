#include "app/App.h"

void App::begin() {
  logger_.begin(115200);
  logger_.info("[App] begin");

  // Display first
  display_.begin();
  logger_.info("[Display] ok");

  // Filesystem
  fs_.begin(&logger_, true);
  logger_.info(fs_.mounted() ? "[FS] mounted=true" : "[FS] mounted=false");

  // WiFi
  wifi_.begin(&logger_);

  // Web (waits for WiFi; no blocking)
  web_.begin(&logger_, &fs_, &wifi_);

  // Other services
  tick_.begin();
  console_.begin(&logger_, &display_);

  // ScreenManager uses Display& signatures
  screens_.begin(display_);
  screens_.set(&boot_, display_);
}

void App::update() {
  tick_.update();

  wifi_.update();
  web_.update();

  console_.update();
  screens_.update(display_);
}
