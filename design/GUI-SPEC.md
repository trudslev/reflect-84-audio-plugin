# REFLECT-84 — GUI Spec (v1.1, conformance pass)

Model **RF-84**, stereo reverb processor. This spec is the contract between the
design and the JUCE build. It supersedes the v1.0 notes in `README.md` wherever
the two disagree.

**Asset format: vector / code-drawn. No exported bitmaps, no filmstrips, no plate.**
The fascia is a gradient with a procedural texture; every heading, label, legend,
scale numeral and tick is drawn at runtime. Nothing in this document is blitted
artwork — all of it becomes drawing code. The one PNG in this bundle
(`screenshots/01-panel.png`) is a **reference render for comparison only** and must
not be used as a backing image.

Identity is unchanged and stays: cream/parchment fascia, navy header bezel,
engraved-plate wordmark, decay-tail scope, phosphor green as the single accent.
The product icon is **unchanged** — see `README.md` § Product icon; nothing in this
pass affects it.

---

## 1. Canvas and coordinate frame

- Panel **1340 × 645 px** at 100%. Fixed aspect 2.077:1, scales proportionally.
- Scaling range must reach at least **200%** as a genuine accessibility lever
  (BRAND.md § Canvas). 50–200% is the intended range.
- All coordinates below are **px from the panel's top-left corner at 100%**.
- Panel padding 16px. Radius 10px. Fill `linear-gradient(180deg, #efe6d0 0%, #e2d8bd 60%, #d8cdb0 100%)`.
- Edge treatment: `inset 0 1px 0 rgba(255,255,255,.75)`, `inset 0 -2px 6px rgba(90,70,40,.18)`, drop `0 24px 60px rgba(0,0,0,.55)`.
- Texture overlay (full bleed, non-interactive): 1px scanline `rgba(80,60,25,.035)` every 3px, plus `radial-gradient(120% 90% at 20% 0%, rgba(255,255,255,.5), transparent 60%)`.

### Regions

| Region | x | y | w | h |
|---|---|---|---|---|
| Header bezel | 16 | 16 | 1308 | 103 |
| Body row | 16 | 119 | 1308 | 511 |
| Col 1 — ALGORITHM + DAMPING | 18 | 137 | 300 | 489 |
| Divider 1 | 318 | 137 | 1 | 489 |
| Col 2 — REVERB TANK | 319 | 137 | 290 | 489 |
| Divider 2 | 609 | 137 | 1 | 489 |
| Col 3 — TANK LIVE + CHARACTER | 610 | 137 | 535 | 489 |
| Divider 3 | 1145 | 137 | 1 | 489 |
| Col 4 — OUTPUT | 1146 | 137 | 176 | 489 |

Vertical dividers: 1px, `linear-gradient(180deg, transparent, rgba(120,98,55,.4) 20%, rgba(120,98,55,.4) 80%, transparent)`, with `1px 0 0 rgba(255,255,255,.65)` to the right (engraved groove).
Horizontal divider in col 1 (y = 269, x 18→300): 1px `linear-gradient(90deg, rgba(120,98,55,.35), transparent)` + `0 1px 0 rgba(255,255,255,.6)` below.

---

## 2. Decisions taken in this pass

Three things the brief asked to be settled rather than left open.

**Damping knobs — promoted to the 52px small variant.** The 44px tiny variant is
**retired from this panel**; Reflect-84 now uses three knob sizes (98 / 60 / 52),
not four. Five 10px numerals will not clear a 29px tick radius, and shrinking the
type below the 10px floor was not an option. Promotion also let both damping
controls keep their full mark set (four for HF, five for LF) rather than dropping
to three, and the spacing was opened at the same time: the pair now sits on a
112px-wide cell each with a 16px gutter, in a column of its own.

**Algorithm tick alignment — fixed.** Ticks are now specified by their **centre
angle**, which coincides exactly with the pointer detent angle. The old
`from 224.45°` figure was the *leading edge* of a 1.1°-wide wedge, and reading it
as a centre is what produced the 0.55° counter-clockwise error. See § 6.

**Bypass — added.** Reflect-84 previously had no disengaged state. It now has one,
per BRAND.md § Bypass. Details in § 10.

---

## 3. Type roles

Two families. **Jost 500** for the wordmark only; **IBM Plex Mono 400/500**
everywhere else. Nothing functional below 10px. Hierarchy is size and weight only —
no opacity is used to push text down the hierarchy anywhere on the panel.

| Role | Family / weight | Size | Tracking | Colour |
|---|---|---|---|---|
| Wordmark | Jost 500 | 42px | .10em | `#f0e2ba` |
| Function + model line | Plex Mono 400 | 10px | .30em | `#a9b6cd` |
| PROGRAM caption | Plex Mono 400 | 10px | .30em | `#d8c18a` |
| Section pill (REVERB TANK / DAMPING / CHARACTER / OUTPUT) | Plex Mono 400 | 10px | .24em | `#d8c18a` |
| Control label | Plex Mono 400 | 10px | .16–.18em | `#3e3527` |
| Control label — CHARACTER (large knobs) | Plex Mono 400 | 11px | .20em | `#332b1e` |
| Printed scale numeral | Plex Mono 400 | 10px | .02em | `#3e3527` |
| Scale unit | Plex Mono 400 | 10px | .14em | `#3e3527` |
| ALGORITHM caption | Plex Mono 400 | 10px | .26em | `#3e3527` |
| Algorithm corner label — selected | Plex Mono **500** | 11px | .16em | `#332b1e` |
| Algorithm corner label — unselected | Plex Mono 400 | 11px | .16em | `#5e5440` |
| TANK LIVE label | Plex Mono 400 | 11px | .26em | `#332b1e` |
| Scope header data (DECAY TAIL / RT60 / 200 ms / DIV) | Plex Mono 400 | 10px | .18em | `#3e3527` |
| Scope on-screen legend | Plex Mono 400 | 10px | .18em | `#8ea0bc` |
| LCD program name / live readout | Plex Mono 400 | 16px | .13em | `#f2e6c2` |
| LCD bank indicator (FACT / USER) | Plex Mono 400 | 16px | .13em | `#f2e6c2` |
| IN / OUT meter value | Plex Mono 400 | 16px | 0 | `#e8dcba` |
| IN / OUT meter caption | Plex Mono 400 | 10px | .28em | `#b7c2d8` |
| IN / OUT caption unit (`dB`) | Plex Mono 400 | 10px | .02em | `#b7c2d8` |
| SAVE / DELETE button | Plex Mono 400 | 10px | .20em | see § 4 |
| Version stamp | Plex Mono 400 | 10px | .10em | `#5e5440` |

