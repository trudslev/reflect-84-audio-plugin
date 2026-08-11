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
- PROGRAM / IN / OUT captions aligned; LCD widened to **641 × 34** (see the header
  items below — it grew again when the Program buttons were rebuilt).
- Disabled states struck from the spec entirely. § 6 and § 11 still described a
  disabled DELETE surface (`#232f49 → #1b2640`, `#4a5670` label at 2.04:1,
  `not-allowed`); the panel never drew it. A button with nothing to do now has one
  documented behaviour: both legends unlit at `#8090ae` (3.91:1) on the unchanged
  dark face, cursor `default`.
- **State matrix added** to § 11 — five panel states × four legends, plus the rules the
  lighting cannot imply: Esc out of naming leaves the Program edited (nothing was
  stored), Enter clears the flag, naming suppresses the menu and the other two legends,
  and a dark legend is never a disabled control. All four legends noted as runtime text,
  since baking them freezes one row of the table into the artwork. Lit contrast figure in
  the prose corrected to **11.91:1** — it still quoted the pre-`#fdf7e6` 10.78:1 while
  the table beside it had moved.
- Lit Program-button legends re-rendered as a genuine backlit bloom: ink `#f4ecd4`
  → `#fdf7e6` (11.91:1 on the light end of the face), single 7px shadow → a
  five-layer stack (1 / 4 / 9 / 18 / 30px). Unlit stays `#8090ae`, matte, no
  bloom — the two still differ in kind, not degree.
- Header captions brought to the 7:1 functional floor measured against the
  *lightest* point of the bezel gradient (`#22304c`), not its midpoint: PROGRAM
  `#c8b177` → `#d8c18a` (6.33:1 → 7.49:1), IN / OUT `#a9b6cd` → `#b7c2d8`
  (6.44:1 → 7.36:1). The meter captions now print their unit — `IN dB`, `OUT dB`
  — in proper case, the unit at normal tracking beside the tracked label.
  The `dB` unit widened the meter column by 4px, which the `flex: 1` LCD would
  otherwise have paid for; the Program row's gaps are set to the **8px** § 9
  already specifies (the panel still had 10px), returning it. **LCD cell stays 641 × 34, budget stays 44 characters, user-name
  cap stays 42** — measured, not inferred: value-cell inner width 515.45px at
  11.68px per character (Plex Mono 16px, .13em) holds 44 at 513.92px.
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

**Conformance to the BRAND.md update of 11 Aug 2026 (panel change):**

- § 9 — **Program buttons rebuilt with two stacked legends each**, SAVE/STORE and
  DELETE/CANCEL, **backlit**: the printed type glows when its function is live and
  there is no indicator lamp on or beside the buttons. The brass face is replaced
  by a dark `#26324d → #1a2438` face — pale brass gave lit type nowhere brighter
  to go — and that face is identical in every state. No disabled face anywhere.
  Legend contrast is stated worst-case along the gradient: lit 10.78:1, unlit
  3.91:1.
- § 9 — header band height fixed at **34px** for LCD, both buttons and both
  meters (was 33px, buttons stretched).
- § 9 — **LCD character budget stated: 44, user-name cap 42**, with the
  arithmetic, measured off the rendered panel rather than derived from the button
  widths. The budget grew by 55px-worth in this pass; the cap has not fallen.
- § 9 — bank tag reads `NAME` while a name is being typed; Factory display names
  stored upper-cased; dirty marker ` *` and SAVE's lit state read one flag.

**`BRAND.md` is no longer bundled** — read it from the repo. The suite document of
11 Aug 2026 is the governing revision for this pass; its spurious "pale fascia: a
small lamp sits beside each legend" bullet is void, since a legend lights by glowing
on every casting whatever its fascia.

**Per-state header renders added** — `screenshots/header/01`–`05`, the whole header row
at 3×, one image per row of the § 9 state matrix, named by state so they are diffable.
The crop is the full row (wordmark → OUT well) rather than the button pair, so the shared
**34 border-box** band and single baseline are checkable; on a wholly code-drawn panel
these are the only external check that exists.

**Naming flow built** — it was stubbed. SAVE on an edited Program now opens the inline
LCD entry: seed = current name less its factory index and any suffix plus the lowest free
one (`01 RAIN ALL DAY` → `RAIN ALL DAY 2`), upper-cased as typed, cap 42 enforced on
input, steady block caret `▌`, Enter commits, Esc cancels leaving the dirty flag set.
Spec gained § 9 *Name entry*; the caret is specified steady rather than blinking so the
state renders stay diffable.

**Border-box figures named beside every header content size** — § 9 now opens with a
part table (border-box, content, padding) and states that border-box is the figure to
hold constant. Measured off the panel: bezel 1308 × 104 (1262 × 76), LCD 641 × 34
(639 × 32), SAVE 62 × 34 (42 × 32), DELETE 70 × 34 (50 × 32), meter wells 64 × 34
(38 × 32). The wells' documented `padding 5px 12px` was stale — vertical padding is 0
with the height fixed at 34.

**Duplicate legend table removed.** § 9 carried a second four-row "Which legend is lit"
table with `—` cells alongside the five-row state matrix; the matrix is now the only one.

**`03-panel-lit.png` added** — an edited User Program, the only render showing a lit
legend. Prototype's unwired naming flow noted in `README.md`.

**Reference renders re-captured** for the rebuilt header: `01-panel.png` (default
state, all four legends unlit) and `02-panel-blank.png` (bare fascia). The
bypassed render is withdrawn — the multiply does not survive our capture path and
a wrong one is worse than none; § 10 specifies the state exactly.

Prototype: `Reflect-84 v1.1.dc.html`.

## v1.0 — initial handoff

Panel 1200 × 530, three body columns. `README.md` only, no `GUI-SPEC.md`.
Prototype `Reflect-84.dc.html` — **superseded.** Still shipped alongside v1.1 as a
reference for the diff; do not build from it.
