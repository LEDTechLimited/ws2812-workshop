# WS2812 + microSD DIY Workshop

A hands-on demo where participants:

1. Build their own **WS2812** LED pattern (decide their own LED count from the
   shape they wire up).
2. Draw a matching **gear** in **Light-Dance-Pro Studio**, add sequence
   **effects**, and **export a `.LED`** file.
3. Fly-wire an **ESP32 (WeMos D1 R32)** + a **microSD module (SPI)**, copy the
   `.LED` file to the card, and **power on → the show plays and loops**.

No buttons, no network, no app — boot and play.

---

## Bill of materials (per participant)

| Item | Notes |
|------|-------|
| WeMos / LOLIN **D1 R32** (ESP32, Uno form factor) | other ESP32-UNO clones OK — match by GPIO |
| **microSD module (SPI)** + microSD card | FAT32-formatted |
| **WS2812 / WS2812B** strip or pixels | participant decides the count |
| Jumper wires (fly-wire) | — |
| 330–470 Ω resistor, ~1000 µF capacitor | data protection + bulk power |
| USB cable (power + flashing) | external 5 V supply for larger strips |
| *(recommended)* 74AHCT125 / 74HCT245 level shifter | 3.3 V → 5 V data, robust colour |

---

## Software setup (facilitator, once per laptop)

Pick **one** of the two paths below. Both use the same known-good toolchain,
verified to compile this sketch cleanly (flash 26 %, RAM 6 %):

| Component | Verified version |
|-----------|------------------|
| ESP32 Arduino core (`esp32:esp32`) | 3.3.10 |
| NeoPixelBus by Makuna | 2.8.4 |
| Board FQBN | `esp32:esp32:d1_uno32` |
| arduino-cli (path B) | 1.5.1 |

### Path A — Arduino IDE (GUI)

1. Install **Arduino IDE 2.x**.
2. *Settings → Additional boards manager URLs*, add:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. *Boards Manager* (left toolbar) → search **esp32** → install
   **"esp32 by Espressif Systems"**.
4. *Library Manager* → search **NeoPixelBus** → install
   **"NeoPixelBus by Makuna"**. (`SD` and `SPI` ship with the ESP32 core.)
5. *Tools → Board* → **"WEMOS D1 R32"**.
6. Open `LedPlayer/LedPlayer.ino`, plug in the board, pick its port under
   *Tools → Port*, click **Upload**. Open *Serial Monitor* @ **115200** to see
   the boot log.

### Path B — arduino-cli (command line)

```bash
# 1. Install arduino-cli
#    macOS (Homebrew):
brew install arduino-cli
#    Linux / other: see https://arduino.github.io/arduino-cli/latest/installation/

# 2. One-time config: register the ESP32 board index
arduino-cli config init
arduino-cli config add board_manager.additional_urls \
  https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index

# 3. Install the ESP32 core (large — pulls the xtensa toolchain) + the library
arduino-cli core install esp32:esp32
arduino-cli lib install "NeoPixelBus by Makuna"

# 4. Compile (run from the repo root; folder name = sketch name)
arduino-cli compile --fqbn esp32:esp32:d1_uno32 LedPlayer

# 5. Find the board's serial port, then upload
arduino-cli board list
arduino-cli upload -p <PORT> --fqbn esp32:esp32:d1_uno32 LedPlayer
#   e.g. <PORT> = /dev/cu.usbserial-XXXX (macOS) or /dev/ttyUSB0 (Linux)

# 6. Watch the boot log
arduino-cli monitor -p <PORT> -c baudrate=115200
```

> If `board list` shows no port, install the USB-UART driver for your board's
> bridge chip (CP210x or CH340), and try a data-capable USB cable.

The default sketch parameters work out-of-the-box:

| `#define` | Default | Meaning |
|-----------|---------|---------|
| `PIN_WS2812` | `26` | WS2812 data → header `D2` |
| `PIN_SD_CS` | `5` | SD chip-select → header `D10` |
| `LED_COUNT_OVERRIDE` | `0` | `0` = read LED count from the `.LED` header |
| `BRIGHTNESS` | `128` | 0–255; keep modest on USB power |
| `FIXED_FILENAME` | `""` | `""` = auto-play first `*.LED` in SD root |

---

## Workshop flow (participant)

### A. Build the pattern
Wire the WS2812 pixels into whatever shape you like. Count your pixels — that's
your **LED count**.

### B. Design in Studio
1. Create a **gear** with the **same LED count** as your pattern.
2. Leave the gear's **pixel order = RGB** (the firmware assumes RGB — see
   `docs/LED_FILE_FORMAT.md`).
3. Add **effects** on the sequence timeline.
4. Open **Sequence Export**, pick your gear, **Export** → you get a
   `S{n}_{name}_{gear}.LED` file (40 fps).

### C. Run it
1. Copy the `.LED` file to the **root** of a FAT32 microSD card. (If there are
   several, the firmware auto-plays the first one — keep just one to be sure,
   or set `FIXED_FILENAME`.)
2. Insert the card, wire the board per **[`docs/PINOUT.md`](docs/PINOUT.md)**.
3. Power on. The strip plays your show and loops forever.

---

## How it works

On boot the firmware mounts the SD card, picks the show file, reads the 4-byte
header (LED count + frame interval), then streams frames to the strip at the
file's frame rate, looping at the end. WS2812 output uses **NeoPixelBus** (a
robust, widely-used WS2812 driver); SD access uses the Arduino `SD` library over
the ESP32's hardware SPI.

**Hidden / system files are skipped automatically.** SD cards almost always
carry hidden files, and macOS in particular writes an AppleDouble companion
`._<name>.LED` next to every real file — it ends in `.LED` but is just a tiny
metadata blob. The file scan ignores any entry whose name starts with a dot
(`._*`, `.DS_Store`, `.Spotlight-V100`, `.Trashes`, `.fseventsd`), so it always
lands on the real show. You do **not** need to "clean" the card first.

- `.LED` format: **[`docs/LED_FILE_FORMAT.md`](docs/LED_FILE_FORMAT.md)**
- Wiring / pinout: **[`docs/PINOUT.md`](docs/PINOUT.md)**
- Firmware: **[`LedPlayer/LedPlayer.ino`](LedPlayer/LedPlayer.ino)**

---

## Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| Strip flashes **red** + on-board LED blinks | Fatal error — open Serial Monitor @115200 for the reason (SD init, no `.LED` file, bad header) |
| `SD init failed` | Wiring (CS/MOSI/MISO/SCK), card not FAT32, or 5 V/GND to the module |
| Nothing lights | Common ground missing; wrong data pin; strip needs more current than USB gives |
| Wrong / swapped colours | Gear was not exported as **RGB** pixel order |
| Garbled / wrong show on a macOS-written card | The real `.LED` is fine — the firmware already skips `._*` AppleDouble files; just make sure your show file is in the card **root** |
| First pixel wrong, rest OK | 3.3 V data marginal — add a level shifter or the series resistor at `DIN` |
| Flicker / glitches on long strips | Add the bulk capacitor; use an external 5 V supply; shorten the data wire |
| Too dim / colours washed | Raise `BRIGHTNESS` (and supply enough current) |
| Plays too fast/slow | The header's frame interval is used as-is; Studio exports 25 ms (40 fps) |
