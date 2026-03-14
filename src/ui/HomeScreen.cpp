#include "ui/HomeScreen.h"
#include "ui/Layout.h"
#include "drivers/Display.h"
#include "services/ImageStore.h"
#include "services/SettingsService.h"
#include "services/ClockService.h"
#include "services/QuoteService.h"
#include "services/WeatherService.h"
#include <string.h>

// ── Colour helpers ────────────────────────────────────────────────────────
static constexpr uint8_t COL_BAR_R=18,  COL_BAR_G=22,  COL_BAR_B=34;
static constexpr uint8_t COL_SEP_R=48,  COL_SEP_G=58,  COL_SEP_B=88;
static constexpr uint8_t COL_PH_R=16,   COL_PH_G=18,   COL_PH_B=26;
static constexpr uint8_t COL_HNT_R=70,  COL_HNT_G=75,  COL_HNT_B=95;

static void hexToRgb(const char* hex, uint8_t& r, uint8_t& g, uint8_t& b) {
  if (!hex || strlen(hex) != 6) { r=0; g=0; b=0; return; }
  auto hv=[](char c)->uint8_t{
    if(c>='0'&&c<='9') return c-'0';
    if(c>='a'&&c<='f') return 10+c-'a';
    if(c>='A'&&c<='F') return 10+c-'A';
    return 0;
  };
  r=(hv(hex[0])<<4)|hv(hex[1]);
  g=(hv(hex[2])<<4)|hv(hex[3]);
  b=(hv(hex[4])<<4)|hv(hex[5]);
}

static void resolveColor(const char* hex, uint8_t r0, uint8_t g0, uint8_t b0,
                          uint8_t& r, uint8_t& g, uint8_t& b) {
  if (hex && strlen(hex) == 6) { hexToRgb(hex, r, g, b); }
  else                         { r=r0; g=g0; b=b0; }
}

void HomeScreen::setServices(ImageStore* store, SettingsService* settings,
                              ClockService* clock, QuoteService* quotes,
                              WeatherService* weather) {
  store_   = store;
  settings_= settings;
  clock_   = clock;
  quotes_  = quotes;
  weather_ = weather;
}

void HomeScreen::setStatusText(const char* mode, const char* ip) {
  if (mode) strncpy(modeText_, mode, sizeof(modeText_) - 1);
  if (ip)   strncpy(ipText_,   ip,   sizeof(ipText_)   - 1);
  statusDirty_ = true;
}

void HomeScreen::begin(Display& d) {
  d.clear();
  dirty_ = clockDirty_ = statusDirty_ = true;
  memset(lastTimeStr_, 0, sizeof(lastTimeStr_));
  drawTopBar_(d);
  drawImage_(d);
  drawStatusBar_(d);
}

void HomeScreen::update(Display& d) {
  if (millis() - lastClockMs_ >= 20000) {
    lastClockMs_ = millis();
    char t[12] = "";
    if (clock_) clock_->getTimeStr(t, sizeof(t));
    if (strcmp(t, lastTimeStr_) != 0) clockDirty_ = true;
  }

  if (settings_ && strcmp(settings_->statusMode(), "quote") == 0) {
    if (millis() - lastQuoteMs_ >= 30000) {
      lastQuoteMs_ = millis();
      if (quotes_) { quotes_->next(); statusDirty_ = true; }
    }
  }

  if (clockDirty_)  { drawTopBar_(d);    clockDirty_  = false; }
  if (dirty_)       { drawImage_(d);     dirty_       = false; }
  if (statusDirty_) { drawStatusBar_(d); statusDirty_ = false; }
}

void HomeScreen::drawTopBar_(Display& d) {
  const int16_t x=Layout::TOPBAR.x, y=Layout::TOPBAR.y;
  const int16_t w=Layout::TOPBAR.w, h=Layout::TOPBAR.h;

  uint8_t barR=COL_BAR_R, barG=COL_BAR_G, barB=COL_BAR_B;
  if (settings_) hexToRgb(settings_->topBarColor(), barR, barG, barB);
  d.fillBar(x, y, w, h, barR, barG, barB);
  d.hline(x, y+h-1, w, COL_SEP_R, COL_SEP_G, COL_SEP_B);

  if (settings_ && !settings_->clockEnabled()) {
    uint8_t dr,dg,db;
    resolveColor(settings_->dateColor(), 130,160,220, dr,dg,db);
    d.textCentered(x, y, w, h-1, 1, dr, dg, db, "ESP32 Display");
    memset(lastTimeStr_, 0, sizeof(lastTimeStr_));
    return;
  }

  char timeStr[12]="", dateStr[16]="";
  if (clock_) {
    clock_->getTimeStr(timeStr, sizeof(timeStr));
    clock_->getDateStr(dateStr, sizeof(dateStr));
  }
  strncpy(lastTimeStr_, timeStr, sizeof(lastTimeStr_)-1);

  const int16_t CHAR_W2=12, CHAR_H2=16;
  const int16_t CHAR_W1=6,  CHAR_H1=8;
  const int16_t half = w / 2;

  if (dateStr[0]) {
    uint8_t dr,dg,db;
    resolveColor(settings_?settings_->dateColor():nullptr, 130,160,220, dr,dg,db);
    d.textCentered(x+2, y, half-2, h-1, 1, dr, dg, db, dateStr);
  }

  for (int i=4; i<h-4; i++)
    d.pixel(x+half, y+i, COL_SEP_R, COL_SEP_G, COL_SEP_B);

  if (timeStr[0]) {
    uint8_t tr,tg,tb;
    resolveColor(settings_?settings_->timeColor():nullptr, 240,240,200, tr,tg,tb);
    int16_t tlen = (int16_t)strlen(timeStr);
    int16_t rx   = x + half + 2;
    int16_t rw   = half - 4;
    if (tlen * CHAR_W2 <= rw) {
      int16_t tw = tlen * CHAR_W2;
      int16_t tx = rx + (rw - tw) / 2;
      int16_t ty = y  + (h - CHAR_H2) / 2;
      d.setText(tx, ty, 2, tr, tg, tb);
    } else {
      int16_t tw = tlen * CHAR_W1;
      int16_t tx = rx + (rw - tw) / 2;
      int16_t ty = y  + (h - CHAR_H1) / 2;
      d.setText(tx, ty, 1, tr, tg, tb);
    }
    d.print(timeStr);
  }
}

