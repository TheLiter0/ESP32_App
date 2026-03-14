#pragma once
#include <Arduino.h>
#include "services/Logger.h"

// Loads quotes from /config/quotes.json and cycles through them.
// quotes.json format: ["Quote one", "Quote two", ...]
class QuoteService {
public:
  bool begin(Logger* logger);

  // Returns the current quote (empty string if none loaded)
  const char* current() const { return current_; }

  // Advance to the next quote
  void next();

  int count() const { return count_; }

private:
  bool load_();

  Logger* logger_  = nullptr;
  int     count_   = 0;
  int     index_   = 0;
  char    current_[120] = "";

  // Store up to 30 quotes, 100 chars each
  static constexpr int kMax     = 30;
  static constexpr int kMaxLen  = 100;
  char quotes_[kMax][kMaxLen];

  static constexpr const char* kPath = "/config/quotes.json";
};