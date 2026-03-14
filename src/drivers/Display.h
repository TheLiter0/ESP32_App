#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

class Display {
public:
  void begin();
  void clear();

  void setText(int16_t x, int16_t y, uint8_t size, uint8_t r, uint8_t g, uint8_t b);
  void print(const char* s);

  void pixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
  void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t r, uint8_t g, uint8_t b);
  void rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r, uint8_t g, uint8_t b);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r, uint8_t g, uint8_t b);

  void drawRGB565(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* buf);

  int16_t width()  const;
  int16_t height() const;

  void clearRect(int16_t x, int16_t y, int16_t w, int16_t h);

  // Clears region and draws text at top-left
  void textInBox(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t size,
                 uint8_t r, uint8_t g, uint8_t b, const char* msg);

  // Text centered horizontally and vertically — does NOT clear background
  void textCentered(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t size,
                    uint8_t r, uint8_t g, uint8_t b, const char* msg);

  // Filled bar with 1px darker top edge
  void fillBar(int16_t x, int16_t y, int16_t w, int16_t h,
               uint8_t r, uint8_t g, uint8_t b);

  // 1px horizontal separator line
  void hline(int16_t x, int16_t y, int16_t w, uint8_t r, uint8_t g, uint8_t b);

  // Text with word-wrap to two lines, centred in the box
  void textWrapped(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t size,
                   uint8_t r, uint8_t g, uint8_t b, const char* msg);

  // "Size 1.5" — size-1 glyphs drawn with extra inter-character spacing
  // so the text is bigger than size-1 but doesn't explode like size-2.
  // Each char is rendered individually at a 9px pitch (vs normal 6px).
  void textMedium(int16_t x, int16_t y, int16_t w, int16_t h,
                  uint8_t r, uint8_t g, uint8_t b, const char* msg);

  // Scrolling marquee for one line of text.
  // Call with increasing offsetX each frame (wraps automatically).
  // Clips drawing to [x, x+w].
  void textMarquee(int16_t x, int16_t y, int16_t w, int16_t h,
                   uint8_t r, uint8_t g, uint8_t b,
                   const char* msg, int16_t offsetX);

private:
  static constexpr int TFT_CS  = 5;
  static constexpr int TFT_DC  = 2;
  static constexpr int TFT_RST = 4;

  Adafruit_ST7789 tft_ = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

  uint16_t rgb565_(uint8_t r, uint8_t g, uint8_t b);
};