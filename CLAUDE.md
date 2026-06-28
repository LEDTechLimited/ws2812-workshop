# WS2812 + microSD DIY Workshop

A boot-and-play LED show: an ESP32 (WeMos D1 R32) reads a `.LED` file from a microSD card and drives
a WS2812 strip — no buttons / network / app. Participants wire LEDs, design a gear + sequence in
Light-Dance-Pro Studio, export `.LED`, copy to SD, power on → it plays and loops.

> **Keep this file lean** (agent entry, <200 lines). Hard rules + build/flash + pointers; full BOM
> and facilitator setup are in [README.md](README.md), detail in `docs/`.

## Docs convention
Closed-bucket convention — [`docs/index.md`](docs/index.md). State + open items in [`STATUS.md`](STATUS.md).

## Build / flash (Arduino — ESP32 D1 R32)
Sketch: `LedPlayer/LedPlayer.ino`. **Verified toolchain** (compiles clean, flash 26% / RAM 6%):
- ESP32 Arduino core `esp32:esp32` **3.3.10** · NeoPixelBus by Makuna **2.8.4** · FQBN `esp32:esp32:d1_uno32` · arduino-cli **1.5.1**.
- `arduino-cli compile --fqbn esp32:esp32:d1_uno32 LedPlayer` then `arduino-cli upload -p <port> --fqbn esp32:esp32:d1_uno32 LedPlayer` (or Arduino IDE).

## Hard rules
- **Match the verified toolchain versions above** — other versions may not compile/behave.
- microSD must be **FAT32**; WS2812 data needs a 330–470Ω resistor + ~1000µF cap (3.3→5V level shifter recommended).

## Where to look
- Wiring / pinout → [`docs/reference/PINOUT.md`](docs/reference/PINOUT.md)
- `.LED` file format → [`docs/reference/LED_FILE_FORMAT.md`](docs/reference/LED_FILE_FORMAT.md)
- BOM / facilitator setup / workshop flow → [README.md](README.md)
- Current state / what's left → [`STATUS.md`](STATUS.md)
