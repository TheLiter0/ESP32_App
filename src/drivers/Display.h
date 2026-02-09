#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

class Display {
public:
  void begin();
  void clear();
  void setText(int16_t x, int16_t y, uint8_t size, uint8_t r, uint8_t g, uint8_t b);
  void print(const char* s);

private:
  static constexpr int TFT_CS  = 5;
  static constexpr int TFT_DC  = 16;
  static constexpr int TFT_RST = 17;

  Adafruit_ST7735 tft_ = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

  uint16_t rgb565_(uint8_t r, uint8_t g, uint8_t b);
};
