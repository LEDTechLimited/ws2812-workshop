/*
 * WS2812 ".LED" Player — DIY Workshop firmware
 * ------------------------------------------------------------------
 * Board   : WeMos / LOLIN D1 R32  (ESP32, Arduino-UNO form factor)
 * Purpose : On power-up, read a Light-Dance-Pro Studio ".LED" export
 *           from a microSD card (SPI) and play it on ONE WS2812 strip,
 *           looping forever. No buttons, no network — just boot & play.
 *
 * Libraries (install via Arduino IDE → Library Manager):
 *   - "NeoPixelBus by Makuna"   (robust WS2812 driver; NeoGrbFeature feeds
 *                                RGB and emits GRB on the wire, as WS2812 wants)
 *   - "SD" / "SPI"              (bundled with the ESP32 Arduino core)
 *
 * Board support: install "esp32 by Espressif Systems" boards package,
 *   then select Tools → Board → "WEMOS D1 R32 (esp32)" (a.k.a. D1 R32).
 *
 * .LED file format  (Studio "Sequence Export" — see docs/LED_FILE_FORMAT.md):
 *   byte 0..1 : LED count           (uint16, BIG-endian)
 *   byte 2..3 : frame interval (ms)  (uint16, BIG-endian; Studio = 25 → 40 fps)
 *   byte 4..  : frames; each frame = ledCount * 3 bytes, one RGB triple/LED
 *   The pixel bytes are in the gear's pixel order — KEEP THE GEAR AT "RGB".
 *
 * Status colours (so a problem is obvious during hands-on, with no laptop):
 *   - SOLID WHITE       : SD card problem (not detected / wiring / not FAT32).
 *   - RAINBOW fade       : SD OK, but no ".LED" file in the card root.
 *   - BLINKING RED (+LED): a ".LED" file is present but bad/unreadable, or OOM.
 *   (normal play = your show; the rainbow fade reuses the LDPS node's
 *    "identify" hue-rotation logic.)
 * ------------------------------------------------------------------
 */

#include <SPI.h>
#include <SD.h>
#include <NeoPixelBus.h>

// ════════════════════════ User parameters ════════════════════════

// WS2812 DATA pin → D1 R32 header "D2"  (= ESP32 GPIO26).
#define PIN_WS2812          26

// microSD module (SPI) Chip-Select → D1 R32 header "D10"  (= GPIO5).
// The other SPI lines are FIXED on the D1 R32 Uno header and need no #define:
//   SCK = D13/GPIO18 , MISO = D12/GPIO19 , MOSI = D11/GPIO23.
#define PIN_SD_CS           5

// LED count.  0 = read it from the .LED file header (RECOMMENDED — it always
// matches the gear you drew in Studio).  Set > 0 to FORCE a fixed count, e.g.
// to light only the first N pixels of the pattern you hand-built.
#define LED_COUNT_OVERRIDE  0

// Global brightness 0..255 (scales every pixel: out = value * BRIGHTNESS / 256).
// Default 77 ≈ 30% — the exported show looks too bright at full output, and a
// lower level keeps current/heat down on USB power. Raise toward 255 only with
// a proper 5 V supply.
#define BRIGHTNESS          77

// "" = auto-play the first *.LED file in the SD root.  Otherwise play this file.
#define FIXED_FILENAME      ""

// How many LEDs to light for the status patterns (white / rainbow) when the real
// count is not known yet (SD/file errors happen before the header is read). If
// LED_COUNT_OVERRIDE is set, that is used instead. Kept modest so a solid-white
// fault on USB power does not brown out the board.
#define STATUS_LED_COUNT    30

// ════════════════════════ Internals ══════════════════════════════

typedef NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod> StripBus;

static const uint32_t DATA_OFFSET = 4;   // bytes 0..3 are the header

StripBus*  strip       = nullptr;
File       showFile;
uint16_t   fileLeds    = 0;   // LED count stored in the file (frame layout)
uint16_t   ledCount    = 0;   // LEDs we actually drive (override or fileLeds)
uint16_t   frameMs     = 25;  // ms per frame (from header)
uint32_t   frameBytes  = 0;   // bytes per frame in the file = fileLeds * 3
uint32_t   totalFrames = 0;
uint32_t   frameIndex  = 0;
uint8_t*   frameBuf    = nullptr;
uint32_t   nextFrameAt = 0;

