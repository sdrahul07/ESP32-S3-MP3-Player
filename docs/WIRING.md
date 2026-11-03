# Wiring: module prototype

GPIO numbers below are ESP32 GPIO identities, not physical header positions. They are taken from both saved `Config.h` files. Disconnect power before rewiring; all modules share ground.

## TFT and its onboard microSD

| TFT breakout label | ESP32-S3 connection |
|---|---|
| VIN | 3V3, as recorded for this prototype |
| GND | GND |
| SCK | GPIO12 |
| MOSI | GPIO11 |
| MISO | GPIO13, used by the card |
| CS | GPIO10 |
| D/C | GPIO9 |
| RST | GPIO14 |
| BL | GPIO21, PWM brightness |
| SDCS | GPIO15 |
| 3Vo | Leave unconnected: regulator output |

Display and card share SPI with separate chip-selects. Saved SPI clocks are 16MHz. Reduce `SD_SPEED` if long/noisy wiring causes card errors; retain short signal wires.

## PCM5102A breakout

| DAC label | Connection |
|---|---|
| VIN | Module-rated supply; use 5V only if its regulator/input specification explicitly permits it |
| GND | ESP32 GND |
| BCK | GPIO4 |
| LCK / LRCK / WS | GPIO5 |
| DIN | GPIO6 |
| SCK | GND for 3-wire PLL operation; this is not BCK |
| L / R / audio GND or jack | Stereo line output to suitable amplifier/AUX input |

No I2C, reset or MCLK output is used. GPIO7/8/16 are unused by the DAC. VIN text and purple color alone do not identify a 5V-capable module.

Required hardware strap states: **FMT low** (Philips I2S), **XSMT/XMT high** (unmute, 3.3V logic), **DEMP low** (off), **FLT low** (normal latency). Verify the actual breakout's solder links before adding wires; H/L jumper numbers differ across boards. Do not drive a control high while it remains strapped to ground.

The PCM5102A is a line-output DAC. Its jack is not proof of headphone-drive capability. Use a load of at least 1kohm / an appropriate headphone amplifier, rather than directly driving 16/32-ohm headphones or an 8-ohm passive speaker.

## ANO wheel, non-I2C

| Breakout signal | ESP32 GPIO | Function |
|---|---|---|
| ENCA | 17 | Encoder A |
| ENCB | 18 | Encoder B |
| SW1 | 1 | Center |
| SW2 | 2 | Down |
| SW3 | 38 | Right |
| SW4 | 39 | Up |
| SW5 | 40 | Left |
| COMA | GND | Common |
| COMB | GND | Common |

Wire by the breakout labels, not guessed connector order. Firmware uses switch input pull-ups.

**Board revision check:** official DevKitC-1 v1.1 uses GPIO38 for its RGB LED. If your exact board has this conflict, move SW3 to a free GPIO such as 41 and change `Pins::RIGHT` in the sketch's `Config.h`. The archived defaults remain unchanged to preserve the original configuration.

## Bring-up order

1. Check power and common ground with power disconnected.
2. Test the DAC using `diagnostics/PCM5102ToneTest`, into a suitable line input at low listening volume.
3. Add TFT and ANO connections using the tables above.
4. Add a FAT32 SD with a few known MP3s; build one complete player checkpoint.
5. Test wheel navigation, playback, brightness and calibration before soldering permanently.