If Jost is unavailable in the build, substitute a geometric grotesque with a tall
x-height — never a neutral system sans.

---

## 4. Palette and measured contrast

Ratios are WCAG relative-luminance contrast, computed **against the darkest point
of the surface the text sits on** — for fascia text that is `#d8cdb0`, the bottom
of the gradient, not the `#efe6d0` top. Functional text targets 7:1; flavour text
4.5:1.

### On fascia (worst case `#d8cdb0`)

| Hex | Ratio | Class | Used for |
|---|---|---|---|
| `#332b1e` | **8.82:1** | functional | TANK LIVE, CHARACTER labels, selected algorithm label |
| `#3e3527` | **7.62:1** | functional | control labels, printed scales, units, ALGORITHM caption, scope header data |
| `#5e5440` | **4.71:1** | flavour | unselected algorithm labels, version stamp |
| `#7a6b4c` | — | graphic | tick marks (not text; no floor applies) |

Ratios are WCAG relative luminance against the darkest fascia point `#d8cdb0`,
sRGB linearised with the 0.03928 / 12.92 threshold. Re-measure rather than
transcribe; if a value here disagrees with yours, yours wins and tell us.

The previous `#4a4132` (6.34:1) and `#5c5241` failed the functional floor and are
retired. The previous unselected-algorithm `#9a8e74` measured **2.10:1** — it was
the worst offender on the panel and is replaced by `#5e5440`. Unselected labels
stay dimmer than selected on purpose (redundant encoding alongside the pointer, the
same convention as Gatecrasher's unselected switch labels), but they now clear the
flavour floor with room to spare, and the selected/unselected distinction is
carried by **weight (500 vs 400)** as well as value.

### On the navy bezel (worst case `#142036`)

| Hex | Ratio | Used for |
|---|---|---|
| `#f0e2ba` | 12.65:1 | wordmark |
| `#a9b6cd` | 7.96:1 | function + model line |
| `#b7c2d8` | 8.90:1 | meter captions, incl. the `dB` unit |
| `#d8c18a` | 9.05:1 | PROGRAM caption |

The header captions are additionally measured against the **lightest** point of
the bezel gradient, `#22304c`, since that is where they sit: `#b7c2d8` 7.36:1,
`#d8c18a` 7.49:1. Both clear 7:1 at the worst case rather than only at the
midpoint — the earlier `#a9b6cd` / `#c8b177` pair measured 6.44:1 and 6.33:1
there and were replaced. The function + model line still carries `#a9b6cd`
(6.44:1 against the same point) and is outstanding.

### On the section pill (`#16223a`)

| Hex | Ratio | Used for |
|---|---|---|
| `#d8c18a` | 9.30:1 | pill text |

### On LCD glass (`#060a11`)

| Hex | Ratio | Used for |
|---|---|---|
| `#f2e6c2` | 15.93:1 | program name, live readout, bank badge |
| `#e8dcba` | 14.52:1 | IN / OUT meter values |

### On scope glass (`#050810`)

| Hex | Ratio | Used for |
|---|---|---|
| `#8ea0bc` | 7.54:1 | DCY ENV, 0 dB, −60 dB, grain state |
| `#5ce07a` (accent) | 11.82:1 | decay trace |

The scope legends were `rgba(190,205,225,.4–.5)` — opacity-driven hierarchy, which
BRAND.md forbids. They are now solid `#8ea0bc` at a single value, with hierarchy
carried by position alone.

### Buttons

| Element | Text | Surface | Ratio |
|---|---|---|---|
| Legend, lit | `#fdf7e6` | button `#26324d` (light end) | 11.91:1 |
| Legend, unlit | `#8090ae` | button `#26324d` (light end) | 3.91:1 |

The Program buttons are no longer brass — see § 9. Both legends sit on one dark
face in every state; lit and unlit are the same weight and differ only in
brightness. The retired brass values were SAVE `#14192a` at 7.58:1 and, before
that, `#2a3550` at 5.28:1, which failed. Both legend ratios are quoted against the
light end of the face, since the legend crosses the gradient.

**There is no disabled state to quote a figure for.** Earlier revisions gave
DELETE a disabled surface (`#232f49 → #1b2640`) with a `#4a5670` label at
**2.04:1**; the surface and the state are both retired. A button with nothing to
do shows both legends unlit at `#8090ae` — **3.91:1**, above the 3:1 state
floor — on the same dark face it always wears. Unlit is a lighting state, not a
disabled control, so it is held above the state floor rather than exempted from
the functional one.

### Surfaces

| Token | Value |
|---|---|
| Fascia | `#efe6d0` → `#e2d8bd` (60%) → `#d8cdb0` |
| Bezel / pill | `#22304c` → `#1a2740` (55%) → `#142036`; pill `#22304c` → `#16223a` |
| LCD well | `#0a0f18` → `#060a11`; hover `#0e1522` → `#080d16` |
| Scope glass | `#080d16` → `#050810`; bezel `#c9bd9c` → `#b8aa87` |
| Scope grid | `rgba(120,160,200,.10)`, plot region only — 200 ms vertical, quarter-scale horizontal |
| Scope reservation hairline | `rgba(120,160,200,.20)` |
| Scope leader tick | `rgba(142,160,188,.85)` |
| Engraved line | `rgba(120,98,55,.35–.4)` over `rgba(255,255,255,.6–.75)` |

### Accent

**One accent: `#5ce07a`** (phosphor green). Used only for the TANK LIVE LED, its
glow, and the decay trace + its 16% fill. It appears nowhere else — not on knobs,
labels, buttons or dividers. Alternates reserved but unused: `#ffb02e`, `#4ec9ff`,
`#ff5c5c`.

---

## 5. Knob geometry

Three variants. Per BRAND.md § Stating coordinates, **the dial centre given below
is the centre of the printed tick arc — the point the needle pivots about.** The
control label sits *below* the arc, so the bounding box of the whole control cell
is not centred on the dial; do not measure the pivot off the cell.

| Variant | Body ⌀ | Tick arc radius (r) | Tick length | Numeral radius (R) | Cell (w × h) | Pointer |
|---|---|---|---|---|---|---|
| Large | 98px | 62px | 8px inward | 80px | 168 × 180 | 3px × 30% of body, `#2f2617` |
| Medium | 60px | 39px | 6px inward | 55px | 118 × 132 | 2px × 38% of body, `#33291a` |
| Small | 52px | 35px | 6px inward | 50px | 112 × 124 | 2px × 37% of body, `#33291a` |

- **Sweep angle: 270°**, from **−135°** (minimum) to **+135°** (maximum), 0° = pointer straight up. Pointer rotation = `−135 + f × 270` degrees, where `f` is the parameter's rotation fraction.
- Tick marks: 2px wide, colour `#7a6b4c`, drawn from radius `r` inward by the tick length, centred on the tick's angle.
- Scale numerals: centred on the point at radius `R` and the tick's angle, then centred on their own bounding box (`translate(-50%, -50%)`).
- Unit string: centred horizontally on the dial centre, at `cy + 44px` (small), `cy + 52px` (medium), `cy + 74px` (large) — inside the 90° gap at the bottom of the arc, between the two end numerals. Units print here and **never** as a suffix on the control name.
- Control label: centred below the cell.
- Body fills:
  - Large `radial-gradient(circle at 50% 22%, #fdf6e0, #ddcb98 52%, #b09a61 76%, #7d6a3b)` + inner cap `inset:15px` `radial-gradient(circle at 50% 28%, rgba(255,255,255,.5), rgba(120,98,55,.18))`
  - Medium `radial-gradient(circle at 50% 24%, #fbf3da, #d6c391 55%, #a58f58 80%, #7a6738)`
  - Small `radial-gradient(circle at 50% 24%, #f9f1d8, #d4c18e 58%, #9f8a55 82%, #77653c)`
- Shadows:
  - Large `0 6px 14px rgba(45,33,12,.45), inset 0 2px 2px rgba(255,255,255,.75), inset 0 -8px 14px rgba(90,70,30,.35)`
  - Medium `0 4px 9px rgba(45,33,12,.4), inset 0 1px 1px rgba(255,255,255,.7), inset 0 -5px 10px rgba(90,70,30,.35)`
  - Small `0 3px 8px rgba(45,33,12,.38), inset 0 1px 1px rgba(255,255,255,.68)`

### Dial centres (panel coordinates, 100%)

| Control | Variant | cx | cy |
|---|---|---|---|
| ALGORITHM | rotary switch, 104px | 158.0 | 275.6 |
| DAMPING HF | small | 94.0 | 475.6 |
| DAMPING LF | small | 222.0 | 475.6 |
| SIZE | medium | 398.0 | 310.6 |
| DECAY | medium | 530.0 | 310.6 |
| PRE-DELAY | medium | 398.0 | 461.1 |
| DENSITY | medium | 530.0 | 461.1 |
| MODULATION | large | 773.5 | 480.1 |
| DIGITAL GRAIN | large | 981.5 | 480.1 |
| STEREO WIDTH | small | 1244.0 | 225.1 |
| MIX | small | 1244.0 | 373.6 |
| OUTPUT TRIM | small | 1244.0 | 522.1 |

---

## 6. ALGORITHM rotary switch

- Body 104px, `radial-gradient(circle at 50% 26%, #1f2b44, #16223a 52%, #0d1526 80%, #070c15)`, shadows `0 5px 12px rgba(45,33,12,.45), inset 0 1px 1px rgba(255,255,255,.16), inset 0 -6px 12px rgba(0,0,0,.6)`, inner disc `inset:12px`. Dark on purpose — it is the one system control among brass knobs.
- Pointer 3px wide × 36% of radius, `linear-gradient(#f4e8c4, #cdb989)`, from 9px inside the top edge.
- **4 detents, no continuous travel.** Pointer angle = `−45 + index × 90` degrees.
- **Ticks: 2px × 9px, radius 65px, colour `#7a6b4c`, each centred exactly on its detent angle — −45°, +45°, +135°, +225°.** The tick's *centre* is the contract, not its leading edge. (The v1.0 artwork specified `from 224.45°`, which is the leading edge of a 1.1°-wide wedge; the centre is 225.0°. Reading the leading edge as a centre drew every tick 0.55° counter-clockwise of the detent it marks.)

### Corner labels

Positioned outside the 104px body at the four diagonals, 11px, .16em, clickable.

| Label | Index | Corner | Pointer angle |
|---|---|---|---|
| PLATE | 0 | top-left | −45° |
| DIGITAL ROOM (two lines) | 1 | top-right | +45° |
| CHAMBER | 2 | bottom-right | +135° |
| HALL | 3 | bottom-left | +225° |

**The corner ordering is deliberate and is not clockwise-sequential in index order.**
It is arranged so the two digital algorithms sit on the right and the two physical
ones on the left. Do not "correct" it. Map indices to the DSP enum as listed.

Caption `ALGORITHM` centred 14px below the body.

---

## 7. Printed scales

Standing numeric readouts under each knob are **removed**. The printed scale is now
the only at-rest value reference, which makes it functional text: 10px, `#3e3527`,
7.62:1.

`f` is the rotation fraction (0 at −135°, 1 at +135°). Tick angle = `−135 + f × 270`.
Every tick sits at a labelled value; there are no minor ticks.

### Linear controls

| Knob | Variant | Marks | Unit | f | Tick angles |
|---|---|---|---|---|---|
| SIZE | medium | 0.2 / 0.4 / 0.6 / 0.8 / 1.0 | *(none)* | 0, .25, .5, .75, 1 | −135.00, −67.50, 0.00, +67.50, +135.00 |
| DECAY | medium | 0.4 / 2 / 4 / 6 / 8 | s | 0, .2105, .4737, .7368, 1 | −135.00, −78.17, −7.10, +63.94, +135.00 |
| PRE-DELAY | medium | 0 / 45 / 90 / 135 / 180 | ms | 0, .25, .5, .75, 1 | −135.00, −67.50, 0.00, +67.50, +135.00 |
| DENSITY | medium | 0 / 25 / 50 / 75 / 100 | % | 0, .25, .5, .75, 1 | −135.00, −67.50, 0.00, +67.50, +135.00 |
| MODULATION | large | 0 / 25 / 50 / 75 / 100 | % | 0, .25, .5, .75, 1 | −135.00, −67.50, 0.00, +67.50, +135.00 |
| DIGITAL GRAIN | large | 0 / 25 / 50 / 75 / 100 | *(none)* | 0, .25, .5, .75, 1 | −135.00, −67.50, 0.00, +67.50, +135.00 |
| STEREO WIDTH | small | 0 / 50 / 100 / 150 / 200 | % | 0, .25, .5, .75, 1 | −135.00, −67.50, 0.00, +67.50, +135.00 |
| MIX | small | 0 / 25 / 50 / 75 / 100 | % | 0, .25, .5, .75, 1 | −135.00, −67.50, 0.00, +67.50, +135.00 |
| OUTPUT TRIM | small | −12 / −6 / 0 / +6 / +12 | dB | 0, .25, .5, .75, 1 | −135.00, −67.50, 0.00, +67.50, +135.00 |

DECAY is linear in seconds over 0.4–8.0 s, so its marks at 0.4 / 2 / 4 / 6 / 8 are
**not** evenly spaced. That is correct — the numbers are round, the spacing is not.

SIZE and DIGITAL GRAIN print bare numbers with no unit, by design.

### Logarithmic controls — taper change

The two damping controls are currently **linear in Hz** and are being changed to a
**log taper** as part of this work. Linear put DAMPING HF's midpoint at 9 kHz, so
half the travel covered 9–16 kHz where the audible difference is slight. Log makes
the marks octaves and lands them evenly.

| Knob | Variant | Range | Curve | Marks | Unit | f | Tick angles |
|---|---|---|---|---|---|---|---|
| DAMPING HF | small | 2 – 16 kHz | `2 · 8^f` kHz (3 octaves, exact log) | 2 / 4 / 8 / 16 | kHz | 0, .3333, .6667, 1 | −135.00, −45.00, +45.00, +135.00 |
| DAMPING LF | small | 40 – 500 Hz | `40 · 12.5^f` Hz (exact log) | 40 / 80 / 160 / 320 / 500 | Hz | 0, .2744, .5489, .8233, 1 | −135.00, −60.91, +13.20, +87.29, +135.00 |

Rotation fraction for a value `v`: `f = log(v / min) / log(max / min)`.
DAMPING HF's four marks come out evenly spaced because they are exact octaves in an
exact-octave range. DAMPING LF's do **not** — 500 Hz is not an octave above 320 Hz,
so the last interval is short. That is correct and must not be evened out.

**The build's taper and the printed marks must agree exactly.** If the DSP ships
anything other than the curves above, the artwork is wrong, not the scale.

---

## 8. Readout formats

Corrected per the brief. These are the strings the LCD shows during manipulation
(§ 9); they are the only place a live numeric value appears on the panel.

| Parameter | Format | Example |
|---|---|---|
| SIZE | 2 dp, **no unit** | `0.71` |
| DECAY | 1 dp + ` s` | `4.8 s` |
| PRE-DELAY | integer + ` ms` | `40 ms` |
| DENSITY | integer + ` %` | `72 %` |
| DAMPING HF | 1 dp + ` kHz` | `7.9 kHz` |
| DAMPING LF | integer + ` Hz` | `169 Hz` |
| MODULATION | integer + ` %` | `34 %` |
| DIGITAL GRAIN | integer, **no unit** | `46` |
| STEREO WIDTH | integer + ` %` | `140 %` |
| MIX | integer + ` %` | `55 %` |
| OUTPUT TRIM | **explicit sign** + 1 dp + ` dB` | `+0.0 dB`, `−6.0 dB` |
| ALGORITHM | name | `PLATE` |

Fixed in this pass: OUTPUT TRIM printed `0.0` with no sign and no unit — it now
prints `+0.0 dB` and carries an explicit `+` or `−` at every value including zero.
DAMPING HF printed the wrong abbreviation, `7.9 k` — it now prints `7.9 kHz`.
DENSITY, MODULATION, MIX and STEREO WIDTH printed bare percentages and now carry
` %`. SIZE and DIGITAL GRAIN stay bare by design.

Minus sign is U+2212 (−), not a hyphen, in both scales and readouts.

---

## 9. Header, PROGRAM LCD and dropdown

Header bezel: `#22304c → #1a2740 (55%) → #142036`, 1px `rgba(0,0,0,.5)`, radius 6,
`inset 0 1px 0 rgba(255,255,255,.12), inset 0 -3px 8px rgba(0,0,0,.45), 0 2px 4px rgba(60,45,20,.35)`.
Padding 13px 22px, three columns gap 16px (wordmark block carries an extra 10px right
margin, so wordmark↔PROGRAM reads 26px): wordmark block (min 292px) · PROGRAM
block (flex) · IN/OUT meters.

All three column captions — `PROGRAM`, `IN`, `OUT` — sit on one line at **y 41**,
and the LCD cell, both Program buttons and both meter wells share a single band at
**y 61** whose height is **34 border-box** — the suite-wide header part height, a
fixed figure rather than a proportion of this panel. **Every size in this section is
given border-box first, content in brackets**, because border-box is the figure the
build holds constant; each part is `box-sizing: border-box` with a 1px border, so
content height is 32 throughout.

| Part | Border-box | Content | Padding |
|---|---|---|---|
| Header bezel | 1308 × 104 | 1262 × 76 | 13px 22px |
| PROGRAM LCD cell | **641 × 34** | 639 × 32 | 0 (inner faces pad themselves) |
| SAVE button | **62 × 34** | 42 × 32 | 0 9px |
| DELETE button | **70 × 34** | 50 × 32 | 0 9px |
| IN well · OUT well | **64 × 34** | 38 × 32 | 0 12px |

The two buttons differ in width because each is sized by its longest legend
(`STORE` 5, `DELETE` 6) — that is intended, and only the **34 border-box** height is
shared. If a content size ever changes, the 34 is the number that stays put: take
the difference out of padding. Two 10px legends with leading and
padding need ~27px, which is what sets it. The LCD
fills the space up to the meters; the gap between DELETE and the IN well is **16px**,
deliberately wider than the **10px** between the IN and OUT wells, so the meters read
as their own pair.

Nameplate metaphor: **engraved plate**. Wordmark `#f0e2ba`, text-shadow
`0 1px 0 rgba(0,0,0,.65), 0 -1px 0 rgba(255,255,255,.18), 0 2px 10px rgba(0,0,0,.4)`.

### LCD cell

At **x 357, y 61, 641 × 34 border-box** (639 × 32 content, 1px border). Well `#0a0f18 → #060a11`, 1px `rgba(0,0,0,.7)`, radius 3,
`inset 0 2px 7px rgba(0,0,0,.8), 0 1px 0 rgba(255,255,255,.12)`. Hover lightens to
`#0e1522 → #080d16`. Whole cell is the click target.

- **Bank indicator** at the left of the LCD face: `FACT`, `USER`, or `NAME` **while a name is being typed** — the Program is not in the user bank until the name is committed, so `USER` there would claim a thing that does not exist yet. Set in the *same* face as the program name, set in the *same* face as the program name — 16px, .13em, `#f2e6c2`, same phosphor glow — with 16px padding either side. It is printed text on the glass, not a badge: no border, no fill, no radius. Separated from the program name by a **1px vertical rule** in `rgba(242,230,194,.35)`, inset 7px from the top and bottom of the well. A single indicator built into the LCD — there is no separate bank control.
- **Program name** centred, 16px .13em `#f2e6c2`, glow `0 0 9px rgba(242,230,194,.3)`.
- **Chevron** at x = cell right − 12: a 9 × 9 box with 1.6px right and bottom borders in `#d8c18a`, rotated 45°, vertically centred with a −6px optical offset. Drawn form, not a glyph.

**Capacity: the cell holds 36 characters at this size against a longest readout of
19 (`DIGITAL GRAIN: 100`). No widening is needed.**

### Character budget

At 16px Plex Mono with .13em tracking a cell character advances **11.68px**. The
name area is the 641px cell less the bank cell (4 chars + 16px either side = 79px),
the 1px divider, and its own 12px / 32px padding — **515.45px, or 44 characters**
(44 × 11.68 = 513.92; a 45th would need 525.6).

**User-name cap: 42** — the budget less the two characters of the dirty marker
` *`. User Programs carry no index, so nothing else comes off it.

This *grew* by 55px in this pass. The second legend costs no width (the buttons
were already sized by their longest word — measured 60px for SAVE/STORE and 68px
for DELETE/CANCEL), the lamps are gone, and the row gaps either side went 10px →
8px; the cell took all of it. Figures here are measured off the rendered panel at
1340 wide, not derived from the button arithmetic. **A budget may grow; the cap may never shrink** — if a future change to
header height, font size, tracking or cell width would reduce it, take the room
from padding or widen the cell. Never lower the cap: names already saved would be
orphaned.

**Factory display names are stored upper-cased.** Case has exactly one source;
applying it at the LCD would leave the dropdown and the host's own Program menu
reading the label raw, and the same Program would read two ways on one panel.

### Name entry

Built in the prototype as of this pass; it is no longer a stub.

- **Seed** — the current program name less its factory index and any existing numeric
  suffix, plus the lowest free suffix: `01 RAIN ALL DAY` → `RAIN ALL DAY 2`. Stripping
  the index matters because User Programs carry none (§ Character budget).
- **Field** — the program name's own centred slot, same 16px .13em `#f2e6c2` and glow.
  It does not become a left-aligned input: the cell's geometry is identical typing or
  not, so nothing in the band shifts when naming opens.
- **Caret** — a solid block `▌` in `#f2e6c2` at the end of the draft, **steady, not
  blinking**. A blink would make the five state renders undiffable and buys nothing on
  a field that is only ever open while the user is typing into it.
- **Cap 42**, enforced on input, not on commit — the same figure as § Character budget,
  since a typed name is a name.
- Input is upper-cased as typed; Enter commits, Esc cancels, Backspace deletes.
  Committing on an empty field falls back to the seed rather than storing a blank name.

### Live parameter readout

While a control is being moved the LCD replaces the program name with
`<PARAMETER NAME>: <value>` in the LCD face — e.g. `DAMPING HF: 8.0 kHz`,
`DECAY: 4.8 s`, `ALGORITHM: PLATE`. It reverts to the program name **900 ms after
release**. Parameter names in the readout are the printed control names in full
(`PRE-DELAY`, `STEREO WIDTH`, `OUTPUT TRIM`, `DAMPING HF`, `DAMPING LF`).

**Only direct user manipulation triggers it.** Host automation must not drive the
readout. No tooltips, no floating value bubbles — the display already on the panel
does this job.

### Dropdown — follows TapeRot

Structure and behaviour identical to TapeRot's; only the palette is Reflect-84's.

- Opens on click anywhere in the LCD cell, anchored flush to the cell's left and right edges, 4px below it. Max height 260px, scrolls beyond that.
- Menu surface `#0d1420 → #080d16`, 1px `rgba(0,0,0,.75)`, radius 3, `0 10px 28px rgba(0,0,0,.6)` + `inset 0 1px 0 rgba(255,255,255,.08)`, 4px vertical padding.
- **Two groups, always both present, Factory first.** Group headers `FACTORY` and `USER`: 10px, .26em, `#c8b177`, padding 9px 12px 4px, not selectable.
- Items: 13px, .10em, `#f2e6c2`, padding 6px 12px, hover `rgba(120,160,200,.10)`. A 12px gutter at the left holds a `✓` on the currently loaded Program.
- When the User bank is empty the USER group shows a single non-selectable row `— none saved —` in 12px `#8ea0bc`. The group header is never hidden.
- Selecting an item loads it, sets the bank badge, and closes the menu.

Factory bank: **twelve Programs, names from the build** — the bank is authored
there, not here. The four listed in v1.0 (`01 RAIN ALL DAY`, `02 SO LONG`,
`03 COLD ATMOSPHERE`, `04 WORLD GONE MAD`) were illustrative and the prototype
still shows them; treat the build's twelve as authoritative and ignore the
prototype's list. Menu geometry above is unaffected — twelve items plus two group
headers exceed the 260px max height, so the menu scrolls, which is specified.

### SAVE / DELETE — naming flow, follows TapeRot

- **SAVE always creates a new named User Program and never overwrites**, even when a User Program is loaded. There is therefore no separate "New Program" control. SAVE is never disabled — on an unmodified Program its legends simply sit unlit.
- SAVE opens an inline name entry **inside the LCD cell**: the program name is replaced by an editable field in the LCD face, cursor at the end, seeded with the current name plus a numeric suffix. Enter commits; Esc cancels and restores the previous display. The naming happens in the display, not in a modal dialog — there is no hardware equivalent of a modal.
- On commit the new Program is appended to the User bank, becomes the loaded Program, and the badge switches to `USER`.
- **DELETE works only on User Programs.** On a Factory Program or INIT it is not disabled and its face does not change: **both legends step back to unlit** `#8090ae`, which reads as "nothing to do here". Cursor `default`, tooltip *"Enabled only on a User Program"*. Clicks are ignored.
- Both buttons: legends 10px .20em, radius 3, padding 0 9px, height 34px, dark face `#26324d → #1a2438` in **every** state — full detail below. Nothing on either button is ever greyed, recessed or drawn inert; only the legends' illumination changes.

### Two legends per button, backlit

**Each Program button carries two legends, stacked, and never changes its face.**
SAVE above STORE, DELETE above CANCEL — resting function on top, what the button
becomes during naming beneath it. 10px, 12px line height, 1px between, weight 400
in every state, both **centred horizontally** in the button, with `text-indent: .20em` to cancel the
trailing tracking on the last character so they centre optically rather than
metrically.

**The legend itself is the lamp.** There is no indicator light anywhere on or
beside these buttons: the printed type glows when its function is live, exactly as
backlit legends on rack gear do. Nothing else on the button changes.

| State | Ink | Glow |
|---|---|---|
| Lit | `#fdf7e6` | `0 0 1px rgba(255,250,236,.95)`, `0 0 4px rgba(248,238,208,.8)`, `0 0 9px rgba(244,232,196,.6)`, `0 0 18px rgba(240,226,184,.35)`, `0 0 30px rgba(236,220,172,.2)` (five layers, one text-shadow) |
| Unlit | `#8090ae` | none |

The bloom is layered rather than a single radius: a 1px hot core keeps the
letterforms crisp, 4px and 9px carry the visible glow, and 18px / 30px give the
faint wash a backlit legend throws onto the face around it. A single mid-radius
shadow reads as a soft edge, not as a light behind the type.

The face is a gradient and a legend sits across it, so both ratios below are the
**worst case along it** — the light end, `#26324d`.

**The button face is dark so that a glowing legend has somewhere brighter to go**:
`#26324d → #1a2438`, 1px `rgba(0,0,0,.55)`, radius 3, `inset 0 1px 0 rgba(255,255,255,.10), 0 1px 2px rgba(0,0,0,.45)`,
padding 0 9px, height 34px. **This replaces the brass face**, which was pale enough
that lit type could not read as illuminated. The face is now identical in every
state — including the state that used to be "disabled".

Contrast on that face: lit **11.91:1** worst case (14.54:1 at the dark end), unlit
**3.91:1** worst case (4.77:1 at the dark end) — both clear of the 3:1 state floor,
and far enough apart to be read without comparing one legend to the
other. **Lit is a neutral bright, never the accent**; `#5ce07a` stays on the TANK
LED and the trace.

### State matrix — the four legends

Five panel states, four legends, twenty decisions. This table is the contract; the
prose above describes the mechanism, but the mechanism does not imply every cell.

| Panel state | SAVE | STORE | DELETE | CANCEL |
|---|---|---|---|---|
| Factory Program, unmodified | dark | dark | dark | dark |
| Factory Program, edited | **lit** | dark | dark | dark |
| User Program, unmodified | dark | dark | **lit** | dark |
| User Program, edited | **lit** | dark | **lit** | dark |
| Naming a Program | dark | **lit** | dark | **lit** |

"Edited" means the dirty flag — any parameter moved since the Program was loaded. It
drives SAVE's lighting and the ` *` marker in the LCD from **one** flag; they can
never disagree. INIT counts as a Factory Program for both rows above.

Behavioural rules the lighting does not imply:

- **Esc out of naming leaves the Program edited, because nothing was stored.** The
  panel returns to the row it came from — *Factory edited* or *User edited* — with
  SAVE lit again. The dirty flag is untouched by cancelling.
- **Enter commits and clears the flag.** The new Program is the loaded one, badge
  `USER`, so the panel lands on *User Program, unmodified*: SAVE goes dark, DELETE
  lights.
- **Naming suppresses the rest of the header.** The LCD is an entry field, so clicking
  it does not open the Program menu, and SAVE and DELETE are dark and inert even where
  they would otherwise be live. Only STORE and CANCEL respond.
- **A dark legend is never a disabled control.** Every one of the twenty cells above is
  drawn on the same face at the same size; only illumination differs. Clicks on a dark
  legend are ignored silently — no flash, no shake, no tooltip beyond DELETE's.

**All four legends are drawn live as text, never baked into the fascia bitmap** —
baking them would freeze one row of this table into the artwork.

Both dark reads as "nothing to do here", never as a blank button — see the state
matrix above, which is the single table for this; the earlier four-row version that
stood here had `—` cells and has been removed rather than kept in parallel.

**No disabled face anywhere.** The previous brass-with-`#232f49`-disabled-DELETE
pair is gone; the button is never greyed, dimmed, recessed or drawn inert, and
there is no "off" sprite. Cursor is `default` when a button has nothing to do,
`pointer` otherwise — it is not disabled, its legend is unlit.

**Ink weight never stands in for illumination.** Weight is 400 throughout; only
brightness and glow change, which is what a lamp does.

Stacked rather than side by side because it costs no width — the button is already
sized by its longest word, and `DELETE`/`CANCEL` are both six characters.

**One face per button; only the legends change.** If the build bakes these, that is
one face per button plus lit and unlit legend text drawn at runtime — not a sprite
per state.

### IN / OUT meters

Right of the header, gap 10px, vertically centred so the captions align with
`PROGRAM`. Caption 10px .28em `#b7c2d8`, followed by the unit `dB` at .02em in
the same size and colour (baseline-aligned, 4px gap; proper case, never `DB`),
above a well
**64 × 34 border-box** (38 × 32 content) — `#0a0f18 → #060a11`, padding 0 12px with
the height fixed at 34 rather than derived from vertical padding, min-width 58,
radius 3,
`inset 0 2px 6px rgba(0,0,0,.75)`), value 16px `#e8dcba`, centred.
**Live peak dB, drawn at runtime — never baked.** The reference render shows
`-3.2` / `-0.8` as sample values only.

