# Controls and display settings

| Input | Playback | Menu |
|---|---|---|
| Wheel | Volume | Scroll selection |
| Center | Play/pause | Select |
| Hold center | Library | Context actions on song rows; cancel calibration session |
| Up | Songs | Context-dependent navigation |
| Down | Queue | Context-dependent navigation |
| Left / right | Previous / next song | Back / context-dependent action |
| Hold left / right | Seek by 10 seconds | Context-dependent |

Latest checkpoint retains Library > Settings > Appearance > Themes with 20 themes. See `firmware/StrandPlayer20/SELECTED-THEMES.json` for the archived selection list and `docs/previews/` for saved native renderer images.

Settings > Display > Brightness gives live 5-100% PWM control on GPIO21; Center saves and Left cancels. Saved brightness persists in NVS.

Settings > Display > Color calibration provides RGB gain, gamma, saturation and contrast with color bars, grayscale, near-black patches and RGB ramps. Save/Cancel/Reset Preview are retained. These are software display adjustments, not a measured colorimeter profile or a backlight-bleed fix.

Long title, artist, album and selected menu text scroll within clipped regions in the latest snapshot. Visualizer themes use decoded PCM amplitude/time history and RMS, not FFT spectrum, random bars or synthetic test waves. The saved target is about 33fps; actual frame rate and audio continuity remain device-test items.

MP3 library browsing, queue, playlists, shuffle and repeat are preserved. The early player supports limited embedded baseline JPEG album artwork; the latest illustrated themes use embedded flash artwork instead. No FLAC, Bluetooth audio or automatic speaker switching is claimed by this baseline.
