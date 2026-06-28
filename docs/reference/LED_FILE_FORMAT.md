# `.LED` file format (Light-Dance-Pro Studio "Sequence Export")

This is the format the **workshop** uses. It is produced by the Light-Dance-Pro
Studio **"Sequence Export"** dialog, **one `.LED` file per gear**. Studio fixes
playback at **40 fps (25 ms/frame)**.

## Layout

```
Offset  Size            Field            Notes
------  --------------  ---------------  --------------------------------------
0x00    2 bytes (u16)   LED count        BIG-endian. Pixels in this gear/file.
0x02    2 bytes (u16)   frame interval   BIG-endian, milliseconds. Studio = 25.
0x04    ledCount*3      frame 0          R,G,B per LED, in LED index order.
…       ledCount*3      frame 1
…       …               …
        ledCount*3      frame N-1
```

- **File size** = `4 + totalFrames * (ledCount * 3)`.
- **`totalFrames`** is therefore `(fileSize - 4) / (ledCount * 3)` — it is NOT
  stored explicitly.
- **Bytes per pixel: 3** (no white/alpha channel).
- **Frame rate is fixed at 40 fps** (25 ms/frame) by the Studio exporter, but
  the firmware reads the value from the header rather than assuming it.

## What the header does and does NOT contain

- ✅ **LED count** — present (bytes 0–1). The workshop firmware reads it, so a
  participant does not have to type it in. `LED_COUNT_OVERRIDE` in the sketch
  can still force a different count for experiments.
- ✅ **Frame interval / fps** — present (bytes 2–3).
- ❌ **Channel count** — not present. The `.LED` export is single-gear /
  single-strip, i.e. effectively **1 channel**.
- ❌ **Magic number / version / CRC** — none. It is a bare header + raw frames.

## Colour order

The exporter applies the **gear's pixel order** to the bytes as it writes the
file. The default is **RGB**.

- Keep the gear at **RGB** in Studio. The firmware then feeds the bytes to
  NeoPixelBus `NeoGrbFeature`, which reorders RGB → GRB on the wire — exactly
  what a WS2812 expects.
- If a gear is set to some other pixel order, the file bytes are pre-reordered
  and the colours will come out wrong on this firmware (which assumes RGB).
  There is no metadata byte to detect this, so **leave it on RGB**.

Also baked into the bytes at export time: the gear's **dimming** and any
**custom per-LED ordering**. The firmware needs no knowledge of these — it just
plays the bytes.

## Note

The workshop deliberately uses this simple `.LED` export so a single ESP32 + SD
module can play a show with only a few lines of parsing. The Studio also offers
a richer multi-channel show format for production installations, which this
workshop firmware does not need.
