#include "drivers/Display.h"
#include <SPI.h>

// Gamma 2.2 LUT: darkens midtones so colors on ST7789 match what you see on screen.
// output = (input/255)^2.2 * 255
static const uint8_t GAMMA[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
    3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,   6,
    6,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,  10,  11,  11,  11,  12,
   12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,
   20,  20,  21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  28,  28,  29,
   30,  30,  31,  32,  33,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,
   42,  43,  43,  44,  45,  46,  47,  48,  49,  49,  50,  51,  52,  53,  54,  55,
   56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,
   73,  74,  75,  76,  77,  78,  79,  81,  82,  83,  84,  85,  87,  88,  89,  90,
   91,  93,  94,  95,  97,  98,  99, 100, 102, 103, 105, 106, 107, 109, 110, 111,
  113, 114, 116, 117, 119, 120, 121, 123, 124, 126, 127, 129, 130, 132, 133, 135,
  137, 138, 140, 141, 143, 145, 146, 148, 149, 151, 153, 154, 156, 158, 159, 161,
  163, 165, 166, 168, 170, 172, 173, 175, 177, 179, 181, 182, 184, 186, 188, 190,
  192, 194, 196, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221,
  223, 225, 227, 229, 231, 234, 236, 238, 240, 242, 244, 246, 248, 251, 253, 255
};

void Display::begin() {
  tft_.init(240, 240);
  tft_.setRotation(0);
  tft_.fillScreen(0x0000);
  tft_.setTextWrap(false);
}

