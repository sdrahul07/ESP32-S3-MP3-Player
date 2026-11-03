# Validation report — STRAND 20 themes

PASS: Native C++ compilation of actual Themes.cpp, SelectedThemes.inc, PcmScope.h and DisplayCalibration.cpp.
PASS: All 20 renderers produce native 320x240 output; every long-title viewport moves.
PASS: Continuous PCM analysis at 32,000 / 44,100 / 48,000 Hz, 100 bins/sec, measured sine-wave RMS.
PASS: Identical analysis across different decoder block partitions; no audio sample modifications.
PASS: Chronological impulse history, independent stereo channels, silence and track/seek reset.
PASS: Signal Report (black/light), Tape Session, Phosphor Deck and Retro Desktop respond to PCM and remain stable in silence/pause.
PASS: Actual brightness handlers: pin 21, 20kHz PWM, 8-bit resolution, clamping, duty conversion and fallback.
PASS: Actual calibration menu handlers: Save / Cancel / Reset; all-65536-pixel identity, gamma endpoints, isolated RGB gains, grayscale and invalid-setting fallback.
PASS: Decoder, media, pinout, I2S setup, gain/output loop, GPIO polling, brightness and calibration handlers unchanged from v4.
PASS: 16MB/OPI/3MB app partition selected and recognized by Arduino CLI; original NVS offsets retained.

Desktop native renderer + dirty-span benchmark (NOT ESP32 performance): changing audio caused approximately
9,324 changed pixels/frame for black Signal Report; 5,830 for Tape; 9,676 for light Signal;
8,180 for Phosphor; 2,278 for Retro Desktop. Static artwork did not retransmit on PCM updates.

ESP32 build: BLOCKED before source compilation: installed Xtensa executable fails to launch with
Windows Illegal System DLL Relocation in the agent execution environment. No application .bin produced.
On-device FPS, audio continuity and final LCD appearance: NOT TESTED. Build in normal Arduino IDE.

V2: fixed both ESP32 unsigned-int / unsigned-long template deduction errors.
PASS: actual 20-theme renderer and PCM suite compiled and ran with uint32_t modeled as unsigned long.

V3 PASS: all 20 themed navigation, Settings, brightness pages, selected-row visibility, long-menu-row scrolling and empty states; PCM, input sampling, brightness and calibration handlers unchanged from v2.
