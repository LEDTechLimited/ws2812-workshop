# WS2812 Workshop — Documentation Map & Convention

> Entry point + map. **Read this first.** Closed-bucket convention (arc42 + Diátaxis + ADR);
> skeletons + rationale: `xavier-detrouble/claude-code-project-template`.

Every doc lives in ONE bucket: `architecture/` · `decisions/` · `reference/` · `how-to/` ·
`runbooks/` · `archive/` (create on first use). State only in [`../STATUS.md`](../STATUS.md).
English-primary. Enforced by [`check_layout.sh`](check_layout.sh) + [`check_links.sh`](check_links.sh) in CI.

## Map
- **Reference:** [PINOUT.md](reference/PINOUT.md) (ESP32↔SD↔WS2812 wiring) · [LED_FILE_FORMAT.md](reference/LED_FILE_FORMAT.md) (`.LED` binary format).
- BOM / setup / workshop flow: [`../README.md`](../README.md).