void Display::clear() {
  tft_.fillScreen(0x0000);
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

void Display::drawRGB565(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* buf) {
  // Apply gamma correction and byteswap into a small scanline buffer
  // to avoid allocating the full frame at once
  const int16_t SCANLINE = w;
  uint16_t line[SCANLINE];

  for (int16_t row = 0; row < h; row++) {
    const uint16_t* src = buf + row * w;
    for (int16_t col = 0; col < w; col++) {
      uint16_t px = src[col];
      // Byteswap (big-endian from JS → little-endian for drawRGBBitmap)
      px = (px >> 8) | (px << 8);
      // Extract RGB565 channels
      uint8_t r5 = (px >> 11) & 0x1F;
      uint8_t g6 = (px >>  5) & 0x3F;
      uint8_t b5 =  px        & 0x1F;
      // Expand to 8-bit, apply gamma, re-pack to 565
      uint8_t r8 = GAMMA[(r5 << 3) | (r5 >> 2)];
      uint8_t g8 = GAMMA[(g6 << 2) | (g6 >> 4)];
      uint8_t b8 = GAMMA[(b5 << 3) | (b5 >> 2)];
      line[col] = ((r8 >> 3) << 11) | ((g8 >> 2) << 5) | (b8 >> 3);
    }
    tft_.drawRGBBitmap(x, y + row, line, w, 1);
  }
}

int16_t Display::width()  const { return tft_.width(); }
int16_t Display::height() const { return tft_.height(); }

void Display::clearRect(int16_t x, int16_t y, int16_t w, int16_t h) {
  tft_.fillRect(x, y, w, h, 0x0000);
}

void Display::textInBox(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t size,
                        uint8_t r, uint8_t g, uint8_t b, const char* msg) {
  clearRect(x, y, w, h);
  setText(x + 2, y + 2, size, r, g, b);
  print(msg);
}
void Display::textCentered(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint8_t size, uint8_t r, uint8_t g, uint8_t b,
                            const char* msg) {
  if (!msg || msg[0] == '\0') return;

  // Adafruit GFX size-1 = 6px wide, 8px tall per char
  const int16_t charW = 6 * size;
  const int16_t charH = 8 * size;
  int16_t textW = (int16_t)strlen(msg) * charW;
  if (textW > w) textW = w;  // clamp

  int16_t tx = x + (w - textW) / 2;
  int16_t ty = y + (h - charH) / 2;

  tft_.setCursor(tx, ty);
  tft_.setTextSize(size);
  tft_.setTextColor(rgb565_(r, g, b));
  tft_.print(msg);
}

void Display::fillBar(int16_t x, int16_t y, int16_t w, int16_t h,
                      uint8_t r, uint8_t g, uint8_t b) {
  tft_.fillRect(x, y, w, h, rgb565_(r, g, b));
  // 1px darker top edge for subtle depth
  uint8_t dr = r > 30 ? r - 30 : 0;
  uint8_t dg = g > 30 ? g - 30 : 0;
  uint8_t db = b > 30 ? b - 30 : 0;
  tft_.drawFastHLine(x, y, w, rgb565_(dr, dg, db));
}

void Display::hline(int16_t x, int16_t y, int16_t w,
                    uint8_t r, uint8_t g, uint8_t b) {
  tft_.drawFastHLine(x, y, w, rgb565_(r, g, b));
}

void Display::textWrapped(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint8_t size, uint8_t r, uint8_t g, uint8_t b,
                           const char* msg) {
  if (!msg || msg[0] == '\0') return;

  const int16_t charW  = 6 * size;
  const int16_t charH  = 8 * size;
  const int16_t maxCols = w / charW;

  int len = (int)strlen(msg);

  if (len <= maxCols) {
    // Fits on one line — centre it normally
    int16_t tx = x + (w - len * charW) / 2;
    int16_t ty = y + (h - charH) / 2;
    tft_.setCursor(tx, ty);
    tft_.setTextSize(size);
    tft_.setTextColor(rgb565_(r, g, b));
    tft_.print(msg);
    return;
  }

  // Need to wrap — find a good break point near the middle
  int breakAt = maxCols;
  // Try to break at a space near the middle of the string
  int mid = len / 2;
  int best = maxCols;
  for (int i = mid; i >= 0; i--) {
    if (msg[i] == ' ') { best = i; break; }
  }
  if (best == maxCols) {
    // No space found going left, try right
    for (int i = mid; i < len && i < maxCols; i++) {
      if (msg[i] == ' ') { best = i; break; }
    }
  }
  breakAt = (best < maxCols) ? best : maxCols;

  // Line 1
  char line1[48]; int l1 = breakAt < 47 ? breakAt : 47;
  strncpy(line1, msg, l1); line1[l1] = '\0';

  // Line 2
  const char* rest = msg + breakAt;
  while (*rest == ' ') rest++;   // skip leading space
  char line2[48]; strncpy(line2, rest, 47); line2[47] = '\0';

  int l2 = (int)strlen(line2);

  // Two lines: stack them vertically centred in the box
  const int16_t totalH = charH * 2 + 1;
  int16_t ty1 = y + (h - totalH) / 2;
  int16_t ty2 = ty1 + charH + 1;

  tft_.setTextSize(size);
  tft_.setTextColor(rgb565_(r, g, b));

  int16_t tx1 = x + (w - l1 * charW) / 2;
  tft_.setCursor(tx1, ty1);
  tft_.print(line1);

  if (l2 > 0) {
    int16_t tx2 = x + (w - l2 * charW) / 2;
    tft_.setCursor(tx2, ty2);
    tft_.print(line2);
  }
}

// "Size 1.5" — renders each char at size-1 but with 9px pitch instead of 6px.
// This gives roughly 50% more space per char without the full size-2 jump.
// The glyph itself is still 6×8 px, just with 3px gap between chars.
void Display::textMedium(int16_t x, int16_t y, int16_t w, int16_t h,
                         uint8_t r, uint8_t g, uint8_t b, const char* msg) {
  if (!msg || msg[0] == '\0') return;
  const int16_t PITCH = 9;   // px per char (size-1 glyph = 6px + 3px gap)
  const int16_t CHARH = 8;   // size-1 glyph height

  int len = (int)strlen(msg);
  int16_t textW = len * PITCH;
  // Centre in the region
  int16_t tx = x + (w - textW) / 2;
  int16_t ty = y + (h - CHARH) / 2;

  tft_.setTextSize(1);
  tft_.setTextColor(rgb565_(r, g, b));

  for (int i = 0; i < len; i++) {
    tft_.setCursor(tx + i * PITCH, ty);
    tft_.print(msg[i]);
  }
}

// Scrolling marquee — draws text offset by offsetX pixels, clipped to [x, x+w].
// Caller advances offsetX by 1–2px per update tick.
// When offsetX >= full text width + gap, the caller should reset it to 0.
void Display::textMarquee(int16_t x, int16_t y, int16_t w, int16_t h,
                          uint8_t r, uint8_t g, uint8_t b,
                          const char* msg, int16_t offsetX) {
  if (!msg || msg[0] == '\0') return;
  const int16_t PITCH = 9;
  const int16_t CHARH = 8;
  int16_t ty = y + (h - CHARH) / 2;
  int len = (int)strlen(msg);

  tft_.setTextSize(1);
  tft_.setTextColor(rgb565_(r, g, b));

  for (int i = 0; i < len; i++) {
    int16_t cx = x + i * PITCH - offsetX;
    // Only draw if the character is within the clip region
    if (cx + PITCH < x) continue;
    if (cx >= x + w)    break;
    tft_.setCursor(cx, ty);
    tft_.print(msg[i]);
  }
}