# Wiring / Pinout — ESP32 (WeMos D1 R32) + microSD (SPI) + WS2812

Target board: **WeMos / LOLIN D1 R32** — an ESP32 in the Arduino-UNO form
factor. Participants fly-wire a microSD module (SPI) and a WS2812 strip to the
Uno-style headers.

> Most "ESP32 UNO" clones share this exact GPIO mapping. If your board's
> silk-screen differs, match by **GPIO number**, not by the `Dx` label.

## Connection table

| Function          | D1 R32 header pin | ESP32 GPIO | Wire to (module / strip)        |
|-------------------|-------------------|-----------:|---------------------------------|
| SD — Chip Select  | `D10`             | GPIO5      | microSD `CS` / `SS`             |
| SD — MOSI         | `D11`             | GPIO23     | microSD `MOSI` / `DI` / `CMD`   |
| SD — MISO         | `D12`             | GPIO19     | microSD `MISO` / `DO` / `DAT0`  |
| SD — Clock        | `D13`             | GPIO18     | microSD `SCK` / `CLK`           |
| SD — Power        | `5V`              | —          | microSD `VCC` (5 V)             |
| SD — Ground       | `GND`             | —          | microSD `GND`                   |
| WS2812 — Data     | `D2`              | GPIO26     | strip `DIN` (via 330–470 Ω)     |
| WS2812 — Power    | `5V`              | —          | strip `+5V`                     |
| WS2812 — Ground   | `GND`             | —          | strip `GND`                     |

These pins are the ESP32's default hardware SPI (VSPI) bus, so the sketch just
calls `SD.begin(5)` — no custom SPI pin setup needed.

## Schematic (fly-wire)

```
   ESP32 (WeMos D1 R32)          microSD module (SPI)
  +--------------------+         +----------------------+
  |           D10/IO5  +---------+ CS                   |
  |          D11/IO23  +---------+ MOSI / DI            |
  |          D12/IO19  +---------+ MISO / DO            |
  |          D13/IO18  +---------+ SCK / CLK            |
  |                5V  +---------+ VCC (5 V)            |
  |               GND  +---------+ GND                  |
  |                    |         +----------------------+
  |                    |
  |                    |         WS2812 strip
  |                    |         +----------------------+
  |           D2/IO26  +--[330R]-+ DIN                  |
  |                5V  +---------+ +5V                  |
  |               GND  +---------+ GND                  |
  +--------------------+         +----------------------+

   [330R] = 330-470 ohm series resistor at the strip's DIN.
   Also add ~1000 uF (>=6.3 V) across the strip's +5V / GND, near the strip.
   All grounds (ESP32, SD module, strip, 5 V supply) must be common.
```

## Wiring rules (do not skip)

1. **Common ground.** The strip's GND, the SD module's GND, the 5 V supply GND
   and the ESP32 GND must all be tied together. A floating ground = no data /
   flicker / garbage colours.
2. **Series resistor on data.** 330–470 Ω in series at the strip's `DIN`
   protects the first pixel and tames ringing on the fly-wire.
3. **Bulk capacitor.** ~1000 µF (≥6.3 V) across the strip's `+5V`/`GND`, close
   to the strip, absorbs the inrush when many LEDs switch.
4. **3.3 V data into a 5 V strip.** The ESP32 drives the data line at 3.3 V.
   For a handful of LEDs on short wires it usually works. For reliability:
   - add a level shifter (74AHCT125 / 74HCT245 are common choices), **or**
   - power the strip from ~4.5 V so its logic-high threshold drops, **or**
   - keep the data wire short and put the series resistor right at `DIN`.
5. **Power budget.** One WS2812 ≈ 60 mA at full white. The D1 R32's `5V` pin
   (USB-powered) is fine for ~10–15 LEDs at the default `BRIGHTNESS 128`. For
   more LEDs or full brightness, feed `+5V` from an external 5 V supply and
   **still share ground** with the ESP32. Do not back-feed a big supply into
   USB; power the strip directly from the external supply's 5 V.

## Pins reserved / avoid for WS2812 data

`GPIO6–11` (flash), input-only `GPIO34/35/36/39` (A2–A5), and strapping pins
`GPIO0/2/12/15` are poor choices for the data line. `D2 = GPIO26` (used here)
is a clean, output-capable, non-strapping pin.
