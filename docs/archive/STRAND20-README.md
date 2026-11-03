# STRAND — 20 selected themes

ESP32-S3 N16R8 / Adafruit ST7789 320x240 / PCM5102A / ANO wheel / MP3 on SD.

## Open and build

Extract the entire ZIP. Open `StrandPlayer20/StrandPlayer20.ino` in Arduino IDE 2.
The small .ino is intentional: setup(), loop(), audio and menus are in App.cpp.
Keep every source file, assets/ and vendor/ beside it.

Use the existing ESP32 Arduino core 3.3.8 and these Tools settings:

- Board: ESP32S3 Dev Module.
- Flash Size: 16MB (128Mb).
- PSRAM: OPI PSRAM.
- Partition Scheme: **16M Flash (3MB APP/9.9MB FATFS)**.
- Upload Speed: 115200, as in the working build.
- USB CDC On Boot: Disabled, as in the working build; keep your working USB connector/serial setup.

The default 1.2MB app partition is too small for the added artwork. The 3MB app
partition is important. No files need to be placed on internal FATFS; music stays
on the existing SD card. No SD reformat or wiring changes are required.

Libraries are the same Adafruit GFX, Adafruit BusIO and Adafruit ST7735/ST7789
used by the working player. Bundled copies are in StrandPlayerLibraries. The
PowerShell helper scopes these libraries to this build. For Arduino IDE, retain
your already installed working versions. No LVGL, TFT_eSPI or DisplayKit is used.

Optional build helper, run from PowerShell in the extracted directory:

    .\Upload-StrandPlayer20.ps1 -CompileOnly

To build and upload after closing Serial Monitor, specify the actual port:

    .\Upload-StrandPlayer20.ps1 -Port COM6

The helper's default remains COM5. Use your device's current port, not a stale
port from a previous session. No upload was performed while generating this release.

## Controls and retained features

- Center: play/pause during playback; select in menus.
- Hold Center: Library. Song-list context actions remain available.
- Wheel: volume during playback; scroll in menus.
- Up: songs. Down: queue. Left/Right: previous/next.
- Hold Left/Right during playback: seek by 10 seconds.
- Library > Settings > Appearance > Themes: select and apply one of 20 themes.
- Settings > Display > Brightness: 5–100%, live preview; Center saves, Left cancels.
- Settings > Display > Color calibration: RGB gain, gamma, saturation and contrast;
  live color bars, grayscale, near-black patches and RGB ramps. Save, Cancel and
  Reset Preview retain the v4 behavior. Holding Center cancels a calibration session.
- Long titles, artists and album labels scroll within their own clipped viewports.
  Pixel themes use their own crisp font, with larger artist text than the first mockup.
- MP3 playback, queue, playlists, shuffle and repeat remain available.

Brightness and color settings use the existing NVS keys and are retained. The
20-theme selection has its own versioned key, theme20. A saved original Signal,
Tape, Arc or Chrono theme is mapped to the corresponding new entry on first run.
Neutral calibration preserves every original RGB565 value exactly.

## Selected theme order

1. Neon Fishbone — cyan / hot pink / yellow.
2. Sakura Tape — spring green / blossom pink / cream.
3. Sunvair Flight.
4. NASA Worm.
5. Card Battle — light.
6. Lunar Courier.
7. Tidal Station.
8. Amber Workshop.
9. Iron Circuit — dark.
10. Melody Machine.
11. Pocket OS.
12. Dungeon Deck.
13. Amber Navigator.
14. Signal Report / Black.
15. Tape Session.
16. Signal Report / Light.
17. Arc Transport.
18. Chrono Deck.
19. Phosphor Deck.
20. Retro Desktop.

The nine illustrated/logo panels are embedded in flash at native 314x138 RGB565,
approximately 780KB total. Playback never opens artwork files from SD. The original
five-theme firmware and its ZIPs are preserved separately for rollback.

## Visualizers: what is measured

Signal Report (both variants), Tape Session, Phosphor Deck and Retro Desktop use
the decoded MP3 samples that feed I2S. Analysis is read-only and runs before
listening-volume scaling. Volume changes do not flatten the music monitor.

