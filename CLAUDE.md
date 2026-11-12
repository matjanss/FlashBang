# CLAUDE.md

FlashBang: ESP32 firmware (Arduino framework, PlatformIO) for a 4-channel flash trigger. The board is a LilyGO TTGO T-Energy. User-facing docs are in README.md.

## Build

- `pio run -e ttgo-t-energy`: build. Add `-t upload` to upload over USB.
- `pio run -e ota -t upload`: OTA upload. The device must be in OTA mode, with the computer joined to the "FlashBang" AP (192.168.4.1).
- There are no tests. Verify a change by building it; the user tests on the hardware.
- If `pio` is not on PATH, use `~/.platformio/penv/bin/pio`.

## Architecture

- **Includes:** every `.cpp` includes `app.h`, which pulls in all module headers.
  - `config.h`: constants and types only.
  - `state.h` / `state.cpp`: the shared globals (`extern` declarations and their definitions).
- **Triggers:** ISRs in `inputs.cpp` stamp `micros()` into `hitUs`, set `hit` and `hitSrc`, and wake the main task with `vTaskNotifyGiveFromISR`. The first trigger wins, because each ISR checks `!hit`.
- **Main loop:** `main.cpp` sleeps in `ulTaskNotifyTake` until an interrupt or the next deadline (menu timeout, display off, holdoff end, delayed save). Keep it event-driven; don't add polling at a fixed rate.
- **Flash schedule:** `buildSchedule()` compiles the current `Setup` into `evT` / `evM` / `evN`: sorted event times in µs, with a channel bitmask per event. `fireSchedule(t0)` plays it, timed from the interrupt timestamp. Call `buildSchedule()` after every change to `cur`.
- **Pulse width:** `evPw` starts at 2 ms and is shortened automatically so that fast repeats on one channel can re-fire.
- **Holdoff:** after a shot, `hit` stays true and `holdoff` blocks re-triggering until `holdoffEnd`. While a menu is open, triggers are ignored.
- **Settings:** `Setup` (`cur`) is what presets store. `Config` (`cfg`) holds the system settings.
  - Both are saved to NVS namespace `"cfg"` in `storage.cpp`, and pass through `clampSetup` / `clampConfig` on load.
  - `dirty` / `saveAt` delay saves so the encoder doesn't wear the flash.
- **Changing the `Setup` or `Preset` struct:** this changes the layout of the presets stored as blobs (`p0`..`p7`). Old blobs then fail the size check and are dropped.
- **Display:** a full redraw takes about 25 ms over I²C. Only redraw when something changed, and never in the trigger path.
- **Menu:** `menu.cpp` is table-driven (`Item` / `PageDef`). An item's `modes` bitmask and `minc` decide whether it is shown.
- **Web UI:** `webui.cpp` builds the page as a `String` from `F()` literals. Forms POST to `/save`, `/preset_*` and `/reset`.

## Conventions

- **Version:** `FW_VERSION` / `FW_NAME` in `config.h`. The web page title and heading in `webui.cpp` repeat it, and every file starts with a `// FlashBang V3.0 - ...` header comment. Update all of these together.
- **Comments:** plain English, explaining why. Units go in names (`_us`, `_ms`).
- **Timing:** use µs (`int`, max `MAX_DELAY_US` = 300 s), and compare times with wrap-safe signed differences: `(long)(millis() - deadline) >= 0`.
- **Pins:** everything is in `config.h`. On the T-Energy, GPIO16/17 belong to the PSRAM and GPIO35 is the battery divider.
