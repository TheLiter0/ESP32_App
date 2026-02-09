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

  char cmd[20];
  if (!nextToken_(p, cmd, sizeof(cmd))) return;

  if (strcmp(cmd, "help") == 0) {
    logger_->info("Commands:");
    logger_->info("  selftest");
    logger_->info("  help");
    logger_->info("  cls");
    logger_->info("  ready");
    logger_->info("  border");
    logger_->info("  layout   (draw region separator lines)");
    logger_->info("  text x y size r g b msg_with_underscores");
    logger_->info("  rect x y w h r g b");
    logger_->info("  fillrect x y w h r g b");
    logger_->info("  line x0 y0 x1 y1 r g b");
    logger_->info("  px x y r g b");
    logger_->info("  top msg_with_underscores");
    logger_->info("  status msg_with_underscores");
    logger_->info("  canvas_cls");
    logger_->info("  canvas_px x y r g b   (x 0-127, y 0-119)");
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
  if (strcmp(cmd, "selftest") == 0) {
  // Full-screen sanity test (fast, deterministic)
  display_->clear();
  // separators (match your current layout y values)
  display_->line(0, 20, 159, 20, 255, 255, 255);
  display_->line(0, 107, 159, 107, 255, 255, 255);

  // top + status
  display_->textInBox(Layout::TOPBAR.x, Layout::TOPBAR.y, Layout::TOPBAR.w, Layout::TOPBAR.h,
                      1, 255, 255, 255, "TOP BAR OK");
  display_->textInBox(Layout::STATUS.x, Layout::STATUS.y, Layout::STATUS.w, Layout::STATUS.h,
                      1, 255, 255, 255, "STATUS OK");

  // clear canvas only
  display_->clearRect(Layout::CANVAS.x, Layout::CANVAS.y, Layout::CANVAS.w, Layout::CANVAS.h);

  // RGB pixels at canvas (5,5)(6,5)(7,5)
  display_->pixel(Layout::CANVAS.x + 5, Layout::CANVAS.y + 5, 255, 0, 0);
  display_->pixel(Layout::CANVAS.x + 6, Layout::CANVAS.y + 5, 0, 255, 0);
  display_->pixel(Layout::CANVAS.x + 7, Layout::CANVAS.y + 5, 0, 0, 255);

  // border
  int w = display_->width();
  int h = display_->height();
  display_->rect(0, 0, w, h, 255, 255, 255);
  display_->fillRect(0, 0, 6, 6, 0, 255, 0);
  display_->fillRect(w - 6, 0, 6, 6, 255, 0, 0);
  display_->fillRect(0, h - 6, 6, 6, 0, 0, 255);
  display_->fillRect(w - 6, h - 6, 6, 6, 0, 255, 255); // cyan BR to match your mapping

  logger_->info("Selftest done");
  return;
}

  if (strcmp(cmd, "border") == 0) {
    int w = display_->width();
    int h = display_->height();
    display_->rect(0, 0, w, h, 255, 255, 255);
    display_->fillRect(0, 0, 6, 6, 0, 255, 0);
    display_->fillRect(w - 6, 0, 6, 6, 255, 0, 0);
    display_->fillRect(0, h - 6, 6, 6, 0, 0, 255);
    display_->fillRect(w - 6, h - 6, 6, 6, 255, 255, 0);
    return;
  }
  if (strcmp(cmd, "layout") == 0) {
  // Draw the two horizontal separators: y=20 and y=140
  display_->line(0, 20, 159, 20, 255, 255, 255);     // between top and canvas
  display_->line(0, 107, 159, 107, 255, 255, 255);   // between canvas and status (outside status)


  return;
}


  // Region text commands
  if (strcmp(cmd, "top") == 0 || strcmp(cmd, "status") == 0) {
    char msg[120];
    if (!nextToken_(p, msg, sizeof(msg))) return;

    for (size_t i = 0; msg[i]; i++) if (msg[i] == '_') msg[i] = ' ';

    const RectI box = (strcmp(cmd, "top") == 0) ? Layout::TOPBAR : Layout::STATUS;
    display_->textInBox(box.x, box.y, box.w, box.h, 1, 255, 255, 255, msg);
    return;
  }

  if (strcmp(cmd, "canvas_cls") == 0) {
    const RectI c = Layout::CANVAS;
    display_->clearRect(c.x, c.y, c.w, c.h);
    return;
  }

  if (strcmp(cmd, "canvas_px") == 0) {
    char t[24];
    int x,y,r,g,b;

    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, x)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, y)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, r)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, g)) return;
    if (!nextToken_(p, t, sizeof(t)) || !parseInt_(t, b)) return;

    const RectI c = Layout::CANVAS;
    // x,y are relative to canvas origin:
    display_->pixel((int16_t)(c.x + x), (int16_t)(c.y + y), (uint8_t)r, (uint8_t)g, (uint8_t)b);
    return;
  }

  // Existing drawing commands (full-screen coordinates)
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
