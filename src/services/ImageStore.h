#pragma once

#include <Arduino.h>
#include <FS.h>

class Logger;
class Display;

struct ImageInfo {
  String   id;
  String   name;
  String   file;
  String   format;
  uint16_t width  = 0;
  uint16_t height = 0;
  uint32_t size   = 0;
};

class ImageStore {
public:
  bool begin(Logger* logger);
  bool ready() const { return ready_; }

  bool saveRgb565(const uint8_t* data,
                  size_t byteCount,
                  uint16_t width,
                  uint16_t height,
                  const char* displayName,
                  ImageInfo* outInfo = nullptr);

  // Load image from storage and draw it to the canvas area.
  // Returns false if id is not found or load fails.
  bool showById(const char* id, Display* display);
  bool deleteById(const char* id);
  int  rebuildIndex();  // scan /images/ and rewrite index.json

  String indexJson() const;

private:
  bool     ensureDir_(const char* path);
  bool     ensureTextFile_(const char* path, const char* defaultContent);
  uint32_t readNextId_();
  bool     writeNextId_(uint32_t nextValue);
  bool     appendIndexEntry_(const ImageInfo& info);
  String   buildIndexEntryJson_(const ImageInfo& info) const;
  static String escapeJson_(const String& s);
  bool     findEntry_(const char* id, ImageInfo& out) const;

  Logger* logger_ = nullptr;
  bool    ready_  = false;

  static constexpr const char* kConfigDir  = "/config";
  static constexpr const char* kImagesDir  = "/images";
  static constexpr const char* kIndexPath  = "/images/index.json";
  static constexpr const char* kNextIdPath = "/config/next_image_id.txt";
};
