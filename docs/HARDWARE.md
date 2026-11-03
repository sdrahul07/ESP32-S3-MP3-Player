# Prototype hardware

| Part | Reference configuration |
|---|---|
| Controller | ESP32-S3 development board, N16R8: 16MB flash / 8MB PSRAM |
| Display / storage | Adafruit 2.0-inch 240x320 ST7789 IPS breakout, product 4311; landscape 320x240; built-in microSD socket |
| Audio DAC | Common purple PCM5102A breakout, hardware configured, stereo line output |
| Controls | Adafruit ANO wheel with non-I2C breakout: quadrature encoder and five switches |
| Storage | FAT32 microSD containing MP3 files |
| Interconnect | Breadboard/jumpers or hand-wired protoboard; shared ground |
| Reference power | USB-powered development board; DAC supplied within its specific module rating |
| Listening | Powered speaker/AUX input or a suitable external headphone amplifier |

The standalone microSD breakout, OLED, ESP32-WROOM-32E, TLV320DAC3100, separate encoder and passive speaker were explored or available but are not wired in this PCM5102A reference configuration. Use the TFT's SD socket, not two cards on the same chip-select.

## Battery boundary

The project inventory includes a protected single-cell 3.7V 800mAh LiPo and a TP4056 module exposing B+/B- and OUT+/OUT-. The owner reported that its charge current was set appropriately for the cell. The complete battery-to-system regulation and switching path was not captured as a verified prototype connection, so it is not specified as a reproducible wiring step here.

TP4056 charging/protection terminals do not establish a regulated 5V output or a load-sharing circuit. Do not infer a direct battery connection to the dev board's 3V3 or 5V pin from this archive. USB power is the reference for bring-up. A battery integration needs its own verified power path.

## Module references

- [Adafruit display #4311](https://www.adafruit.com/product/4311)
- [PCM5102A datasheet](https://www.ti.com/lit/ds/symlink/pcm5102a.pdf)
- [ESP32-S3 DevKitC-1 v1.1 guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html)

The prototype's exact clone revision and PCM breakout supply rating are not independently established. Board labels and regulator specifications take precedence over appearance.
