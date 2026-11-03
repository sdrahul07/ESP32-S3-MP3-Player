# Validation and known limits

## Archive checks

The export copies firmware, assets, vendor files and selected library build sources byte for byte. `snapshots/SHA256.json` records the copies; `scripts/verify-snapshot.py` detects missing or modified archived files. Both Config.h snapshots use the GPIO assignments in `docs/WIRING.md`.

Only prototype sources, build dependencies, diagnostic code, documentation and native firmware preview images are included. Firmware was not edited or flashed while preparing this repository.

## Historical validation

The full saved report is in `docs/archive/STRAND20-TEST-REPORT.md`. It reports desktop C++ tests of the actual theme renderer, long-text movement, decoded-PCM analysis, stereo isolation, silence/pause/reset behavior, calibration identity and save/cancel/reset handlers, and themed menu rendering.

The archived agent environment could not launch its installed Xtensa compiler due to Windows Illegal System DLL Relocation. Therefore this is a source snapshot, with no verified ESP32 binary supplied. This archival task does not re-run or independently certify that historical desktop harness.

## Device verification still required

- Compile in the owner's working Arduino toolchain.
- Test playback, track changes, queue and SD activity on the assembled prototype.
- Measure actual animation timing and confirm long-label scrolling on the LCD.
- Verify supply ratings, board revision pin conflicts and audio output loading.
- Compare calibration patterns on the physical display under normal lighting.

The owner's reported working playback is useful project history, not a claim that this exact final source checkout passed a new hardware test. No gapless, high-resolution FLAC, runtime or headphone-power specification is established here.
