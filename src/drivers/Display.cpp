#include "src/drivers/Display.h"
#include <SPI.h>

void Display::begin() {
  tft_.initR(INITR_GREENTAB);
  tft_.setRotation(1);
  tft_.fillScreen(ST77XX_BLACK);
  tft_.setTextWrap(false);
}

void Display::clear() {
  tft_.fillScreen(ST77XX_BLACK);
}

uint16_t Display::rgb565_(uint8_t r, uint8_t g, uint8_t b) {
  return tft_.color565(r, g, b);
}

void Display::setText(int16_t x, int16_t y, uint8_t size, uint8_t r, uint8_t g, uint8_t b) {
  tft_.setCursor(x, y);
  tft_.setTextSize(size);
  tft_.setTextColor(rgb565_(r, g, b));
}

void Display::print(const char* s) {
  tft_.print(s);
}
