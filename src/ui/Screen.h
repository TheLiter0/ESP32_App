#pragma once

class Display;

class Screen {
public:
  virtual ~Screen() {}

  virtual void begin(Display& display) = 0;
  virtual void update(Display& display) = 0;
};
