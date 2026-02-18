#include "services/Logger.h"
#include "drivers/Display.h"
#include "ui/Layout.h"

class SerialConsole {
public:
  void begin(Logger* logger, Display* display);
  void update();

private:
  Logger* logger_ = nullptr;
  Display* display_ = nullptr;

  static constexpr size_t CONSOLE_LINE_MAX = 160;
  char line_[CONSOLE_LINE_MAX];
  size_t len_ = 0;

  void handleLine_(char* s);
  static bool nextToken_(char*& p, char* out, size_t outMax);
  static bool parseInt_(const char* s, int& out);
};
