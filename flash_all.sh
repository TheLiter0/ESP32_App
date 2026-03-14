#!/bin/bash

# Full flash script - burns bootloader, partition table, firmware, and filesystem
# all in one esptool command. This guarantees everything is consistent.

PORT="/dev/ttyUSB0"
ESPTOOL=$(find ~/.platformio/packages/tool-esptoolpy -name "esptool.py" 2>/dev/null | head -1)
BUILD=".pio/build/esp32dev"
BOOT_APP="$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"

if [ -z "$ESPTOOL" ]; then
  echo "ERROR: esptool.py not found in ~/.platformio"
  echo "Run 'pio run' once first to download it, then try again."
  exit 1
fi

if [ ! -f "$BUILD/firmware.bin" ]; then
  echo "ERROR: $BUILD/firmware.bin not found."
  echo "Run 'pio run' first to build the firmware, then run this script."
  exit 1
fi

if [ ! -f "$BUILD/littlefs.bin" ]; then
  echo "ERROR: $BUILD/littlefs.bin not found."
  echo "Run 'pio run --target buildfs' first, then run this script."
  exit 1
fi

echo "============================================"
echo "  ESP32 Full Flash"
echo "  Port:   $PORT"
echo "  Tool:   $ESPTOOL"
echo "============================================"
echo ""
echo "Step 1/2: Erasing chip..."
python3 "$ESPTOOL" --chip esp32 --port "$PORT" --baud 921600 erase_flash
if [ $? -ne 0 ]; then echo "ERASE FAILED"; exit 1; fi

echo ""
echo "Step 2/2: Flashing everything..."
python3 "$ESPTOOL" \
  --chip esp32 \
  --port "$PORT" \
  --baud 921600 \
  --before default_reset \
  --after hard_reset \
  write_flash \
  -z \
  --flash_mode dio \
  --flash_freq 40m \
  --flash_size detect \
  0x1000   "$BUILD/bootloader.bin" \
  0x8000   "$BUILD/partitions.bin" \
  0xe000   "$BOOT_APP" \
  0x10000  "$BUILD/firmware.bin" \
  0x190000 "$BUILD/littlefs.bin"

if [ $? -ne 0 ]; then echo "FLASH FAILED"; exit 1; fi

echo ""
echo "============================================"
echo "  Done! Reset the ESP32 and open:"
echo "  http://10.0.0.82"
echo "============================================"
