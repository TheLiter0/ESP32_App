# ESP32 Web Drawing Display

An embedded system that lets users sketch on a browser canvas and render the drawing in real time on an ESP32-connected TFT display. The ESP32 hosts its own local web server, so no internet connection or external backend is required.

---

## How It Works

```
Browser Canvas  →  HTTP Upload  →  ESP32 Web Server  →  TFT Display
```

1. The ESP32 connects to your Wi-Fi network and starts a local HTTP server.
2. Any device on the same network can open the drawing interface in a browser.
3. The user draws on the canvas and hits **Upload**.
4. The ESP32 receives the image data and renders it pixel-by-pixel on the TFT display.

---

## Features

- **Zero-dependency frontend** — drawing interface served directly from the ESP32, no app needed
- **Works on any device** — phone, tablet, or desktop browser
- **Real-time rendering** — image appears on the TFT display within seconds of uploading
- **Lightweight HTTP server** — handles drawing uploads, previews, and status checks
- **Supports ST7735 & ST7789** — common SPI TFT displays

---

## Hardware Requirements

| Component | Notes |
|-----------|-------|
| ESP32 development board | Any standard ESP32 devkit |
| SPI TFT display | ST7735 or ST7789 |
| Jumper wires | |
| USB cable | For flashing and power |

### Wiring

| ESP32 Pin | Display Pin |
|-----------|-------------|
| GPIO 23 | MOSI |
| GPIO 18 | SCLK |
| GPIO 5 | CS |
| GPIO 2 | DC |
| GPIO 4 | RST |
| 3.3V | VCC |
| GND | GND |

---

## Software Requirements

**Tools:**
- [PlatformIO](https://platformio.org/) + VS Code
- ESP32 Arduino Framework

**Libraries** (managed via PlatformIO):
- `WiFi`
- `WebServer`
- `SPI`
- `Adafruit_GFX`
- `Adafruit_ST7735` / `Adafruit_ST7789`

---

## Getting Started

### 1. Clone and configure

```bash
git clone https://github.com/TheLiter0/ESP32_App.git
cd ESP32_App
```

Add `src/secrets.h` (or edit your config file) to set your Wi-Fi credentials:

```cpp
const char* SSID     = "your-network";
const char* PASSWORD = "your-password";
```

### 2. Build and flash

```bash
pio run --target upload       # Flash firmware
pio run --target uploadfs     # Upload web interface to SPIFFS
```

### 3. Use it

1. Power the ESP32 — it will connect to Wi-Fi and print its IP address over serial.
2. On a device connected to the same network, open a browser and go to the ESP32's IP.
3. Draw on the canvas and click **Upload**.
4. The image renders on the TFT display.

---

## API Routes

| Route | Method | Description |
|-------|--------|-------------|
| `/` | GET | Serves the web drawing interface |
| `/upload` | POST | Receives image data from the canvas |
| `/preview` | GET | Returns the currently stored image |
| `/status` | GET | Returns device status info |

---

## Project Structure

```
ESP32_App/
├── src/
│   ├── drivers/
│   │   ├── Display.h / .cpp       # TFT display driver
│   ├── services/
│   │   ├── WebService.h / .cpp    # HTTP server logic
│   │   └── WiFiService.h / .cpp   # Wi-Fi connection management
│   ├── ui/
│   │   ├── BootScreen.h / .cpp    # Startup screen
│   │   └── Layout.h / .cpp        # Display layout helpers
│   └── main.cpp
├── data/                           # Web interface files (served via SPIFFS)
├── platformio.ini
└── README.md
```

---

## Roadmap

- [ ] Live drawing streaming (no upload step)
- [ ] Multiple UI screens / navigation
- [ ] On-device image storage and gallery mode
- [ ] Touchscreen input support
- [ ] OTA firmware updates
- [ ] Animation playback

---

## License

MIT — see [LICENSE](LICENSE) for details.
