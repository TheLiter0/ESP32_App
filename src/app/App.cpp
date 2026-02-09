#include "App.h"

void App::begin() {
  logger_.begin(115200);
  logger_.info("Boot");

  tick_.begin();

  display_.begin();
  display_.clear();

  display_.setText(10, 10, 2, 255, 255, 255);
  display_.print("READY");

  console_.begin(&logger_, &display_);
}

void App::update() {
  tick_.update();
  console_.update();
}
