#include "services/ImageStore.h"
#include "services/Logger.h"
#include "drivers/Display.h"
#include "ui/Layout.h"

#include <LittleFS.h>

bool ImageStore::begin(Logger* logger) {
  logger_ = logger;
  ready_ = false;


  bool ok = true;
  ok &= ensureDir_(kConfigDir);
  ok &= ensureDir_(kImagesDir);
  ok &= ensureTextFile_("/config/settings.json", "{}\n");
  ok &= ensureTextFile_("/config/quotes.json", "[]\n");
  ok &= ensureTextFile_(kIndexPath, "[]\n");
  ok &= ensureTextFile_(kNextIdPath, "1\n");

  ready_ = ok;

  if (logger_) {
    logger_->info(ok ? "[ImageStore] ready" : "[ImageStore] init incomplete");
  }

  return ready_;
}

bool ImageStore::saveRgb565(const uint8_t* data,
                            size_t byteCount,
                            uint16_t width,
                            uint16_t height,
                            const char* displayName,
                            ImageInfo* outInfo) {
  if (!ready_ || !data || byteCount == 0 || width == 0 || height == 0) {
    if (logger_) logger_->error("[ImageStore] saveRgb565 invalid args");
    return false;
  }

  const uint32_t expectedBytes = (uint32_t)width * (uint32_t)height * 2UL;
  if (byteCount != expectedBytes) {
    if (logger_) {
      String msg = String("[ImageStore] size mismatch expected=") + expectedBytes +
                   " got=" + (uint32_t)byteCount;
      logger_->warn(msg.c_str());
    }
  }

  const uint32_t idNum = readNextId_();
  if (idNum == 0) {
    if (logger_) logger_->error("[ImageStore] failed to read next image id");
    return false;
  }

  ImageInfo info;

  char idBuf[16];
  snprintf(idBuf, sizeof(idBuf), "img%04lu", (unsigned long)idNum);

  info.id = idBuf;
  info.name = (displayName && displayName[0]) ? String(displayName)
                                              : String("Image ") + idNum;
  info.file = String(kImagesDir) + "/" + info.id + ".rgb565";
  info.format = "rgb565";
  info.width = width;
  info.height = height;
  info.size = (uint32_t)byteCount;

  // Pre-check: make sure the filesystem has enough free space
  {
    size_t fsFree = LittleFS.totalBytes() - LittleFS.usedBytes();
    if (fsFree < byteCount + 4096) {
      if (logger_) {
        String msg = "[ImageStore] not enough space: free=" + String(fsFree) +
                     " need=" + String(byteCount);
        logger_->error(msg.c_str());
      }
      return false;
    }
  }

  File f = LittleFS.open(info.file, FILE_WRITE);
  if (!f) {
    if (logger_) logger_->error("[ImageStore] failed to open image file for write");
    return false;
  }

  // Write in 4KB chunks so a mid-write full-fs fails gracefully
  size_t written = 0;
  const size_t kChunk = 4096;
  while (written < byteCount) {
    size_t toWrite = min(kChunk, byteCount - written);
    size_t w = f.write(data + written, toWrite);
    written += w;
    if (w != toWrite) {
      if (logger_) logger_->error("[ImageStore] write stalled (fs full?)");
      break;
    }
  }
  f.close();

  if (written != byteCount) {
    if (logger_) logger_->error("[ImageStore] failed to write full image file");
    LittleFS.remove(info.file);
    return false;
  }

  if (!appendIndexEntry_(info)) {
    if (logger_) logger_->error("[ImageStore] failed to update index.json");
    LittleFS.remove(info.file);
    return false;
  }

  if (!writeNextId_(idNum + 1)) {
    if (logger_) logger_->warn("[ImageStore] saved image but failed to advance id counter");
  }

  if (outInfo) {
    *outInfo = info;
  }

  if (logger_) {
    String msg = String("[ImageStore] saved ") + info.id + " -> " + info.file +
                 " (" + info.size + " bytes)";
    logger_->info(msg.c_str());
  }

  return true;
}

String ImageStore::indexJson() const {
  if (!ready_ || !LittleFS.exists(kIndexPath)) return "[]";

  File f = LittleFS.open(kIndexPath, FILE_READ);
  if (!f) return "[]";

  String content = f.readString();
  f.close();

  content.trim();
  return content.length() ? content : String("[]");
}

bool ImageStore::ensureDir_(const char* path) {
  if (LittleFS.exists(path)) return true;

  const bool ok = LittleFS.mkdir(path);
  if (!ok && logger_) {
    String msg = String("[ImageStore] mkdir failed: ") + path;
    logger_->error(msg.c_str());
  }
  return ok;
}

