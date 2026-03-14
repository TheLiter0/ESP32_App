#include "services/QuoteService.h"
#include <LittleFS.h>

bool QuoteService::begin(Logger* logger) {
  logger_ = logger;
  bool ok = load_();
  if (logger_) {
    logger_->info(ok ? ("[Quotes] loaded " + String(count_) + " quotes").c_str()
                     : "[Quotes] no quotes found");
  }
  return ok;
}

void QuoteService::next() {
  if (count_ == 0) return;
  index_ = (index_ + 1) % count_;
  strncpy(current_, quotes_[index_], sizeof(current_) - 1);
  current_[sizeof(current_) - 1] = '\0';
}

bool QuoteService::load_() {
  count_ = 0;
  current_[0] = '\0';

  if (!LittleFS.exists(kPath)) return false;
  File f = LittleFS.open(kPath, FILE_READ);
  if (!f) return false;
  String json = f.readString();
  f.close();

  // Parse ["quote1","quote2",...] — simple scan without ArduinoJson
  int searchFrom = 0;
  while (count_ < kMax) {
    int qs = json.indexOf('"', searchFrom);
    if (qs < 0) break;
    int qe = qs + 1;
    // find closing quote, skipping escaped ones
    while (qe < (int)json.length()) {
      if (json[qe] == '"' && json[qe-1] != '\\') break;
      qe++;
    }
    if (qe >= (int)json.length()) break;

    String quote = json.substring(qs + 1, qe);
    // Replace \n with space, unescape \"
    quote.replace("\\n", " ");
    quote.replace("\\\"", "\"");
    if (quote.length() == 0) { searchFrom = qe + 1; continue; }

    strncpy(quotes_[count_], quote.c_str(), kMaxLen - 1);
    quotes_[count_][kMaxLen - 1] = '\0';
    count_++;
    searchFrom = qe + 1;
  }

  if (count_ > 0) {
    strncpy(current_, quotes_[0], sizeof(current_) - 1);
    current_[sizeof(current_) - 1] = '\0';
  }
  return count_ > 0;
}