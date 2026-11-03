# ES32-S3 MP3 Player — initial PCM5102A checkpoint

ESP32-S3 N16R8 module prototype with Adafruit ST7789 display/onboard microSD,
PCM5102A DAC and non-I2C ANO wheel. This tag freezes the earlier MP3 source.

- Open `firmware/CyberPlayer/CyberPlayer.ino` with every adjacent source file.
- [Complete module wiring and GPIOs](docs/WIRING.md)
- [Parts and power boundaries](docs/HARDWARE.md)
- [DAC setup / tone test](firmware/PCM5102-WIRING.md)
- [Original firmware build notes](firmware/CyberPlayer/README.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)

The tiny .ino delegates to App.cpp. Use Arduino-ESP32 3.3.8, ESP32S3 Dev Module,
16MB flash, OPI PSRAM and the board's current serial port. Bundled libraries
retain their upstream notices. `python scripts/verify-snapshot.py` verifies
archived file bytes. The owner reported working playback on this module setup;
this historical source has not been newly compiled or hardware-tested here.

No custom PCB or fabrication data is included. The later archive on main adds a
separate 20-theme checkpoint; this earlier source remains available unchanged.