bool ImageStore::ensureTextFile_(const char* path, const char* defaultContent) {
  if (LittleFS.exists(path)) return true;

  File f = LittleFS.open(path, FILE_WRITE);
  if (!f) {
    if (logger_) {
      String msg = String("[ImageStore] create file failed: ") + path;
      logger_->error(msg.c_str());
    }
    return false;
  }

  f.print(defaultContent);
  f.close();
  return true;
}

uint32_t ImageStore::readNextId_() {
  File f = LittleFS.open(kNextIdPath, FILE_READ);
  if (!f) return 0;

  String s = f.readString();
  f.close();
  s.trim();

  const uint32_t value = (uint32_t)s.toInt();
  return value > 0 ? value : 1;
}

bool ImageStore::writeNextId_(uint32_t nextValue) {
  File f = LittleFS.open(kNextIdPath, FILE_WRITE);
  if (!f) return false;

  f.print(nextValue);
  f.print('\n');
  f.close();
  return true;
}

bool ImageStore::appendIndexEntry_(const ImageInfo& info) {
  String current = indexJson();
  current.trim();

  const String entry = buildIndexEntryJson_(info);
  String next;

  if (current.length() == 0 || current == "[]") {
    next = String("[\n  ") + entry + "\n]\n";
  } else {
    const int closePos = current.lastIndexOf(']');
    if (closePos < 0) return false;

    String before = current.substring(0, closePos);
    before.trim();

    if (before.endsWith("[")) {
      next = String("[\n  ") + entry + "\n]\n";
    } else {
      next = before + ",\n  " + entry + "\n]\n";
    }
  }

  File f = LittleFS.open(kIndexPath, FILE_WRITE);
  if (!f) return false;

  f.print(next);
  f.close();
  return true;
}

String ImageStore::buildIndexEntryJson_(const ImageInfo& info) const {
  String json = "{";
  json += "\"id\":\"" + escapeJson_(info.id) + "\",";
  json += "\"name\":\"" + escapeJson_(info.name) + "\",";
  json += "\"file\":\"" + escapeJson_(info.file) + "\",";
  json += "\"format\":\"" + escapeJson_(info.format) + "\",";
  json += "\"width\":" + String(info.width) + ",";
  json += "\"height\":" + String(info.height) + ",";
  json += "\"size\":" + String(info.size);
  json += "}";
  return json;
}