---

## 10. Bypass / disengaged state

**A disengaged state is being added.** It did not exist in v1.0.

- Driven by the **host's bypass**; there is no on-panel bypass control and none is being added.
- **Lighting change only**: a full-bleed **multiply** of `#808080` over the entire panel — 0.50, matching Chorus-60. Not an alpha blend toward the fascia colour, which would read as fog rather than darkness.
- The multiply sits above every panel layer including the texture overlay, and below nothing.
- **Pointers do not move.** Knob bodies are not redrawn, dimmed individually, desaturated, blurred or flattened. The accent is not drained — the LED and trace darken with everything else, by the same multiply.
- **No caption.** Nothing prints "bypassed", "settings retained" or similar.
- The 7:1 / 4.5:1 legibility floors **do not apply in this state**. The panel is deliberately not operable; conveying *not usable* is the job.

---

## 11. TANK LIVE scope

The plugin's one signature live display and its only LED.

- Header row at y 136.6, full column width. LED 15px at x 632: `radial-gradient(circle at 40% 32%, <accent lightened 85%>, <accent> 70%, #2a3a24)`, glow `0 0 14px 3px <accent at 45%>`, `inset 0 1px 1px rgba(255,255,255,.5)`. Label `TANK LIVE`. Right side, gap 18px: `DECAY TAIL` · `RT60 <decay> s` · `200 ms / DIV`.
- Scope bezel at **x 632, y 163.6, 491 × 182**: padding 6px, `#c9bd9c → #b8aa87`, radius 4, `inset 0 2px 4px rgba(70,54,25,.4), 0 1px 0 rgba(255,255,255,.6)`.
- **Screen rectangle** — content box **477 × 168 at x 639, y 170.6** (1px border outside that), `#080d16 → #050810`, 1px `rgba(0,0,0,.7)`, radius 2, `inset 0 3px 12px rgba(0,0,0,.85)`.

