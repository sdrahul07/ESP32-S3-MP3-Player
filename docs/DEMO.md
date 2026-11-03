# Interactive browser demo

**Live:** https://sdrahul07.github.io/ES32-S3-MP3-Player/

The gallery packages the original interactive HTML concepts for all 20 themes selected for the module prototype. Theme selection is shareable through the URL fragment. It offers search, previous/next theme navigation, 1x/2x canvas display and each concept's original controls. Illustrated themes include sample-track selection, play/pause, seeking, library and volume controls. Other concepts expose the controls present in that saved design.

The **Firmware renders** tab shows static images from the actual saved C++ renderer: Now Playing, Library, Settings and Brightness for each theme. These images provide a comparison with the later firmware implementation. Browser concepts and firmware can differ in typography, navigation and animation.

## Limits

- This is an HTML design preview, not an ESP32 emulator or MP3 decoder.
- Track names, clocks and signal animation are simulated. No music files are included or played.
- Keyboard shortcuts invoke existing concept controls only where available; unsupported actions do nothing.
- Hardware brightness, display color calibration and decoded-audio visualizers are preserved in firmware, not emulated here.
- Fonts in some original technical concepts use Google Fonts with browser fallbacks. Artwork is embedded in the individual pages. The gallery itself has no framework, analytics or account requirement.
- Preview adapters do not change firmware or the frozen `prototype-baseline` tag.

## Run locally

From the repository root:

```sh
python -m http.server 8000 --directory docs
```

Open http://localhost:8000/. The gallery needs HTTP because it loads a local JSON theme manifest. The individual HTML pages in `docs/demo/themes/` can also be opened directly; their built-in buttons work without the gallery.

## Hosting

GitHub Pages serves the `main` branch's `/docs` directory. `.nojekyll` enables plain static-file hosting. No build service or secret is needed. The repository README links directly to the public demo.

Source provenance is listed in `docs/demo/themes.json`: each entry identifies its original saved HTML filename and selected variant. Adapted copies add standalone page wrappers, shared styles, variant selection, sizing and keyboard message handling; the firmware source snapshots and checksums stay unchanged. Artwork and third-party marks retain the notices and rights described in the repository's `THIRD_PARTY_NOTICES.md`.