static uint16_t be16(const uint8_t* p) { return (uint16_t(p[0]) << 8) | p[1]; }
static bool     findShowFile(char* out, size_t outLen);
static void     haltWhite(const char* msg);    // SD problem
static void     haltRainbow(const char* msg);  // no .LED file
static void     haltError(const char* msg);    // .LED present but bad / OOM

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[Player] WS2812 .LED player booting...");

  // 1) microSD over the default VSPI bus (SCK18 / MISO19 / MOSI23), CS = GPIO5.
  //    Fault here = white (the card is unreadable, so we can't know more).
  if (!SD.begin(PIN_SD_CS)) haltWhite("SD init failed - check wiring / card / format (FAT32)");
  Serial.println("[Player] SD card OK");

  // 2) Find the show file. No file = rainbow (SD works, just nothing to play).
  char path[64];
  if (strlen(FIXED_FILENAME) > 0) {
    snprintf(path, sizeof(path), "/%s", FIXED_FILENAME);
  } else if (!findShowFile(path, sizeof(path))) {
    haltRainbow("No *.LED file found in SD root");
  }
  Serial.printf("[Player] Playing: %s\n", path);

  showFile = SD.open(path, FILE_READ);
  if (!showFile) haltError("Cannot open show file");

  // 3) Read the 4-byte header. A present-but-bad file = red.
  uint8_t hdr[4];
  if (showFile.read(hdr, 4) != 4) haltError("File too small (no header)");
  fileLeds = be16(&hdr[0]);
  frameMs  = be16(&hdr[2]);
  if (frameMs  == 0)    frameMs = 25;     // guard against a 0 = infinite-fps file
  if (fileLeds == 0)    haltError("Header LED count is 0");
  if (fileLeds > 2000)  haltError("Header LED count implausibly large - wrong/corrupt file?");

  ledCount   = (LED_COUNT_OVERRIDE > 0) ? (uint16_t)LED_COUNT_OVERRIDE : fileLeds;
  frameBytes = (uint32_t)fileLeds * 3;    // frame layout always follows the FILE's count

  uint32_t dataBytes = showFile.size() - DATA_OFFSET;
  totalFrames = dataBytes / frameBytes;
  if (totalFrames == 0) haltError("No frames in file");

  Serial.printf("[Player] fileLEDs=%u  drivingLEDs=%u  frame=%ums (%ufps)  frames=%lu\n",
                fileLeds, ledCount, frameMs, 1000u / frameMs, (unsigned long)totalFrames);

  // 4) Allocate the per-frame read buffer and the strip.
  frameBuf = (uint8_t*)malloc(frameBytes);
  if (!frameBuf) haltError("Out of memory for frame buffer");

  strip = new StripBus(ledCount, PIN_WS2812);
  strip->Begin();
  strip->ClearTo(RgbColor(0));
  strip->Show();

  frameIndex  = 0;
  nextFrameAt = 0;
  Serial.println("[Player] Playback started");
}

void loop() {
  // Read one frame's worth of bytes (the file stores fileLeds * 3 per frame).
  if ((uint32_t)showFile.read(frameBuf, frameBytes) != frameBytes) {
    // Short read / EOF -> loop back to the first frame.
    showFile.seek(DATA_OFFSET);
    frameIndex = 0;
    return;
  }

  // Paint min(ledCount, fileLeds) pixels; any extra physical pixels stay dark.
  uint16_t paint = (ledCount < fileLeds) ? ledCount : fileLeds;
  for (uint16_t i = 0; i < paint; i++) {
    const uint8_t* p = &frameBuf[i * 3];          // file bytes are R,G,B (gear = RGB)
    uint8_t r = ((uint16_t)p[0] * BRIGHTNESS) >> 8;
    uint8_t g = ((uint16_t)p[1] * BRIGHTNESS) >> 8;
    uint8_t b = ((uint16_t)p[2] * BRIGHTNESS) >> 8;
    strip->SetPixelColor(i, RgbColor(r, g, b));   // NeoGrbFeature -> GRB on the wire
  }
  for (uint16_t i = paint; i < ledCount; i++) strip->SetPixelColor(i, RgbColor(0));

  strip->Show();

  // Pace output to the file's frame rate (drift-corrected).
  if (nextFrameAt == 0) nextFrameAt = millis();
  nextFrameAt += frameMs;
  int32_t wait = (int32_t)(nextFrameAt - millis());
  if (wait > 0) delay(wait);
  else          nextFrameAt = millis();           // fell behind -> resync clock

  // Loop the show when we reach the end.
  if (++frameIndex >= totalFrames) {
    showFile.seek(DATA_OFFSET);
    frameIndex = 0;
  }
}

// ───────────────────────── Helpers ─────────────────────────

// Last path component of an SD entry name ("/FOO.LED" -> "FOO.LED").
static const char* baseName(const char* n) {
  const char* slash = strrchr(n, '/');
  return slash ? slash + 1 : n;
}

// True if the name ends in ".LED" (any case).
static bool hasLedExt(const char* base) {
  size_t len = strlen(base);
  if (len < 4) return false;
  const char* e = base + len - 4;
  return e[0] == '.' &&
         (e[1] == 'L' || e[1] == 'l') &&
         (e[2] == 'E' || e[2] == 'e') &&
         (e[3] == 'D' || e[3] == 'd');
}

