#include "src/ui/ScreenManager.h"
#include "src/drivers/Display.h"

void ScreenManager::begin(Display&) {}

void ScreenManager::set(Screen* s, Display& display) {
  current_ = s;
  if (current_) current_->begin(display);
}

void ScreenManager::update(Display& display) {
  if (current_) current_->update(display);
}
