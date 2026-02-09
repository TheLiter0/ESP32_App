#include "src/drivers/Display.h"
#include <SPI.h>

void Display::begin() {
  // If you still see edge bars, try INITR_REDTAB or INITR_GREENTAB here.
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

void Display::pixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
  tft_.drawPixel(x, y, rgb565_(r, g, b));
}

void Display::line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t r, uint8_t g, uint8_t b) {
  tft_.drawLine(x0, y0, x1, y1, rgb565_(r, g, b));
}

void Display::rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r, uint8_t g, uint8_t b) {
  tft_.drawRect(x, y, w, h, rgb565_(r, g, b));
}

void Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r, uint8_t g, uint8_t b) {
  tft_.fillRect(x, y, w, h, rgb565_(r, g, b));
}

int16_t Display::width() const { return tft_.width(); }
int16_t Display::height() const { return tft_.height(); }
