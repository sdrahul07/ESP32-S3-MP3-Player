# PCM5102A replacement: wiring and bring-up

The current CyberPlayer source now targets PCM5102A. TLV320 I2C scanning,
configuration, reset and speaker/headphone-detection controls are removed. It
uses native MP3 sample rates (32/44.1/48 kHz) with stereo 32-bit Philips I2S.

Disconnect USB before changing wires. Remove the TLV320 board from the circuit.
GPIO7, GPIO8 and GPIO16 are no longer used by the player.

| Purple PCM5102A module | ESP32-S3 |
|---|---|
| VIN | 5V ONLY if the module has a regulator and specifies a 5V-capable VIN |
| GND | GND |
| BCK | GPIO4 |
| LCK / LRCK / WS | GPIO5 |
| DIN | GPIO6 |
| SCK (master/system clock) | GND for three-wire operation |

VIN labeling and purple PCB color alone do not establish the voltage rating.
Check the module listing or send a photo showing its regulator before applying
5V. Never connect 5V to a pad labeled 3V3 or a PCM5102A logic input.

SCK is NOT the audio bit clock: BCK receives GPIO4; SCK is tied LOW to select
the BCK-based PLL. The firmware does not output MCLK.

If exposed, hardware controls must select FMT=LOW (Philips I2S), XSMT/XMT=HIGH
(unmuted; 3.3V logic), and DEMP=LOW (de-emphasis off). FLT=LOW selects normal
latency. These are often already set by solder straps. Do not guess H/L jumper
numbers or connect a strapped-to-ground control to 3V3 without inspecting the
board. A photo of both sides is needed to identify the exact strap layout.

The output jack / L-R-GND pads provide line-level stereo, with a minimum load of
1 kohm. Use an amplifier AUX input or powered speakers. Ordinary 16/32-ohm
headphones need a separate headphone amplifier. An 8-ohm passive speaker cannot
connect directly. The jack alone does not establish headphone-drive capability.

## First test

1. Open PCM5102ToneTest/PCM5102ToneTest.ino in Arduino IDE.
2. Select ESP32S3 Dev Module and the same working USB port; use Arduino-ESP32 3.x.
3. Upload. The sketch needs no third-party libraries, SD card or PSRAM.
4. Connect line out to powered speakers/AUX with their volume initially low.
5. You should hear a low-level 440 Hz tone on the left and 660 Hz on the right.
6. Serial at 115200 reports I2S errors. There is no I2C detection or DAC identity
   check because this is a hardware-configured DAC.

Then open CyberPlayer/CyberPlayer.ino and upload the MP3 player. Display, SD and
ANO wiring remain as previously documented. At boot Serial prints
`PCM5102A build v1: no I2C detection required.` If it instead prints
`DAC diagnostic v2`, the old TLV320 firmware is still loaded.

Validation: source updated and reviewed; not compiled or hardware-tested here
because the installed ESP32 compiler fails to launch in the agent environment.

Reference: https://www.ti.com/lit/ds/symlink/pcm5102a.pdf
