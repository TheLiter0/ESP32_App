#include "App.h"

void App::begin() {
  logger_.begin(115200);
  logger_.info("Boot");
  
  fs_.begin(&logger_, true);
  
  web_.begin(&logger_, &fs_);

  // 1) Display must be initialized first
  display_.begin();

  // 2) Now width/height are valid
  logger_.info(String(display_.width()).c_str());
  logger_.info(String(display_.height()).c_str());

  // 3) Services
  tick_.begin();
  console_.begin(&logger_, &display_);

  // 4) Screens own the rendering
  screens_.begin(display_);
  screens_.set(&boot_, display_);
}

void App::update() {
  tick_.update();
  console_.update();
  web_.update();
  screens_.update(display_);
}
