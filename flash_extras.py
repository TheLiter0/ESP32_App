Import("env")

# Force the partition table binary to be flashed at 0x8000 every upload
# This fixes the "partition not found" error after changing partition layout
env.Replace(
    UPLOADERFLAGS=[
        "--chip", "esp32",
        "--baud", "$UPLOAD_SPEED",
        "--before", "default_reset",
        "--after", "hard_reset",
        "write_flash",
        "-z",
        "--flash_mode", "dio",
        "--flash_freq", "40m",
        "--flash_size", "detect",
        "0x1000",  "$BUILD_DIR/bootloader.bin",
        "0x8000",  "$BUILD_DIR/partitions.bin",
        "0xe000",  env.subst("$PACKAGE_DIR/framework-arduinoespressif32/tools/partitions/boot_app0.bin"),
        "0x10000", "$BUILD_DIR/firmware.bin",
    ]
)
