# FlashBang

FlashBang is a four-channel flash trigger for high-speed photography, built on an ESP32. A sound, an external trigger signal or the camera's sync contact starts a sequence of flashes. You can set the timing per channel down to 0.1 ms, from a rotary encoder menu or from a web page on your phone.

Typical uses: popping balloons, water drops, breaking glass and other short events in a dark room with the shutter open (bulb), or multi-flash stroboscopic shots.

## Features

- **Three trigger inputs, in any combination:**
  - a sound module (with a sensitivity trimpot)
  - an external 3.3 V logic input, active low or active high
  - the camera's PC sync contact

  The first input to fire wins. A holdoff time after each shot blocks re-triggering.
- **Manual fire button.** It always works, even with a menu open, so you can test a setup.
- **Four optocoupler outputs**, one per flash.
- **Three flash modes:**
  - **Single:** the selected channels fire together, once, after one delay.
  - **Multi:** each channel has its own flash count, interval and start delay. Use it for stroboscopic shots.
  - **Sequence:** a free list of up to 50 (time, channel) entries.

  Delays go up to 5 minutes. Up to 100 flashes per sequence.
- **Status screen on a 128×64 OLED** showing:
  - the preset name and battery %
  - the armed / fired state
  - the trigger source and sensitivity
  - the flash count per channel
  - the source of the last trigger and a shot counter
- **Quick adjust:** turning the encoder on the status screen shifts the whole timing. Slow turns give fine steps; fast turns give bigger steps.
- **WiFi config page:** the device opens its own access point with a captive portal. The page holds every setting and up to 8 named presets.
- **OTA firmware updates** over the same access point.
- **Battery monitoring** with a runtime estimate for the 18650 cell.
- **Power saving:**
  - the main loop sleeps until an interrupt or a deadline
  - the display switches off after a timeout
  - the CPU frequency can be set to 80, 160 or 240 MHz

## Hardware

| Part | Notes |
|---|---|
| LilyGO TTGO T-Energy | ESP32-WROVER with PSRAM and an 18650 holder. Built as `esp-wrover-kit`. |
| SH1106 128×64 OLED, I²C | Address 0x3C (labelled 0x78 on the back of most boards) |
| Rotary encoder with push button | |
| Sound module with digital output and trimpot | e.g. an LM393-based module; DO goes low on sound |
| 4 × optocoupler | One per flash. The ESP32 drives the LED side; the transistor side shorts the flash's trigger contacts. |
| Push button | Manual fire |
| PC sync socket, 10 kΩ + 1 kΩ | Optional |
| 3.5 mm jack | Optional, for an external trigger |

### Pinout

| GPIO | Function |
|---|---|
| 23 | Sound module DO |
| 34 | Sound module trimpot wiper (reads the sensitivity, ADC) |
| 35 | Battery divider (on the board) |
| 13 | External trigger input, **3.3 V logic only** |
| 14 | PC sync contact: 10 kΩ pull-up to 3V3, 1 kΩ in series to the socket |
| 4 | Manual fire button to GND |
| 32 | Encoder push button to GND |
| 18 / 19 | Encoder A (CLK) / B (DT) |
| 21 / 22 | I²C SDA / SCL to the OLED |
| 26, 25, 27, 33 | Optocouplers for channels 1–4 (active high) |
| 5 | Onboard LED (held low) |

GPIO16 and 17 are taken by the PSRAM on the T-Energy, so they are not used. The encoder pins must stay below GPIO32, because the interrupt reads them straight from the GPIO register. All pin assignments are in [src/config.h](src/config.h).

## Building and flashing

You need [PlatformIO](https://platformio.org/), either the VS Code extension or the CLI. The platform is [pioarduino](https://github.com/pioarduino/platform-espressif32) (Arduino core 3.x).

```sh
# Build and upload over USB
pio run -e ttgo-t-energy -t upload

# Serial monitor
pio device monitor
```

### Over the air

1. On the device, open the menu and choose **OTA Update**.
2. Join the open WiFi network **FlashBang** from your computer.
3. Run:
   ```sh
   pio run -e ota -t upload
   ```
4. The display shows the progress. The device restarts when the upload is done.

Hold the encoder button to leave OTA mode without updating.

## Using it

**Status screen**

| Control | Action |
|---|---|
| Turn the encoder | Shift the timing |
| Click | Open the menu |
| Fire button | Test shot |

**Menu**

| Control | Action |
|---|---|
| Turn | Move through the items |
| Click | Select or edit |
| Long press (0.8 s) | Back, or cancel an edit |

The menu closes by itself after 30 s without input. Sound, external and PC sync triggers are off while a menu is open. The fire button still works there.

**WiFi setup**

1. Choose **WiFi setup** in the menu.
2. Join the open network **FlashBang**. The captive portal should open the config page by itself; if not, browse to `192.168.4.1`.
3. Change settings, then save or load presets.

Hold the encoder button to leave config mode and apply the settings. Triggers are off while config mode is running.

Settings are stored in the ESP32's flash (NVS) and survive a reflash.

### Settings

- **Flash setup** (stored in presets): mode, delays, channels, counts and intervals, the sequence, the trigger source and the external input level.
- **Holdoff:** how long sound and the other inputs are ignored after a shot.
- **Display on-time:** how long the screen stays lit after a shot or input.
- **Battery:** calibration factor (compare with a multimeter), cell capacity and average current draw. The capacity and current are used for the runtime estimate.
- **CPU frequency:** 80, 160 or 240 MHz. Lower saves power.

## Project layout

```
src/
  main.cpp       setup() and the main loop: triggers, holdoff, housekeeping
  config.h       pins, constants, enums, the settings structs and their defaults
  state.h/.cpp   global state shared between the modules
  inputs.*       interrupts (triggers, fire button, encoder), button polling, arming
  schedule.*     turns the setup into a timed list of flash events and fires it
  display.*      OLED screens and display power
  menu.*         on-device menu and quick adjust
  webui.*        WiFi config page and its form handlers
  net.*          access point, captive portal, OTA
  storage.*      loading and saving settings and presets (NVS), value clamping
  sensors.*      sensitivity pot and battery
```

## Safety

Some flashes, especially older ones, put **hundreds of volts** on their trigger contacts. The optocouplers keep that voltage away from the ESP32. Check the trigger voltage of your flash, and use an optocoupler or SCR rated for it. The external trigger input takes 3.3 V logic only.

## License

[MIT](LICENSE)
