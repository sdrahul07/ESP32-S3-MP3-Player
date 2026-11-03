# CyberPlayer MP3 / PCM5102A

This source targets the PCM5102A DAC. Read ../PCM5102-WIRING.md before connecting
power or audio outputs. The PCM5102ToneTest sketch checks audio without SD or UI.

Open CyberPlayer.ino in this folder; App.cpp, Media.cpp and Decoder.cpp compile
alongside it. Board: ESP32S3 Dev Module, Flash Size 16MB, OPI PSRAM, COM5 CH343
port. USB CDC should be disabled for that UART bridge. Arduino-ESP32 3.x required;
source authored against the installed 3.3.8 core.

Arduino libraries: Adafruit ST7735 and ST7789 Library, Adafruit GFX Library,
JPEGDEC, and their dependencies. TLV320 and ESP32-audioI2S are NOT required.
Alternatively run ../Upload-CyberPlayer.ps1 to use bundled libraries and build
before flashing. Close Serial Monitor before using the script.

## Connections

DAC: BCK=4, LRCK/LCK=5, DIN=6, SCK=GND, common GND. Verify VIN's supply rating
for your exact purple breakout. No I2C or reset pins are used.
TFT: VIN=3V3, GND=GND, SCK=12, MOSI=11, MISO=13, CS=10, DC=9, RST=14, BL=21,
onboard SD CS=15. Leave TFT 3Vo unconnected.
ANO: ENCA=17, ENCB=18, SW1 center=1, SW2 down=2, SW3 right=38, SW4 up=39,
SW5 left=40, COMA=GND, COMB=GND. Read the ANO breakout labels, not pin positions.
GPIO38 is used by the RGB LED on official DevKitC-1 v1.1 boards; if that is your
exact board, move ANO SW3 to a free GPIO such as 41 AND change RIGHT in Config.h.

## Controls

Rotate changes volume on Now Playing and selection in menus. Center selects or
plays/pauses. Up opens tracks; Down opens queue. Left/right skip tracks, and hold
left/right seeks 10 seconds. Hold center opens the menu, or track actions on a
track-list row. Menu has artists, albums, folders, playlists, queue, shuffle and
repeat. The output entry is informational; no speaker control or jack detection.

## Features and limits

MP3 mono/stereo at 32/44.1/48 kHz; native-rate I2S; smoothed software volume;
PCM peak meters; background audio task; buffered TFT UI; artist/album/folder
browsing; queue, play-next and M3U playlists. Embedded baseline JPEG artwork is
read from ID3v2.3/v2.4 APIC frames (up to 1 MiB). PNG, progressive JPEG, ID3v2.2,
unsynchronised/compressed tags and full Unicode fonts are not implemented.

Use a FAT32 SD card. Startup scans up to 4096 tracks and 12 directory levels.
Artist/album grouping is basic; library caching and power-off resume are absent.
Some MP3s without duration headers display unknown duration, and seeking may
pause to build an index. The next decoder is prepared when duration is known;
LAME/Xing delay and padding handling is provided by minimp3. Gapless transitions
remain unverified, especially across sample-rate changes.

Loading playlists is read-only. Menu > Save queue creates a new CYBERQUEUE-N.m3u
on the SD card without overwriting an existing name. Existing playlists support
exact absolute or playlist-relative paths, without .. normalization or network
URLs. SD runs at 16 MHz; reduce SD_SPEED to 8 MHz in Config.h if wiring is noisy.

No speaker amplifier or headphone driver is included in a bare PCM5102A module.
Connect line out to powered speakers or an appropriate external amplifier.

Validation: source reviewed, but not compiled or hardware-tested in this agent
environment because both installed ESP32 compilers fail at process launch with
Illegal System DLL Relocation. No PCM5102A firmware has been uploaded here.

Third-party sources and licenses: minimp3 (CC0 notices in vendor/), Adafruit
ST7735/ST7789, Adafruit GFX, Adafruit BusIO, and JPEGDEC. Bundled library licenses
are retained. minimp3 source: https://github.com/lieff/minimp3
PCM5102A reference: https://www.ti.com/lit/ds/symlink/pcm5102a.pdf
