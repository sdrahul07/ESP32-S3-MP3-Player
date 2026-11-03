# Build and restore

## Arduino IDE 2

Install Espressif's ESP32 board package **3.3.8**, the archived development version. Open one complete sketch:

- `firmware/CyberPlayer/CyberPlayer.ino` for the earlier prototype.
- `firmware/StrandPlayer20/StrandPlayer20.ino` for the later theme/menu snapshot.
- `diagnostics/PCM5102ToneTest/PCM5102ToneTest.ino` for isolated audio testing.

Keep all adjacent C++, header, `.inc`, `assets` and `vendor` files. An almost-empty `.ino` is intentional. Do not combine the two sketches.

| Tool setting | Saved player configuration |
|---|---|
| Board | ESP32S3 Dev Module |
| Flash | 16MB |
| PSRAM | OPI PSRAM |
| Partition | 16M Flash, 3MB APP / 9.9MB FATFS |
| Upload speed | 115200 |
| USB CDC on boot | Disabled for the original UART bridge setup |
| Port | The board's current port, not an archived COM number |

The 20-theme artwork needs the larger application partition. Internal FATFS does not contain the music; MP3 files remain on SD. A board without the expected PSRAM requires a separate port, not just selecting a different board label.

Copy the archived `libraries/` subdirectories into your Arduino sketchbook libraries directory, avoiding conflicting duplicate installs. Versions: BusIO 1.17.4, GFX 1.13.0, ST7735/ST7789 1.12.0, JPEGDEC 1.8.4, plus its archived bb_spi_lcd dependency. Latest uses the three Adafruit libraries; Earlier additionally needs JPEGDEC. The audio test uses the ESP32 core only.

## Command line

The helper accepts `-ArduinoCli` if the CLI is not in PATH. Install core 3.3.8 through Boards Manager or your normal Arduino CLI setup first. The helper compiles by default and uploads only with `-Upload -Port <current-port>`.

```powershell
.\scripts\Build-Prototype.ps1 -Checkpoint Early
.\scripts\Build-Prototype.ps1 -Checkpoint Latest
.\scripts\Build-Prototype.ps1 -Checkpoint AudioTest
.\scripts\Build-Prototype.ps1 -Checkpoint Latest -Upload -Port COM6
```

Player FQBN:

```text
esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,CDCOnBoot=default,UploadSpeed=115200
```

The helper uses bundled libraries and writes outputs to ignored `build/` directories. It does not install or upgrade dependencies, erase flash, or reformat the SD card.

## Rollback

Use the tagged `prototype-baseline` checkout as the frozen reference, or `pcm5102a-initial` for the earlier-source-only checkpoint. Both complete source versions remain on main. Open the desired sketch and upload it with the settings above. Preserve your music and any desired settings separately before experimenting; Git archives source, not a complete device flash/NVS backup.

Archive READMEs retain original relative paths and old COM examples for provenance. These root docs and the portable build helper describe this repository's layout.
