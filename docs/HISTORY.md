# Source checkpoints

This archive was assembled on 2026-11-02 from saved module-prototype releases. Commit timestamps represent archiving, not the original development dates.

| Tag | Contents / reference |
|---|---|
| `pcm5102a-initial` | Earlier CyberPlayer snapshot, PCM tone diagnostic and exact archived libraries |
| `strand20-v3` | Later 20-theme module firmware with themed submenus and display controls |
| `prototype-baseline` | Same final reference checkout as strand20-v3; return point before further hardware changes |

The original TLV320DAC3100 attempt had failed I2C bring-up. The prototype moved to the hardware-configured PCM5102A; the owner subsequently reported successful audio and song playback. This repository focuses on that module configuration and preserves the initial and later source checkpoints without changing their firmware.

The later saved source incorporates theme-specific menu rendering after feedback about generic submenus. Historical desktop validation is retained in `docs/archive/`. It does not establish that every later firmware snapshot was tested successfully on the physical player.

Checksums in `snapshots/SHA256.json` record each copied file and its relative source-release provenance. Run `python scripts/verify-snapshot.py` after cloning. No local user paths, build logs, music collection or custom-board files are part of the archive.
