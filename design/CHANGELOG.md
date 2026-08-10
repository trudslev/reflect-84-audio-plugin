# REFLECT-84 handoff — revision log

Check this file first to confirm which revision you are holding.

## v1.1 — conformance pass (current)

Panel **1340 × 645**, four body columns. Adds `GUI-SPEC.md`, which is the build
contract and supersedes `README.md` wherever they disagree.

- `GUI-SPEC.md` — **new file in this revision.** If your bundle has no
  `GUI-SPEC.md`, you are holding v1.0 and none of the items below reached you.
- Printed scales on every knob; standing value readouts removed.
- Damping knobs promoted 44px → 52px; the tiny variant is retired. Three sizes now
  (98 / 60 / 52).
- Damping tapers changed to log — `2 · 8^f` kHz and `40 · 12.5^f` Hz.
- Ticks moved onto labelled values; per-knob tick angle tables in § 7.
- Algorithm detent ticks specified by centre angle, ending the 0.55° offset.
- Contrast audited across every text role, measured against the darkest fascia
  point. New tokens in § 4.
- Corrected readout formats — `+0.0 dB`, `7.9 kHz`, ` %` suffixes.
- LCD takes live parameter readout; dropdown and User-Program flow follow TapeRot.
- Bank indicator moved into the LCD face with a divider rule.
- PROGRAM / IN / OUT captions aligned; LCD widened to 586 × 33.
- Scope split into **screen rectangle vs plot region** — reserved title strip and
  level gutter, leader ticks, trace clamped to the plot region. § 11.
- Bypass state added — 0.50 multiply, host-driven, no caption. § 10.
- `fonts/README.md` added (no binaries — sources and licence only).

**Corrections since first delivery (spec only, no design change):**

- § 4 — `#332b1e` restated 9.07:1 → **8.82:1**; retired `#4a4132` 6.53:1 → **6.34:1**.
  Both were transcription errors on our side; the build's measurements are correct
  and no classification changes. The measurement basis is now stated in § 4.
- § 13 — SIZE default restated in physical units, **0.71** (stored 0.64), matching
  every other row and the § 9 readout example.
- § 12 — factory bank is **twelve** Programs from the build, not the four
  illustrative names carried over from v1.0.

Prototype: `Reflect-84 v1.1.dc.html`.

## v1.0 — initial handoff

Panel 1200 × 530, three body columns. `README.md` only, no `GUI-SPEC.md`.
Prototype `Reflect-84.dc.html` — **superseded.** Still shipped alongside v1.1 as a
reference for the diff; do not build from it.