### Screen rectangle vs plot region — read this before drawing the trace

**The screen rectangle and the plot region are two different rectangles, and the
trace is clamped to the plot region, not to the screen.** Coordinates below are in
the drawing space `600 × 168`, which maps onto the 477 × 168 screen content box with
`preserveAspectRatio: none` — **x scales 477/600 = 0.795**, y scales 1.0.

In panel coordinates that puts the gutter's left edge at **x 1052.4**
(639 + 520 × 0.795) and the leader ticks at **x 1052.4 → 1061.9**, with the `0 dB` /
`−60 dB` labels running from x 1062. Deriving these from any screen width other than
477 misplaces the plot/gutter split and pulls the ticks off the labels they point at.

| Rectangle | x | y | w | h |
|---|---|---|---|---|
| Screen | 0 | 0 | 600 | 168 |
| Title strip (reserved) | 0 | 0 | 600 | 20 |
| **Plot region** | 0 | 20 | **520** | 148 |
| Level gutter (reserved) | 520 | 20 | 80 | 148 |

Within the plot region: **0 dB is y = 26, −60 dB (the baseline) is y = 156**, so
full-scale height is 130.

The trace uses the **full vertical extent** of the plot region — it touches y = 26 at
peak and rests on y = 156 — and is clipped horizontally at **x = 520**. It never
enters the gutter or the title strip. Time axis: 2.4 s across the 520px plot width.

