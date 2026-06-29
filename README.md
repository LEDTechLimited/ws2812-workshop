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
| `BRIGHTNESS` | `77` | ~30 % (0–255 scale); keep modest on USB power |
| `FIXED_FILENAME` | `""` | `""` = auto-play first `*.LED` in SD root |

---

## Workshop flow (participant)

### A. Build the pattern
Wire the WS2812 pixels into whatever shape you like. Count your pixels — that's
your **LED count**.

### B. Design in Studio
1. Create a **gear** with the **same LED count** as your pattern.
2. Leave the gear's **pixel order = RGB** (the firmware assumes RGB — see
   `docs/reference/LED_FILE_FORMAT.md`).
3. Add **effects** on the sequence timeline.
4. Open **Sequence Export**, pick your gear, **Export** → you get a
   `S{n}_{name}_{gear}.LED` file (40 fps).

### C. Run it
1. Copy the `.LED` file to the **root** of a FAT32 microSD card. (If there are
   several, the firmware auto-plays the first one — keep just one to be sure,
   or set `FIXED_FILENAME`.)
2. Insert the card, wire the board per the **[Wiring](#wiring-fly-wire)**
   section below.
3. Power on. The strip plays your show and loops forever.

---

## Wiring (fly-wire)

Board: **WeMos / LOLIN D1 R32** — an ESP32 in the Arduino-UNO form factor.
Fly-wire the microSD module (SPI) and the WS2812 strip to the Uno-style headers.
Most "ESP32 UNO" clones share this mapping — if your silk-screen differs, match
by **GPIO number**, not by the `Dx` label.

| Function          | D1 R32 pin | ESP32 GPIO | Wire to                         |
|-------------------|------------|-----------:|---------------------------------|
| SD — Chip Select  | `D10`      | GPIO5      | microSD `CS` / `SS`             |
| SD — MOSI         | `D11`      | GPIO23     | microSD `MOSI` / `DI` / `CMD`   |
| SD — MISO         | `D12`      | GPIO19     | microSD `MISO` / `DO` / `DAT0`  |
| SD — Clock        | `D13`      | GPIO18     | microSD `SCK` / `CLK`           |
| SD — Power        | `5V` / `3V3` | —        | microSD `VCC` (**see note ↓**)  |
| SD — Ground       | `GND`      | —          | microSD `GND`                   |
| WS2812 — Data     | `D2`       | GPIO26     | strip `DIN` (via 330–470 Ω)     |
| WS2812 — Power    | `5V`       | —          | strip `+5V`                     |
| WS2812 — Ground   | `GND`      | —          | strip `GND`                     |

These are the ESP32's default hardware SPI (VSPI) pins, so the sketch just calls
`SD.begin(5)` — no custom SPI setup. WS2812 data is GPIO26 (a clean,
output-capable, non-strapping pin).

**SD module power — 3.3 V or 5 V?** A microSD card is a **3.3 V** device and so
is the ESP32 — **never let 5 V reach an ESP32 pin**. Which `VCC` to use depends
on your module:

- **Module with a regulator *and* a level-shifter chip** (most "microSD
  modules" — a 3-pin regulator like `AMS1117` **plus** a buffer like `74VHC125`):
  power `VCC` at **5 V**. It regulates 3.3 V for the card and returns the SPI
  lines to the ESP32 at 3.3 V, so 5 V never reaches a GPIO. ✅
- **Bare module** (just the socket + a few passives; silk says 3.3 V): power
  `VCC` at **3.3 V** (the `3V3` pin) and wire SPI directly. ⚠️ Giving it 5 V
  destroys the card and pushes 5 V onto the SPI lines into the ESP32.
- **Not sure?** Power it up and measure `MISO` (→ GPIO19) at idle: it must read
  ~3.3 V, **never ~5 V**, before you connect it to the ESP32. (Beware cheap
  "5 V" modules that level-shift with resistor dividers instead of a buffer
  chip — they can misbehave on a 3.3 V host.)

```
   ESP32 (WeMos D1 R32)          microSD module (SPI)
  +--------------------+         +----------------------+
  |           D10/IO5  +---------+ CS                   |
  |          D11/IO23  +---------+ MOSI / DI            |
  |          D12/IO19  +---------+ MISO / DO            |
  |          D13/IO18  +---------+ SCK / CLK            |
  |            5V/3V3  +---------+ VCC                  |
  |               GND  +---------+ GND                  |
  |                    |         +----------------------+
  |                    |
  |                    |         WS2812 strip
  |                    |         +----------------------+
  |           D2/IO26  +--[330R]-+ DIN                  |
  |                5V  +---------+ +5V                  |
  |               GND  +---------+ GND                  |
  +--------------------+         +----------------------+

   SD VCC: 5 V for regulator+buffer modules, 3.3 V for bare ones (see note above).
   [330R] = 330-470 ohm series resistor at the strip's DIN.
   Also add ~1000 uF (>=6.3 V) across the strip's +5V / GND, near the strip.
   All grounds (ESP32, SD module, strip, 5 V supply) must be common.
```

**Wiring rules — do not skip:**

1. **Common ground.** Strip GND, SD module GND, the 5 V supply GND and the
   ESP32 GND must all be tied together. A floating ground = no data / flicker.
2. **Series resistor on data.** 330–470 Ω in series at the strip's `DIN`
   protects the first pixel and tames ringing on the fly-wire.
3. **Bulk capacitor.** ~1000 µF (≥6.3 V) across the strip's `+5V`/`GND`, close
   to the strip, absorbs inrush when many LEDs switch.
4. **3.3 V data into a 5 V strip.** The ESP32 drives data at 3.3 V. For a few
   LEDs on short wires it usually works; for reliability add a level shifter
   (74AHCT125 / 74HCT245), power the strip at ~4.5 V, or keep the data wire
   short with the resistor right at `DIN`.
5. **Power budget.** One WS2812 ≈ 60 mA at full white. At the default
   `BRIGHTNESS 77` (~30 %) the `5V` pin (USB) is fine for ~20–25 LEDs. For more
   LEDs or higher brightness, feed `+5V` from an external 5 V supply and **still
   share ground** with the ESP32.

> Full detail (pins to avoid for data, module power variants) is in
> **[`docs/reference/PINOUT.md`](docs/reference/PINOUT.md)**.

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

**Status colours — read the strip itself.** During hands-on you can tell what's
wrong from the LEDs alone, no laptop needed:

| The strip shows… | Meaning |
|---|---|
| **your show** (loops) | all good |
| **solid white** | SD card problem — not detected, wiring, or not FAT32 |
| **rainbow fade** | SD is fine, but there is no `.LED` file in the card root |
| **blinking red** (+ on-board LED) | a `.LED` file is present but bad / unreadable (or out of memory) |

The rainbow fade reuses the LDPS node's "identify" hue-rotation. Status patterns
light up to `STATUS_LED_COUNT` (default 30) LEDs since the real count isn't known
until a valid file is read.

- `.LED` format: **[`docs/reference/LED_FILE_FORMAT.md`](docs/reference/LED_FILE_FORMAT.md)**
- Wiring / pinout: **[`docs/reference/PINOUT.md`](docs/reference/PINOUT.md)**
- Firmware: **[`LedPlayer/LedPlayer.ino`](LedPlayer/LedPlayer.ino)**

---

## Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| **Solid white** strip | SD problem — wiring (CS/MOSI/MISO/SCK), card not FAT32, or wrong `VCC` for the module type (regulator+buffer module → 5 V; bare module → 3.3 V — see Wiring note) |
| **Rainbow fade** strip | SD mounts but no `.LED` in root — copy your file (or check it's not hidden / in a subfolder) |
| **Blinking red** + on-board LED | `.LED` present but bad/corrupt/unreadable, or OOM — open Serial Monitor @115200 for the exact reason |
| Nothing lights | Common ground missing; wrong data pin; strip needs more current than USB gives |
| Wrong / swapped colours | Gear was not exported as **RGB** pixel order |
| Garbled / wrong show on a macOS-written card | The real `.LED` is fine — the firmware already skips `._*` AppleDouble files; just make sure your show file is in the card **root** |
| First pixel wrong, rest OK | 3.3 V data marginal — add a level shifter or the series resistor at `DIN` |
| Flicker / glitches on long strips | Add the bulk capacitor; use an external 5 V supply; shorten the data wire |
| Too dim / colours washed | Raise `BRIGHTNESS` (and supply enough current) |
| Plays too fast/slow | The header's frame interval is used as-is; Studio exports 25 ms (40 fps) |