// Scan the SD root for the first PLAYABLE *.LED file.
//
// HIDDEN-FILE SAFETY: SD cards almost always carry hidden / system files, and
// some would otherwise be mistaken for the show:
//   - macOS writes an AppleDouble companion "._<name>.LED" next to EVERY real
//     file on a FAT volume. It ends in ".LED" but is a tiny metadata blob — if
//     the player opened it instead of the real show, the header would be junk.
//   - ".DS_Store", ".Spotlight-V100", ".Trashes", ".fseventsd" (macOS) and
//     "System Volume Information" (Windows) also live in the root.
// All hidden / AppleDouble entries begin with a dot, so we skip any entry whose
// basename starts with '.'. Directories (incl. the dot-dirs above and Windows'
// "System Volume Information") are skipped by the isDirectory() check.
static bool findShowFile(char* out, size_t outLen) {
  File root = SD.open("/");
  if (!root) return false;
  bool found = false;
  for (File f = root.openNextFile(); f; f = root.openNextFile()) {
    if (!f.isDirectory()) {
      const char* n    = f.name();                // "FOO.LED" or "/FOO.LED"
      const char* base = baseName(n);
      if (base[0] != '.' && hasLedExt(base)) {    // skip hidden / "._" companions
        if (n[0] == '/') snprintf(out, outLen, "%s", n);
        else             snprintf(out, outLen, "/%s", n);
        found = true;
        f.close();
        break;
      }
    }
    f.close();
  }
  root.close();
  return found;
}

// ─────────────────── Status / fault indicators ───────────────────
// These are TERMINAL states (the board must be fixed + power-cycled). The real
// LED count is unknown when SD/file errors occur — the strip is created here
// with STATUS_LED_COUNT (or LED_COUNT_OVERRIDE if set) just for the indicator.
// Each boot creates the strip exactly once: either here, or in setup() for play.

static void ensureStatusStrip() {
  if (strip) return;
  uint16_t n = (LED_COUNT_OVERRIDE > 0) ? (uint16_t)LED_COUNT_OVERRIDE : STATUS_LED_COUNT;
  strip = new StripBus(n, PIN_WS2812);
  strip->Begin();
}

// HSV→RGB, all pixels one colour, hue rotating with time (~3 s/cycle).
// Ported from the LDPS Edge-Node "identify" tick (playback_engine.cpp).
static void rainbowColor(uint32_t now, uint8_t& r, uint8_t& g, uint8_t& b) {
  uint16_t hue       = ((now / 8) * 170) & 0xFFFF;   // ~3 s full cycle
  uint8_t  region    = hue / 10923;
  uint16_t remainder = (hue - region * 10923) * 6;
  uint8_t  q = 255 - (remainder >> 8);
  uint8_t  t = (remainder >> 8);
  switch (region) {
    case 0: r = 255; g = t;   b = 0;   break;
    case 1: r = q;   g = 255; b = 0;   break;
    case 2: r = 0;   g = 255; b = t;   break;
    case 3: r = 0;   g = q;   b = 255; break;
    case 4: r = t;   g = 0;   b = 255; break;
    default:r = 255; g = 0;   b = q;   break;
  }
}

// SD problem → SOLID WHITE (scaled by BRIGHTNESS so it stays power-safe).
static void haltWhite(const char* msg) {
  Serial.printf("[Player] SD FAULT (solid white): %s\n", msg);
  ensureStatusStrip();
  strip->ClearTo(RgbColor(BRIGHTNESS, BRIGHTNESS, BRIGHTNESS));
  strip->Show();
  for (;;) { strip->Show(); delay(1000); }   // hold + periodic re-latch
}

// No .LED file → RAINBOW fade (node "identify" hue rotation).
static void haltRainbow(const char* msg) {
  Serial.printf("[Player] NO .LED FILE (rainbow): %s\n", msg);
  ensureStatusStrip();
  for (;;) {
    uint8_t r, g, b;
    rainbowColor(millis(), r, g, b);
    r = ((uint16_t)r * BRIGHTNESS) >> 8;
    g = ((uint16_t)g * BRIGHTNESS) >> 8;
    b = ((uint16_t)b * BRIGHTNESS) >> 8;
    strip->ClearTo(RgbColor(r, g, b));
    strip->Show();
    delay(16);   // ~60 fps colour fade
  }
}

// .LED present but bad/unreadable (or OOM) → BLINKING RED + on-board LED.
static void haltError(const char* msg) {
  Serial.printf("[Player] FILE ERROR (blinking red): %s\n", msg);
  ensureStatusStrip();
  pinMode(LED_BUILTIN, OUTPUT);
  for (;;) {
    strip->ClearTo(RgbColor(BRIGHTNESS, 0, 0)); strip->Show();
    digitalWrite(LED_BUILTIN, HIGH); delay(200);
    strip->ClearTo(RgbColor(0));                strip->Show();
    digitalWrite(LED_BUILTIN, LOW);  delay(200);
  }
}