This separation is deliberate and replaces the earlier arrangement, where the four
legends were drawn inside the plot area and the trace was free to run underneath
them. That only looked safe because the reference render happened to show a short
decay: at 200 ms per division the visible window is under two seconds, so a long
decay reaches the right edge and settles near the baseline — exactly where the
−60 dB legend sat, with the grain-state legend exposed the same way.

**Do not substitute a top or bottom margin for the gutter.** 0 dB and −60 dB are
the levels being annotated; the trace has to be able to reach them, so the
separation must be horizontal. A build that reads this section as one rectangle and
clamps the trace to the screen will reintroduce the collision.

### Legends

All 10px, `#8ea0bc`, .18em. None are drawn in the plot region.

| Legend | Placement |
|---|---|
| `DCY ENV` | Title strip, left, 9px in, vertically centred in the 20px strip |
| `GRAIN <n> · <levels> STEP` / `GRAIN OFF · SMOOTH` | Title strip, right, 9px in |
| `0 dB` | Level gutter, x = 532, vertically centred on **y = 26** |
| `−60 dB` | Level gutter, x = 532, vertically centred on **y = 156** |

**Leader ticks** tie each level label back to the level it marks: a 12px horizontal
line from x = 520 to x = 532 at y = 26 and y = 156, `rgba(142,160,188,.85)`, 1.5px.

