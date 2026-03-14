#include "app/App.h"
#include "secrets.h"

static const int SWITCH_PIN = 27;
static const int OUTPUT_A   = 22;
static const int OUTPUT_B   = 21;

void App::begin() {
  logger_.begin(115200);
  display_.begin();
  fs_.begin(&logger_, true);
  imageStore_.begin(&logger_);
  settings_.begin(&logger_);
  quotes_.begin(&logger_);
  weather_.begin(&logger_);

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(OUTPUT_A, OUTPUT);
  pinMode(OUTPUT_B, OUTPUT);
  digitalWrite(OUTPUT_A, LOW);
  digitalWrite(OUTPUT_B, LOW);

  int state = digitalRead(SWITCH_PIN);
  if (state == LOW) {
    digitalWrite(OUTPUT_A, HIGH);
    wifiKey_ = true;
    logger_.info("[App] GPIO27 grounded -> Station mode");
  } else {
    digitalWrite(OUTPUT_B, HIGH);
    wifiKey_ = false;
    logger_.info("[App] GPIO27 open -> Hotspot mode");
  }

  boot_.setModeText(wifiKey_ ? "Mode: WiFi" : "Mode: Hotspot");
  boot_.setIpText("IP: waiting...");
  boot_.setShowApInfo(!wifiKey_);
  if (!wifiKey_) {
    boot_.setApSsidText("SSID: ESP32_Draw");
    boot_.setApPasswordText("PASS: draw1234");
  }

  wifi_.begin(&logger_);

  static const char* ssids[]  = { WIFI_SSID,  WIFI_SSID2  };
  static const char* passes[] = { WIFI_PASS,  WIFI_PASS2  };

  if (wifiKey_) {
    wifi_.startConnect(ssids, passes, 2);
  } else {
    wifi_.startAccessPoint("ESP32_Draw", "draw1234");
  }

  clock_.begin();

  // Wire slideshow
  slideshow_.begin(&logger_, &imageStore_, &settings_, &display_);

  web_.begin(&logger_, &fs_, &wifi_, &imageStore_, &display_, &settings_, &slideshow_);
  tick_.begin();
  console_.begin(&logger_, &display_);

  home_.setServices(&imageStore_, &settings_, &clock_, &quotes_, &weather_);

  screens_.begin(display_);
  screens_.set(&boot_, display_);

  // Snapshot initial settings so we can detect changes in update()
  prevSlideshowEnabled_ = settings_.slideshowEnabled();
  prevClockEnabled_     = settings_.clockEnabled();
  strncpy(prevActiveImage_, settings_.activeImage(), sizeof(prevActiveImage_) - 1);
  strncpy(prevStatusMode_,      settings_.statusMode(),      sizeof(prevStatusMode_)      - 1);
  strncpy(prevTopBarColor_,     settings_.topBarColor(),     sizeof(prevTopBarColor_)     - 1);
  strncpy(prevStatusBarColor_,  settings_.statusBarColor(),  sizeof(prevStatusBarColor_)  - 1);
}

void App::update() {
  tick_.update();
  wifi_.update();
  web_.update();
  clock_.update();

  static bool ipShown      = false;
  static bool switchedHome = false;

  if (!ipShown && wifi_.ready()) {
    String ipText = wifiKey_
      ? "IP: " + wifi_.ip().toString()
      : "AP: " + wifi_.ip().toString();

    boot_.setIpText(ipText.c_str());
    logger_.info(("[App] address -> " + ipText).c_str());
    ipShown = true;

    if (wifiKey_ && !ntpDone_) {
      logger_.info("[App] syncing NTP...");
      // UTC-5 EST + 1hr DST = UTC-4 EDT (Michigan/Eastern time)
      if (clock_.syncNtp("pool.ntp.org", -18000, 3600)) {
        logger_.info("[App] NTP sync OK");
      } else {
        logger_.info("[App] NTP sync failed, using elapsed time");
      }
      ntpDone_ = true;
    }
  }

  if (ipShown && !switchedHome && millis() > 3000) {
    String modeStr = wifiKey_ ? "WiFi" : "Hotspot";
    String ipStr   = wifi_.ip().toString();
    home_.setStatusText(("Mode: " + modeStr).c_str(), ("IP: " + ipStr).c_str());
    screens_.set(&home_, display_);
    switchedHome = true;
    logger_.info("[App] switched to HomeScreen");
  }

  // ── Detect settings changes and react immediately ──────────────────────
  if (switchedHome) {
    bool curSlideshow = settings_.slideshowEnabled();
    bool curClock     = settings_.clockEnabled();
    const char* curImg = settings_.activeImage();

    // Clock toggled — force topbar redraw
    if (curClock != prevClockEnabled_) {
      prevClockEnabled_ = curClock;
      home_.invalidateClock();
      logger_.info(curClock ? "[App] clock enabled" : "[App] clock disabled");
    }

    // Active image changed externally (e.g. via /api/images/show) — redraw canvas
    if (strcmp(curImg, prevActiveImage_) != 0) {
      strncpy(prevActiveImage_, curImg, sizeof(prevActiveImage_) - 1);
      home_.invalidateImage();
    }

    // Bar colour changed
    if (strcmp(settings_.topBarColor(), prevTopBarColor_) != 0) {
      strncpy(prevTopBarColor_, settings_.topBarColor(), sizeof(prevTopBarColor_) - 1);
      home_.invalidateClock();   // top bar redraw
    }
    if (strcmp(settings_.statusBarColor(), prevStatusBarColor_) != 0) {
      strncpy(prevStatusBarColor_, settings_.statusBarColor(), sizeof(prevStatusBarColor_) - 1);
      home_.invalidateStatus();  // status bar redraw
    }

    // Reload weather data periodically when in weather mode
    static unsigned long lastWeatherReloadMs = 0;
    if (strcmp(settings_.statusMode(), "weather") == 0 &&
        millis() - lastWeatherReloadMs > 60000) {
      lastWeatherReloadMs = millis();
      if (weather_.reload()) home_.invalidateStatus();
    }

    // Status mode changed
    if (strcmp(settings_.statusMode(), prevStatusMode_) != 0) {
      strncpy(prevStatusMode_, settings_.statusMode(), sizeof(prevStatusMode_) - 1);
      home_.invalidateStatus();
    }

    // Slideshow toggled
    if (curSlideshow != prevSlideshowEnabled_) {
      prevSlideshowEnabled_ = curSlideshow;
      logger_.info(curSlideshow ? "[App] slideshow ON" : "[App] slideshow OFF");
    }

    // Run slideshow tick — advances active image when interval elapses
    slideshow_.update();

    // If slideshow (or /api/slideshow/next) just changed the image, redraw
    if (slideshow_.consumeAdvanced()) {
      strncpy(prevActiveImage_, settings_.activeImage(), sizeof(prevActiveImage_) - 1);
      home_.invalidateImage();
    }
  }

  console_.update();
  screens_.update(display_);
}