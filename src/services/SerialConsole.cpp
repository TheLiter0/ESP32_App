#include "src/services/SerialConsole.h"
#include <Arduino.h>
#include <cstring>
#include <cstdlib>

void SerialConsole::begin(Logger* logger, Display* display) {
  logger_ = logger;
  display_ = display;
  len_ = 0;
  memset(line_, 0, sizeof(line_));
}

void SerialConsole::update() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      line_[len_] = '\0';
      if (len_ > 0) handleLine_(line_);
      len_ = 0;
      continue;
    }

    if (len_ < CONSOLE_LINE_MAX - 1) line_[len_++] = c;
  }
}

void SerialConsole::handleLine_(char* s) {
  char* p = s;

  char cmd[16];
  if (!nextToken_(p, cmd, sizeof(cmd))) return;

  if (strcmp(cmd, "help") == 0) {
    logger_->info("Commands:");
    logger_->info("  help");
    logger_->info("  cls");
    logger_->info("  ready");
    logger_->info("  text x y size r g b msg_with_underscores");
    logger_->info("  rect x y w h r g b");
    logger_->info("  fillrect x y w h r g b");
    logger_->info("  line x0 y0 x1 y1 r g b");
    logger_->info("  px x y r g b");
    logger_->info("  border   (draw 1px border + corners)");
    return;
  }

  if (strcmp(cmd, "cls") == 0) {
    display_->clear();
    return;
  }

  if (strcmp(cmd, "ready") == 0) {
    display_->setText(10, 10, 2, 255, 255, 255);
    display_->print("READY");
    return;
  }

  if (strcmp(cmd, "border") == 0) {
    int w = display_->width();
    int h = display_->height();
    display_->rect(0, 0, w, h, 255, 255, 255);
    display_->fillRect(0, 0, 6, 6, 0, 255, 0);                 // TL
    display_->fillRect(w - 6, 0, 6, 6, 255, 0, 0);             // TR
    display_->fillRect(0, h - 6, 6, 6, 0, 0, 255);             // BL
    display_->fillRect(w - 6, h - 6, 6, 6, 255, 255, 0);       // BR
    return;
  }

  if (strcmp(cmd, "text") == 0) {
    char t[24];
    int x,y,size,r,g,b;
    char msg[96];

    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, x)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, y)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, size)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, r)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, g)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, b)) return;
    if (!nextToken_(p, msg, sizeof(msg))) return;

    for (size_t i = 0; msg[i]; i++) if (msg[i] == '_') msg[i] = ' ';

    display_->setText((int16_t)x, (int16_t)y, (uint8_t)size, (uint8_t)r, (uint8_t)g, (uint8_t)b);
    display_->print(msg);
    return;
  }

  if (strcmp(cmd, "rect") == 0 || strcmp(cmd, "fillrect") == 0) {
    char t[24];
    int x,y,w,h,r,g,b;

    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, x)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, y)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, w)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, h)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, r)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, g)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, b)) return;

    if (strcmp(cmd, "rect") == 0) display_->rect(x,y,w,h,r,g,b);
    else display_->fillRect(x,y,w,h,r,g,b);
    return;
  }

  if (strcmp(cmd, "line") == 0) {
    char t[24];
    int x0,y0,x1,y1,r,g,b;

    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, x0)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, y0)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, x1)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, y1)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, r)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, g)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, b)) return;

    display_->line(x0,y0,x1,y1,r,g,b);
    return;
  }

  if (strcmp(cmd, "px") == 0) {
    char t[24];
    int x,y,r,g,b;

    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, x)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, y)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, r)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, g)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, b)) return;

    display_->pixel(x,y,r,g,b);
    return;
  }

  logger_->warn("Unknown command. Type: help");
}

bool SerialConsole::nextToken_(char*& p, char* out, size_t outMax) {
  while (*p == ' ' || *p == '\t') p++;
  if (*p == '\0') return false;

  size_t i = 0;
  while (*p && *p != ' ' && *p != '\t') {
    if (i + 1 < outMax) out[i++] = *p;
    p++;
  }
  out[i] = '\0';
  return true;
}

bool SerialConsole::parseInt_(const char* s, int& out) {
  char* end = nullptr;
  long v = strtol(s, &end, 10);
  if (end == s) return false;
  out = (int)v;
  return true;
}