**Reservation hairlines**, `rgba(120,160,200,.20)`, 1px: one under the title strip
(y = 20, full width) and one down the gutter's left edge (x = 520, y 20→168). They
make the reserved areas legible as instrument furniture rather than as empty space.

### Grid

`rgba(120,160,200,.10)`, 1px, **drawn inside the plot region only** — it stops at
x = 520 and does not run under the legends.

- Vertical: every 200 ms — 11 interior lines at `x = n × 43.33`, n = 1…11.
- Horizontal: 3 interior lines at quarter-points of full scale — `y = 26 + n × 32.5`, n = 1…3.

Drawing model (plot region 520 × 148 as defined above, baseline y = 156, full-scale
height 130, time axis 2.4 s across the plot width, `τ = decaySeconds / 6.0`):

1. **Noise tail** behind: 240 vertical hairlines at `rgba(190,205,225,.16)`, each from the baseline up to `exp(−t / (τ×1.6)) × noise[i] × (0.5 + density×0.5)`, scaled to full-scale height less 4px. The random series is **seeded once at construction and never regenerated per frame**.
2. **Envelope trace** from `preDelayX` rightward: `env = exp(−t / (τ×1.6))`, modulated `× (1 + mod × 0.10 × sin(x×0.075 + phase×12))`, scaled `× (0.55 + density×0.45)`.
3. **Grain quantisation** — the signature behaviour. When grain > 0.03, quantise `env` to `levels = max(3, round(30 − grain×26))` steps and advance x by `3 + grain×24` px, emitting an extra point at the previous y before each new x. The result is a sample-and-hold stair-step. At grain 0 the curve is smooth with a 3px step. **This is the visual payoff of the plugin and must survive implementation.**
4. **Sweep**: the trace is clipped at `x = 52 + phase × 745`, phase advancing 0.016 per ~40 ms tick and wrapping — the trace paints in and re-triggers like a real scope. The sweep clip is in addition to the hard plot-region clamp at x = 520, never instead of it. In JUCE, a `Timer` at 25–30 Hz repainting the scope component only.
5. **Fill** under the trace: accent at 16%.
6. **Stroke** drawn twice: a 2.6px pass at 75% opacity through a Gaussian bloom (`stdDeviation 3.2`, merged twice), then a crisp 1.6px pass.

