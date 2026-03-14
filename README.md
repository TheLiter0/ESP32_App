# ESP32 Display Control System

An ESP32-powered web-controlled display system built around a 240×240 ST7789 screen.

The ESP32 serves its own web app, lets you draw from a browser, uploads the drawing as raw RGB565 data, shows it on the screen, and is being expanded into a small standalone display platform with saved images, gallery controls, slideshow mode, quotes, clock, and weather.

---

## What this project does

This project turns an ESP32 + TFT screen into a self-hosted display device.

Current direction:

- host a browser-based drawing app directly from the ESP32
- upload drawings to the device
- render them on the TFT display
- save images to LittleFS
- manage images through a gallery
- support slideshow mode
- show a home screen with a top bar, main image area, and bottom status bar
- support quotes, clock, and weather in the UI

---

## Main idea

The system is split into two parts:

### 1. Browser UI
A phone, tablet, or computer connects to the ESP32 and opens the built-in web app.

From there you can:

- draw on a canvas
- change colors and brush size
- use shape tools
- upload the current canvas to the ESP32
- browse saved images
- change device settings

### 2. ESP32 device
The ESP32:

- hosts the website
- receives uploads
- drives the TFT display
- stores settings and image data in LittleFS
- manages Wi-Fi / hotspot mode
- updates the on-screen UI

---

## Display layout

The screen is organized into three regions:

- **Top bar** — date / time / title area
- **Canvas area** — main image display area
- **Bottom status bar** — Wi-Fi info, quotes, or weather

Current layout from `Layout.h`:

- Top bar: `240 × 30`
- Canvas area: `240 × 180`
- Status bar: `240 × 30`

---

## Features

### Working now
- ESP32 web server hosted directly on the device
- Browser drawing interface
- Raw RGB565 upload pipeline
- TFT image rendering
- ST7789 display driver wrapper
- LittleFS support
- Wi-Fi station mode
- Hotspot / AP mode
- Boot screen with connection info
- Home screen architecture
- Settings storage
- Quote loading from JSON
- Weather JSON storage and display hooks
- Gallery and settings pages in the web UI

### In active development
- persistent saved image flow
- image index repair / rebuild tools
- slideshow behavior
- polished home screen behavior
- storage stability and cleanup
- modular frontend cleanup

---

## Hardware

### Required
- ESP32 development board
- ST7789 SPI display (240×240)
- USB cable
- jumper wires

### Current display pins
The current code defaults to:

- `TFT_CS   = GPIO5`
- `TFT_DC   = GPIO2`
- `TFT_RST  = GPIO4`

SPI uses the ESP32 hardware SPI pins.

### Mode switch
The project uses:

- `GPIO27` as a startup mode switch
- `GPIO22` and `GPIO21` as simple indicators

Behavior:

- **GPIO27 grounded** → station / Wi-Fi mode
- **GPIO27 open** → hotspot mode

---

## Network behavior

The device supports two startup modes.

### Station mode
If `GPIO27` is grounded on boot, the ESP32 tries to connect to saved Wi-Fi networks from `src/secrets.h`.

### Hotspot mode
If `GPIO27` is not grounded, the ESP32 starts its own access point.

Current defaults:

- **SSID:** `ESP32_Draw`
- **Password:** `draw1234`

---

## Tech stack

- **PlatformIO**
- **ESP32 Arduino framework**
- **LittleFS**
- **Adafruit GFX**
- **Adafruit ST7735/ST7789 library**
- **WebServer**
- **WiFi / WiFiServer**

---

## Project structure

```text
ESP32_App/
├── data/                        # Files uploaded to LittleFS and served by the ESP32
│   ├── index.html               # Main web UI
│   ├── style.css                # Frontend styling
│   ├── app.js                   # Frontend logic
│   ├── config/
│   │   ├── settings.json
│   │   └── quotes.json
│   └── images/
│       └── index.json
│
├── src/
│   ├── app/
│   │   ├── App.h
│   │   └── App.cpp              # Main application wiring
│   │
│   ├── drivers/
│   │   ├── Display.h
│   │   └── Display.cpp          # Screen wrapper around Adafruit ST7789
│   │
│   ├── services/
│   │   ├── FsService.*          # LittleFS mount/setup
│   │   ├── WiFiService.*        # Wi-Fi / AP management
│   │   ├── WebService.*         # Web UI + API + upload server
│   │   ├── ImageStore.*         # Saved image storage/index logic
│   │   ├── SettingsService.*    # Device settings load/save
│   │   ├── ClockService.*       # NTP / time formatting
│   │   ├── QuoteService.*       # Quote loading/rotation
│   │   ├── SlideshowService.*   # Slideshow control
│   │   ├── WeatherService.*     # Weather JSON storage/formatting
│   │   ├── Logger.h
│   │   ├── SerialConsole.*
│   │   └── Tick.h
│   │
│   ├── ui/
│   │   ├── Layout.h
│   │   ├── Screen.h
│   │   ├── ScreenManager.*
│   │   ├── BootScreen.*
│   │   └── HomeScreen.*
│   │
│   ├── main.cpp
│   └── secrets.h                # Local Wi-Fi credentials (not for public commits)
│
├── platformio.ini
├── partitions.csv
└── README.md