String ImageStore::escapeJson_(const String& s) {
  String out;
  out.reserve(s.length() + 8);

  for (size_t i = 0; i < s.length(); ++i) {
    const char c = s[i];

    if (c == '\\' || c == '"') {
      out += '\\';
      out += c;
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else if (c == '\t') {
      out += "\\t";
    } else {
      out += c;
    }
  }

  return out;
}
bool ImageStore::showById(const char* id, Display* display) {
  if (!ready_ || !id || !display) return false;

  ImageInfo info;
  if (!findEntry_(id, info)) {
    if (logger_) logger_->error(("[ImageStore] showById: not found: " + String(id)).c_str());
    return false;
  }

  File f = LittleFS.open(info.file, FILE_READ);
  if (!f) {
    if (logger_) logger_->error(("[ImageStore] showById: open failed: " + info.file).c_str());
    return false;
  }

  const uint32_t expectedBytes = (uint32_t)info.width * info.height * 2;
  uint8_t* buf = (uint8_t*)malloc(expectedBytes);
  if (!buf) {
    f.close();
    if (logger_) logger_->error("[ImageStore] showById: malloc failed");
    return false;
  }

  const size_t bytesRead = f.read(buf, expectedBytes);
  f.close();

  if (bytesRead != expectedBytes) {
    free(buf);
    if (logger_) logger_->error("[ImageStore] showById: read truncated");
    return false;
  }

  // Import here to avoid circular header dependency
  display->drawRGB565(Layout::CANVAS.x, Layout::CANVAS.y,
                      info.width, info.height,
                      (const uint16_t*)buf);
  free(buf);

  if (logger_) logger_->info(("[ImageStore] showById: displayed " + String(id)).c_str());
  return true;
}

bool ImageStore::findEntry_(const char* id, ImageInfo& out) const {
  // Walk the index JSON and find the entry matching "id":"<id>"
  String index = indexJson();
  String needle = String("\"id\":\"") + id + "\"";
  int pos = index.indexOf(needle);
  if (pos < 0) return false;

  // Backtrack to the opening { of this entry
  int start = index.lastIndexOf('{', pos);
  if (start < 0) return false;
  int end = index.indexOf('}', pos);
  if (end < 0) return false;

  String entry = index.substring(start, end + 1);

  auto strVal = [&](const char* key) -> String {
    String k = String("\"") + key + "\":\"";
    int s = entry.indexOf(k);
    if (s < 0) return "";
    s += k.length();
    int e = entry.indexOf('"', s);
    return e < 0 ? "" : entry.substring(s, e);
  };
  auto intVal = [&](const char* key) -> int {
    String k = String("\"") + key + "\":";
    int s = entry.indexOf(k);
    if (s < 0) return 0;
    return entry.substring(s + k.length()).toInt();
  };

  out.id     = strVal("id");
  out.name   = strVal("name");
  out.file   = strVal("file");
  out.format = strVal("format");
  out.width  = (uint16_t)intVal("width");
  out.height = (uint16_t)intVal("height");
  out.size   = (uint32_t)intVal("size");

  return out.id.length() > 0 && out.file.length() > 0;
}

bool ImageStore::deleteById(const char* id) {
  if (!ready_ || !id) return false;

  // Remove the actual image file
  ImageInfo info;
  bool found = findEntry_(id, info);
  if (found && LittleFS.exists(info.file)) {
    LittleFS.remove(info.file);
  }

  // Rebuild the index by collecting all entries that do NOT match this id.
  // Rebuilding from scratch avoids any leading/trailing comma bugs.
  String index = indexJson();
  String needle = String("\"id\":\"") + id + "\"";

  String newIndex = "[";
  bool   first    = true;
  int    searchFrom = 0;

  while (true) {
    int entryStart = index.indexOf('{', searchFrom);
    if (entryStart < 0) break;
    int entryEnd = index.indexOf('}', entryStart);
    if (entryEnd < 0) break;

    String entry = index.substring(entryStart, entryEnd + 1);
    searchFrom = entryEnd + 1;

    if (entry.indexOf(needle) >= 0) continue;  // skip deleted entry

    if (!first) newIndex += ",";
    newIndex += "\n  " + entry;
    first = false;
  }

  newIndex += first ? "]\n" : "\n]\n";

  File f = LittleFS.open(kIndexPath, FILE_WRITE);
  if (!f) return false;
  f.print(newIndex);
  f.close();

  if (logger_) logger_->info(("[ImageStore] deleted " + String(id)).c_str());
  return true;
}

int ImageStore::rebuildIndex() {
  if (!ready_) return -1;

  // Scan /images/ for all .rgb565 files and rebuild index + next_id from scratch
  File dir = LittleFS.open(kImagesDir);
  if (!dir || !dir.isDirectory()) return -1;

  String newIndex = "[";
  bool   first    = true;
  int    count    = 0;
  uint32_t maxId  = 0;

  File entry = dir.openNextFile();
  while (entry) {
    String name = String(entry.name());
    // entry.name() returns just the filename on some versions, full path on others
    if (!name.endsWith(".rgb565")) {
      entry = dir.openNextFile();
      continue;
    }

    // Extract id from filename e.g. "img0003.rgb565" -> "img0003"
    // entry.name() may return "/images/img0003.rgb565" or just "img0003.rgb565"
    int slashPos = name.lastIndexOf('/');
    String filename = slashPos >= 0 ? name.substring(slashPos + 1) : name;
    String id = filename.substring(0, filename.length() - 7); // strip ".rgb565"

    // Extract the numeric part to track max id
    String numPart = id.substring(3); // strip "img"
    uint32_t idNum = (uint32_t)numPart.toInt();
    if (idNum > maxId) maxId = idNum;

    uint32_t fileSize = entry.size();
    // Infer dimensions: we know width=240, height=fileSize/2/240
    uint16_t w = 240;
    uint16_t h = (fileSize > 0) ? (uint16_t)(fileSize / 2 / w) : 0;

    String filePath = String(kImagesDir) + "/" + filename;
    String entryJson = "{";
    entryJson += "\"id\":\"" + id + "\",";
    entryJson += "\"name\":\"Image " + numPart + "\",";
    entryJson += "\"file\":\"" + filePath + "\",";
    entryJson += "\"format\":\"rgb565\",";
    entryJson += "\"width\":" + String(w) + ",";
    entryJson += "\"height\":" + String(h) + ",";
    entryJson += "\"size\":" + String(fileSize);
    entryJson += "}";

    if (!first) newIndex += ",";
    newIndex += "\n  " + entryJson;
    first = false;
    count++;

    entry = dir.openNextFile();
  }

  newIndex += first ? "]\n" : "\n]\n";

  // Write repaired index
  File f = LittleFS.open(kIndexPath, FILE_WRITE);
  if (!f) return -1;
  f.print(newIndex);
  f.close();

  // Update next_id counter so new saves don't collide
  writeNextId_(maxId + 1);

  if (logger_) {
    logger_->info(("[ImageStore] rebuilt index: " + String(count) + " images").c_str());
  }

  return count;
}