Grain-state legend: `GRAIN <n> · <levels> STEP`, or `GRAIN OFF · SMOOTH` when grain ≤ 0.03. It lives in the title strip (see above), not in the plot region.

---

## 12. Interaction

- **Knobs**: pointer-down + vertical drag; 180px of travel = full 0 → 1. Cursor `grab`.
  Add in the build (absent from the reference render): **shift = ×0.25 fine sensitivity** and **double-click restores default**.
- **ALGORITHM**: click a corner label to select it; clicking the knob body advances to the next index. In the build prefer click-to-nearest-detent and scroll-through as well. Always snaps — no intermediate positions.
- **LCD**: click anywhere in the cell to open the dropdown.
- Every control responds immediately. There are no loading, error or validation states on this panel.
- No responsive reflow — the panel scales as one unit.

---

## 13. Parameter inventory

Take this read-only from the build and reconcile before any artwork is finalised
(BRAND.md § Before commissioning artwork). All eleven continuous parameters are
normalised 0–1 internally and none are quantised. ALGORITHM is a 4-position
discrete choice.

| ID | Display name | Min | Max | Unit | Taper | Default |
|---|---|---|---|---|---|---|
| `size` | SIZE | 0.2 | 1.0 | — | linear | 0.71 |
| `decay` | DECAY | 0.4 | 8.0 | s | linear | 4.8 |
| `predly` | PRE-DELAY | 0 | 180 | ms | linear | 40 |
| `density` | DENSITY | 0 | 100 | % | linear | 72 |
| `hf` | DAMPING HF | 2 | 16 | kHz | **log, `2·8^f`** | 4.8 |
| `lf` | DAMPING LF | 40 | 500 | Hz | **log, `40·12.5^f`** | 81 |
| `mod` | MODULATION | 0 | 100 | % | linear | 34 |
| `grain` | DIGITAL GRAIN | 0 | 100 | — | linear | 46 |
| `width` | STEREO WIDTH | 0 | 200 | % | linear | 140 |
| `mix` | MIX | 0 | 100 | % | linear | 55 |
| `trim` | OUTPUT TRIM | −12 | +12 | dB | linear | 0 |
| `algo` | ALGORITHM | 0 | 3 | — | discrete, 4 detents | 0 (PLATE) |