Every decoded stereo block feeds a streaming analyzer, including blocks between
UI updates. It records signed min/max and true RMS in chronological 10ms bins.
The 92-bin trace covers about 0.92 seconds. Publishing occurs approximately every
20ms. There are no synthesized sine waves, random bars or fabricated spectrum data.

Waveform movement interpolates the known chronological bins with a small display
lag. Stereo RMS meters use time-based 35ms attack / 180ms release. The Phosphor
field and Retro Desktop bars show amplitude history, **not frequency spectrum**.
Digital silence produces zero levels and a flat trace. Pause freezes history and
lets live meters settle; track changes and seeking clear the analyzer.

The renderer targets approximately 33 frames/second, compares framebuffers, and
transmits only changed horizontal spans in eight-row bands. Each SPI transaction
is bounded to at most 2,560 pixels, with the shared bus released between bands.
Audio retains its own core/task and higher priority. The working 16MHz SPI clocks
and DMA configuration are unchanged. Actual FPS, latency and audio continuity
must still be checked on your assembled device, especially with SD activity.

## Validation and limits

The desktop C++ harness compiled the actual theme renderer and analysis code. Tests
passed all 20 renderers, long-title motion, 32/44.1/48kHz RMS accuracy, 100 bins per
second, block-boundary invariance, read-only input, impulse chronology, stereo
isolation, silence, reset, paused visualizer stability, and calibration.
The actual calibration handlers also passed Save/Cancel/Reset and color tests.
Decoder, media/indexing, pinout, I2S configuration and volume/output loop are unchanged.

RenderedPreviews contains native-resolution output from the actual C++ renderers,
not the earlier browser artwork concepts. Artwork and fonts therefore show their
real 320x240 limits before upload.

Full ESP32 compilation was blocked in the agent execution environment by Windows
"Illegal System DLL Relocation" when launching the installed Xtensa compiler.
This ZIP is the complete Arduino source release, not a verified .bin. Compile in
your normal Arduino IDE before upload. On-device audio/FPS testing is still pending.

Audio format remains MP3 at 32/44.1/48kHz, mono or stereo. PCM5102A remains line out;
use the same listening setup that already worked. No FLAC or hardware changes added.

## v2 build compatibility fix

Both reported compiler errors are fixed by explicitly typing the std::min/std::max
operations as uint32_t. This avoids unsigned-int versus unsigned-long deduction
conflicts in the ESP32 toolchain. A second desktop compilation models the ESP32
unsigned-long uint32_t typedef and passes the actual renderer and PCM tests.

The first reported Arduino build still selected PartitionScheme=default. Select
Tools > Partition Scheme > 16M Flash (3MB APP/9.9MB FATFS) before compiling again.
All artwork, theme choices, audio behavior and display controls are unchanged.

## v3: themed navigation and settings

The first 20-theme release still used the old generic submenu renderer. v3 replaces
that renderer for Library, songs/groups/queue/playlists, song actions, Settings,
Appearance, Themes, Display, the color-calibration menu, brightness and toast messages.

Artwork themes retain their art behind the selection panel. Melody Machine has
lavender instrument rows; Pocket OS uses a desktop window; Dungeon uses inventory
frames; Amber Navigator uses four selectable instrument tiles. Signal/Tape use their
technical typography and panels, Arc has its ruler and rounded selection, Chrono
uses its orange transport band, and Retro Desktop keeps its monochrome window chrome.
Long selected menu rows scroll. Their page sizes follow the layouts (3, 4 or 5 rows).
Navigator tiles paginate in groups of four. Song libraries are referenced directly,
without copying thousands of titles on every animation frame.

The color test patterns themselves keep their neutral measurement background.
Brightness values, saving/cancelling, calibration controls, audio analysis, decoder,
I2S output and physical controls are unchanged. ESP32 typedef fixes from v2 remain.
The 3MB app partition is still required.

The native C++ test suite now renders each theme's Library, Settings and brightness
pages, verifies visible selection changes, long-menu-row scrolling, empty lists and
changing brightness values. It passes with the ESP32 unsigned-long uint32_t model.
RenderedMenus contains outputs of the actual firmware renderer, not browser mockups.
Full ESP32 compilation and device testing are still pending in this environment.