void HomeScreen::drawImage_(Display& d) {
  if (!store_ || !store_->ready() || !settings_) {
    d.fillRect(Layout::CANVAS.x, Layout::CANVAS.y,
               Layout::CANVAS.w, Layout::CANVAS.h, COL_PH_R, COL_PH_G, COL_PH_B);
    d.textCentered(Layout::CANVAS.x,
                   Layout::CANVAS.y+Layout::CANVAS.h/2-4,
                   Layout::CANVAS.w, 8, 1, COL_HNT_R, COL_HNT_G, COL_HNT_B,
                   "Draw & upload an image");
    return;
  }
  const char* id = settings_->activeImage();
  if (!id || id[0]=='\0') {
    d.fillRect(Layout::CANVAS.x, Layout::CANVAS.y,
               Layout::CANVAS.w, Layout::CANVAS.h, COL_PH_R, COL_PH_G, COL_PH_B);
    const int16_t cx=Layout::CANVAS.x+Layout::CANVAS.w/2;
    const int16_t cy=Layout::CANVAS.y+Layout::CANVAS.h/2;
    for (int dx=-10; dx<=10; dx+=10)
      d.fillRect(cx+dx-2, cy-2, 5, 5, COL_SEP_R, COL_SEP_G, COL_SEP_B);
    d.textCentered(Layout::CANVAS.x, cy+12,
                   Layout::CANVAS.w, 8, 1, COL_HNT_R, COL_HNT_G, COL_HNT_B,
                   "No image selected");
    return;
  }
  store_->showById(id, &d);
}

void HomeScreen::drawStatusBar_(Display& d) {
  const int16_t x=Layout::STATUS.x, y=Layout::STATUS.y;
  const int16_t w=Layout::STATUS.w, h=Layout::STATUS.h;

  uint8_t staR, staG, staB;
  resolveColor(settings_?settings_->statusBarColor():nullptr, 14,18,28, staR,staG,staB);
  d.fillBar(x, y, w, h, staR, staG, staB);
  d.hline(x, y, w, COL_SEP_R, COL_SEP_G, COL_SEP_B);

  const char* mode = settings_ ? settings_->statusMode() : "wifi";

  if (strcmp(mode, "quote") == 0) {
    const char* q = (quotes_ && quotes_->count() > 0) ? quotes_->current() : nullptr;
    if (q && q[0]) {
      uint8_t qr,qg,qb;
      resolveColor(settings_?settings_->quoteColor():nullptr, 200,190,160, qr,qg,qb);
      d.textMedium(x, y, w, h, qr, qg, qb, q);
    } else {
      d.textCentered(x, y, w, h, 1, COL_HNT_R, COL_HNT_G, COL_HNT_B, "Add quotes in settings");
    }
    return;
  }

  if (strcmp(mode, "weather") == 0) {
    uint8_t wtr,wtg,wtb;
    resolveColor(settings_?settings_->weatherTempColor():nullptr, 255,220,100, wtr,wtg,wtb);
    if (weather_ && weather_->hasData()) {
      d.textMedium(x, y, w, h, wtr, wtg, wtb, weather_->line());
    } else {
      d.textCentered(x, y, w, h, 1, COL_HNT_R, COL_HNT_G, COL_HNT_B, "Fetching weather...");
    }
    return;
  }

  uint8_t mr,mg,mb, ir,ig,ib;
  resolveColor(settings_?settings_->modeColor():nullptr, 80,210,180, mr,mg,mb);
  resolveColor(settings_?settings_->ipColor():nullptr,  120,150,200, ir,ig,ib);
  const int16_t half = w / 2;
  d.textMedium(x + 2,        y, half - 2, h, mr, mg, mb, modeText_);
  for (int i = 5; i < h - 5; i++)
    d.pixel(x + half, y + i, COL_SEP_R, COL_SEP_G, COL_SEP_B);
  d.textMedium(x + half + 2, y, half - 2, h, ir, ig, ib, ipText_);
}