**Every default in this table is a physical value, in the row's own unit.** SIZE
was the one row stating a normalised position (0.64); it now states the physical
default **0.71**, which is where the pointer sits on the printed 0.2–1.0 scale and
what § 9 has always shown in the readout. Normalised 0.64 is correct as the stored
value — the two numbers describe the same knob position and both are right in
their own frame.

`phase` (scope sweep) is animation state only — **not** a parameter and not automatable.

**Defaults above are the design's proposal.** Reconcile against the build's
`AudioProcessorValueTreeState` before finalising; where they disagree, the build's
numbers win and the reference render should be re-shot.

---

## 14. Fonts and assets

Google Fonts, both OFL, safe to embed as JUCE `BinaryData`:
**Jost** (400/500/600) and **IBM Plex Mono** (400/500/600).

No bitmaps, no filmstrips, no plate. The product icon is unchanged from the
previous handoff — `icon/icon-1024.png`, `icon/icon-256.png` and the 128/64/32
review cuts still apply, and nothing in this pass touches them.

---

## 15. Open items

- Fine-drag (shift) and double-click-to-default are specified above but not in the reference render.
- ~~The SAVE naming flow is stubbed in the prototype.~~ **Closed this pass** — the inline LCD name entry is built and is render `05`; see § 9 *Name entry*. Remaining gap: not demonstrated. Build it from § 9, not from the prototype's behaviour.
- IN / OUT meters are sample text in the render; wire to real peak metering.
- Reconcile § 13 against the build's actual parameter table before implementation.
- Update the Roster row for Reflect-84 in `BRAND.md` when this pass lands.
