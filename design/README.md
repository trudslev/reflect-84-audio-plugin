# Handoff: REFLECT-84 — Neon Foundry reverb processor GUI

## Overview
REFLECT-84 (model **RF-84**, stereo) is the next Neon Foundry casting: a reverb
processor whose panel metaphor is a **1980s cream/parchment-fascia digital rack unit**
with a recessed dark-blue control bezel. The GUI presents one algorithm selector,
a reverb tank section, damping, character controls, a live decay-envelope scope,
and an output section — all on a single fixed-aspect panel.

The signature element is the **live decay-tail scope** ("TANK LIVE"): the envelope
of the reverb tail sweeps left-to-right in real time and visibly *quantizes into
stair-steps* as the DIGITAL GRAIN control is raised. That behavior is the point of
the plugin's identity — it must survive implementation.

## About the design files
The files in this bundle are **design references created in HTML** — a working
prototype showing intended look and behavior. They are **not production code to
copy**. The task is to recreate this design in the target codebase's existing
environment. For Neon Foundry that means **JUCE / C++** (`Source/`, a
`LookAndFeel` subclass + custom `Component`s), following the patterns already
established in `taperot/Source/` and `gatecrasher/Source/` — the knob drawing,
LED, and scope components there are the precedent. If you are implementing this
somewhere with no existing environment, pick the framework that fits and port the
values below literally.

`BRAND.md` is included: the shared Neon Foundry DNA this design was built against.
Read it before implementing — especially the one-accent-color rule and the
"Programs, never Presets" terminology rule.

## Fidelity
**High-fidelity.** Colors, type, sizes, and interaction behavior below are final
and should be matched closely. The one intentionally soft area is the exact
gradient/shadow stack used to fake sculpted plastic and metal — reproduce the
*read* (top-lit, slightly domed, soft contact shadow) using whatever the
codebase's existing knob rendering already does, rather than transcribing CSS
gradients into JUCE paint calls one-for-one.

## Canvas
- Reference size: **1200 × ~530 px** panel (outer element is `width: 1200px`,
  `padding: 14px`). Preview frame is 1228 × 560.
- Fixed aspect, scales proportionally. This is wider than the 960×400 BRAND.md
  reference because the control set is denser; ratio is close (≈2.26 vs 2.4).
- Outer panel: `border-radius: 10px`, drop shadow `0 24px 60px rgba(0,0,0,.55)`,
  inner highlight `inset 0 1px 0 rgba(255,255,255,.75)`,
  inner base shade `inset 0 -2px 6px rgba(90,70,40,.18)`.
- Fascia fill: vertical gradient `#efe6d0 → #e2d8bd (60%) → #d8cdb0`.
- Overlay texture (non-interactive, full-bleed): 1px horizontal scanlines
  `rgba(80,60,25,.035)` every 3px, plus a top-left radial sheen
  `radial-gradient(120% 90% at 20% 0%, rgba(255,255,255,.5), transparent 60%)`.

## Layout
Two stacked regions inside the panel padding:

1. **Header bezel** — full width, `padding: 14px 22px`, `gap: 26px`, three columns:
   wordmark block (min-width 300px) · PROGRAM block (flex:1) · IN/OUT meters (right).
2. **Body row** — `padding: 20px 4px 6px`, three columns separated by 1px vertical
   hairline dividers:
   - Left column: **336px** wide, `padding-right: 22px` — ALGORITHM rotary + hairline + REVERB TANK + DAMPING
   - Center column: **flex: 1**, `padding: 0 24px` — TANK LIVE scope + CHARACTER
   - Right column: **176px** wide, `padding-left: 22px` — OUTPUT

Vertical dividers: 1px, `linear-gradient(180deg, transparent, rgba(120,98,55,.4) 20%, rgba(120,98,55,.4) 80%, transparent)` with `box-shadow: 1px 0 0 rgba(255,255,255,.65)` (the engraved-groove look).
Horizontal divider (left column): 1px, `linear-gradient(90deg, rgba(120,98,55,.35), transparent)`, `box-shadow: 0 1px 0 rgba(255,255,255,.6)`.

## Screens / sections

### 1. Header bezel
Recessed dark-blue plate: gradient `#22304c → #1a2740 (55%) → #142036`,
`1px solid rgba(0,0,0,.5)`, `radius 6px`,
shadows `inset 0 1px 0 rgba(255,255,255,.12), inset 0 -3px 8px rgba(0,0,0,.45), 0 2px 4px rgba(60,45,20,.35)`.

**Wordmark** — text `REFLECT-84`, Jost 500, **42px**, `line-height: .92`,
`letter-spacing: .10em`, color `#f0e2ba`, engraved text-shadow
`0 1px 0 rgba(0,0,0,.65), 0 -1px 0 rgba(255,255,255,.18), 0 2px 10px rgba(0,0,0,.4)`.
Nameplate metaphor: **etched/engraved metal legend on the bezel** — distinct from
TapeRot's Dymo label and Gatecrasher's stencil, per BRAND.md.

Beneath, stacked 3px apart, IBM Plex Mono 10px, `letter-spacing: .30em`, `#a9b6cd`:
- `REVERB PROCESSOR`
- `MODEL RF-84 · STEREO`

**PROGRAM block** — label `PROGRAM`, 9px, `.34em` tracking, `#c8b177`.
Row of three controls, gap 10px:
- **Program display** (flex:1, max-width 430px): LCD well, gradient `#0a0f18 → #060a11`,
  `1px solid rgba(0,0,0,.7)`, radius 3px, `inset 0 2px 7px rgba(0,0,0,.8)` + `0 1px 0 rgba(255,255,255,.12)`.
  Left: a `FACT` badge — 11px, `.16em`, `#f2e6c2`, 1px border `rgba(242,230,194,.4)`, radius 2px, padding `2px 7px`
  (reads FACT for factory bank; a user program would read `USER`).
  Center: program name, 17px, `.16em`, `#f2e6c2`, text-shadow `0 0 9px rgba(242,230,194,.35)` (phosphor glow).
  Right: an 8px chevron (rotated 45° square border, `#d8c18a`) at `right: 11px`.
  Hover: well lightens to `#0e1522 → #080d16`.
- **SAVE** button: brass, gradient `#ded0a6 → #bda979`, text `#2a3550` 10px `.20em`,
  radius 3px, `inset 0 1px 0 rgba(255,255,255,.55), 0 1px 2px rgba(0,0,0,.4)`.
  Hover `#eadcb4 → #cbb787`.
- **DELETE** button: disabled by default — gradient `#232f49 → #1b2640`, text `#4a5670`,
  `inset 0 1px 3px rgba(0,0,0,.5)`, `cursor: not-allowed`,
  tooltip "Enabled only when a User Program is loaded".

**IN / OUT meters** (right, gap 10px): label 9px `.28em` `#a9b6cd` above an LCD well
(same well recipe, `padding 6px 12px`, `min-width 58px`, 16px `#e8dcba`).
Prototype shows static `-3.2` / `-0.8`; in the real plugin these are live peak dB readouts.

### 2. ALGORITHM rotary (left column)
- Body 104 × 104px, `radius 50%`, fill `radial-gradient(circle at 50% 26%, #1f2b44, #16223a 52%, #0d1526 80%, #070c15)` — dark, matching the bezel, so it reads as the one "system" control among brass knobs.
- Shadows: `0 5px 12px rgba(45,33,12,.45), inset 0 1px 1px rgba(255,255,255,.16), inset 0 -6px 12px rgba(0,0,0,.6)`. Inner disc `inset: 12px` with a soft top sheen.
- Pointer: 3px × 36% of radius, `linear-gradient(#f4e8c4, #cdb989)`, radius 2px, from `top: 9px`.
- Tick ring: `inset: -15px`, dense conic ticks starting at 224.45°, masked to a ring.
- **4-position detented switch**, not a continuous knob. Pointer angles: `-45° + index × 90°`.
- Corner labels at the four diagonals, 10px, `.18em`, clickable:
  `PLATE` (top-left, index 0) · `DIGITAL ROOM` (top-right, 1, wraps to two lines) ·
  `HALL` (bottom-left, index 3) · `CHAMBER` (bottom-right, index 2).
  Selected label `#2f2718`; unselected `#9a8e74`.
  ⚠️ Note the label→index mapping is *not* clockwise-sequential — HALL is index 3 at bottom-left. Keep the visual arrangement; map indices to your DSP enum however that enum is ordered.
- Caption `ALGORITHM` below, 9px, `.26em`, `#6d6148`, 16px above it.
- Interaction: click a corner label selects it; pointer-down on the knob body advances to the next algorithm (in a real build, prefer click-to-nearest-detent or scroll-through).

### 3. Section header pills
`REVERB TANK`, `CHARACTER`, `OUTPUT` all use the same pill:
`padding 4px 11px`, radius 3px, gradient `#22304c → #16223a`, text 9px `.26em` `#d8c18a`.

### 4. Knobs (the shared grammar)
All continuous knobs share one component with four size variants. Common rules:
- Brass/ivory sculpted body, top-lit.
- Travel arc **270°**, from `-135°` to `+135°`; pointer rotation = `-135 + value × 270` degrees.
- Tick ring drawn outside the body, masked to the 270° arc only (starts at 225°).
- Pointer: dark line (`#33291a`, or `#2f2617` on the large variant) from near the top edge inward.
- Label below in 9px `.18em` `#5c5241`; numeric readout below that in 10px `#7d7159`.
- Drag: **vertical**, `Δvalue = (startY − currentY) / 180` px, clamped 0–1. Cursor `grab`.

| Variant | Body | Tick inset | Pointer | Body gradient | Label / readout |
|---|---|---|---|---|---|
| Large (CHARACTER) | 98px | −13px, 1° every 22.5° | 3px × 30% | `radial(50% 22%, #fdf6e0, #ddcb98 52%, #b09a61 76%, #7d6a3b)` + inner cap `inset:15px` | 11px `.22em` `#4a4132` / 11px |
| Medium (REVERB TANK) | 60px | −9px, 1.2° every 27° | 2px × 38% | `radial(50% 24%, #fbf3da, #d6c391 55%, #a58f58 80%, #7a6738)` | 9px `.18em` |
| Small (OUTPUT) | 52px | −8px, 1.3° every 33.75° | 2px × 37% | `radial(50% 24%, #f9f1d8, #d4c18e 58%, #9f8a55 82%, #77653c)` | 9px `.16em` |
| Tiny (DAMPING) | 44px | −7px, 1.4° every 45° | 2px × 36% | `radial(50% 24%, #f7efd6, #d2bf8c 58%, #9d8853 82%, #75633a)` | 9px `.16em` |

Large shadow: `0 6px 14px rgba(45,33,12,.45), inset 0 2px 2px rgba(255,255,255,.75), inset 0 -8px 14px rgba(90,70,30,.35)`.
Medium: `0 4px 9px rgba(45,33,12,.4), inset 0 1px 1px rgba(255,255,255,.7), inset 0 -5px 10px rgba(90,70,30,.35)`.
Small/tiny: `0 3px 8px rgba(45,33,12,.38), inset 0 1px 1px rgba(255,255,255,.68)`.

### 5. Parameters
Knob size communicates importance, per BRAND.md.

**REVERB TANK** — 4-up grid, gap `14px 8px`:

| Param | Default (0–1) | Readout | Mapping |
|---|---|---|---|
| SIZE | 0.64 | `0.71` | `0.2 + v×0.8`, 2 dp |
| DECAY | 0.58 | `4.8 s` | `0.4 + v×7.6` seconds, 1 dp |
| PRE-DLY | 0.22 | `40 ms` | `round(v×180)` ms |
| DENSITY | 0.72 | `72%` | `round(v×100)%` |

**DAMPING** — 2 tiny knobs, gap 20px, with the word `DAMPING` set as a 9px `.24em` `#6d6148` label to the *left* of the pair (absolute, `right: calc(50% + 70px); top: 18px`):

| Param | Default | Readout | Mapping |
|---|---|---|---|
| HF | 0.42 | `7.9 k` | `2 + v×14` kHz, 1 dp |
| LF | 0.28 | `169 Hz` | `round(40 + v×460)` Hz |

**CHARACTER** — 2 large knobs, gap 70px:

| Param | Default | Readout | Mapping |
|---|---|---|---|
| MODULATION | 0.34 | `34%` | `round(v×100)%` |
| DIGITAL GRAIN | 0.46 | `46` | `round(v×100)`, unitless |

**OUTPUT** — 3 small knobs stacked, gap 17px:

| Param | Default | Readout | Mapping |
|---|---|---|---|
| WIDTH | 0.70 | `140%` | `round(v×200)%` |
| MIX | 0.55 | `55%` | `round(v×100)%` |
| TRIM | 0.50 | `0.0` | `(v−0.5)×24` dB, 1 dp |

Version stamp `v1.0` bottom-right of the OUTPUT column, 10px `.1em` `#9a8e74`.

### 6. TANK LIVE scope (center column, the signature element)
Header row above the scope:
- **LED**: 15px circle, `radial-gradient(circle at 40% 32%, <accent lightened>, <accent> 70%, #2a3a24)`,
  glow `0 0 14px 3px <accent @45% alpha>`, inner highlight `inset 0 1px 1px rgba(255,255,255,.5)`.
  Label `TANK LIVE`, 11px `.26em` `#4a4132`. This is the plugin's single LED per BRAND.md
  (reports that the tank is actively decaying).
- Right side, 10px `.20em` `#7d7159`, gap 18px: `DECAY TAIL` · `RT60 <decay seconds>` · `200 ms / DIV`.

Scope bezel: `padding 6px`, gradient `#c9bd9c → #b8aa87`, radius 4px,
`inset 0 2px 4px rgba(70,54,25,.4), 0 1px 0 rgba(255,255,255,.6)`.
Screen: height **168px**, gradient `#080d16 → #050810`, `1px solid rgba(0,0,0,.7)`,
radius 2px, `inset 0 3px 12px rgba(0,0,0,.85)`.
Grid: `rgba(120,160,200,.10)` 1px lines — vertical every **60px**, horizontal every **42px**.
Corner legends, 9px `.2em`: `DCY ENV` (TL, `rgba(190,205,225,.5)`), `0 dB` (TR),
`-60 dB` (BR, `.4` alpha), grain state (BL).

Drawing model (viewBox `600 × 168`, baseline `y = 164`, full-scale height `H − 26`):
1. **Noise tail** (behind): 240 fixed random vertical hairlines at
   `rgba(190,205,225,.16)`, each from baseline up to `exp(−t/(τ×1.6)) × noise[i] × (0.5 + density×0.5)`.
   Randomness is seeded once at construction, never per-frame.
2. **Envelope trace**: from `preDelayX` rightward, `env = exp(−t / (τ×1.6))`,
   with `τ = decaySeconds / 6.0`, time axis spanning **2.4 s** across the width.
   Modulated by `× (1 + mod × 0.10 × sin(x×0.075 + phase×12))`, scaled by `× (0.55 + density×0.45)`.
3. **Grain quantization**: when grain > 0.03, quantize `env` to
   `levels = max(3, round(30 − grain×26))` steps and step the x-axis by `3 + grain×24` px,
   emitting an extra point at the previous y before each new x — i.e. a **stair-step / sample-and-hold**
   trace. At grain 0 the curve is smooth with a 3px step. This is the visual payoff of the plugin.
4. **Sweep**: a refresh sweep clips the trace at `x = 60 + phase × (W + 260)`, phase advancing
   `0.016` per ~40 ms tick and wrapping — so the trace paints in and re-triggers, like a real scope.
5. **Fill** under the trace: accent at 16% (`color-mix(in oklab, accent 16%, transparent)`).
6. **Stroke**: drawn twice — a 2.6px pass at 75% opacity through a Gaussian bloom
   (`stdDeviation 3.2`, merged twice), then a crisp 1.6px pass. That's the phosphor look.

Bottom-left readout text: `GRAIN <n> · <levels> STEP` when grain is on,
`GRAIN OFF · SMOOTH` when grain ≤ 0.03.

## Interactions & behavior
- **Knobs**: pointer-down + vertical drag, 180px = full 0→1 travel. No fine/shift mode in the prototype — add one (shift = ×0.25 sensitivity) and double-click-to-default in the real build.
- **Algorithm**: click corner labels, or click the knob to advance. Snaps to 4 detents.
- **Program display**: click cycles the factory bank (placeholder for a dropdown/menu).
  Factory bank in the prototype: `01 RAIN ALL DAY`, `02 SO LONG`, `03 COLD ATMOSPHERE`, `04 WORLD GONE MAD`.
- **SAVE**: no-op in the prototype. **DELETE**: permanently disabled until a User Program is loaded.
- **Scope**: animates continuously at ~25 fps via rAF. In JUCE, a `Timer` at 25–30 Hz repainting only the scope component.
- All controls respond immediately; no loading, error, or validation states exist in this design.
- No responsive behavior — the panel scales as a whole unit.

## State
- `vals`: 11 normalized floats (`size, decay, predly, density, hf, lf, mod, grain, width, mix, trim`), all 0–1.
- `algo`: integer 0–3.
- `program`: integer index into the factory bank.
- `phase`: 0–1 scope sweep position, animation-only, **not** an automatable parameter.
- Everything except `phase` maps 1:1 to plugin parameters (`AudioProcessorValueTreeState`).
  `grain` and `algo` additionally accept an initial value from the prototype's
  `digitalGrain` / `algorithm` props — those exist only so the mock is tweakable and
  have no meaning in the plugin.

## Design tokens

**Fascia / body**
| Token | Value |
|---|---|
| fascia top / mid / bottom | `#efe6d0` / `#e2d8bd` / `#d8cdb0` |
| engraved line | `rgba(120,98,55,.35–.4)` |
| highlight edge | `rgba(255,255,255,.6–.75)` |
| text primary | `#4a4132` |
| text secondary | `#5c5241` |
| text tertiary / readout | `#7d7159` |
| text muted / caption | `#6d6148`, `#9a8e74` |
| selected label | `#2f2718` |

**Bezel / dark plate**
| Token | Value |
|---|---|
| bezel gradient | `#22304c` → `#1a2740` → `#142036` |
| pill gradient | `#22304c` → `#16223a` |
| bezel label | `#a9b6cd` |
| bezel gold label | `#c8b177`, `#d8c18a` |
| wordmark | `#f0e2ba` |

**LCD**
| Token | Value |
|---|---|
| well | `#0a0f18` → `#060a11` |
| well hover | `#0e1522` → `#080d16` |
| phosphor text | `#f2e6c2` (glow `0 0 9px rgba(242,230,194,.35)`) |
| meter text | `#e8dcba` |

**Scope**
| Token | Value |
|---|---|
| screen | `#080d16` → `#050810` |
| bezel | `#c9bd9c` → `#b8aa87` |
| grid line | `rgba(120,160,200,.10)` |
| legend text | `rgba(190,205,225,.4–.5)` |
| noise tail | `rgba(190,205,225,.16)` |

**Accent — one only, per BRAND.md.** Default `#5ce07a` (phosphor green).
Used *exclusively* for the TANK LIVE LED, its glow, and the live decay trace + its fill.
Never on knobs, labels, or decoration. Alternates explored: `#ffb02e`, `#4ec9ff`, `#ff5c5c`.

**Typography**
- Wordmark: **Jost** 500 — 42px / `.10em`.
- Everything else: **IBM Plex Mono** 400–500.
- Scale: 9px (section labels, small knob labels, scope legends, taglines) · 10px (readouts, buttons, meter labels) · 11px (large knob labels, TANK LIVE, FACT badge) · 16px (meter values) · 17px (program name) · 42px (wordmark).
- Tracking scale: `.10em` (wordmark) · `.16em` · `.18em` · `.20em` · `.22em` · `.24em` · `.26em` · `.28em` · `.30em` · `.34em` — wider tracking for smaller/quieter text.
- If Jost isn't available in the JUCE build, substitute a geometric grotesque with a
  tall x-height; do **not** substitute a neutral system sans.

**Radii**: 2px (LCD screen, FACT badge) · 3px (LCD well, buttons, pills) · 4px (scope bezel) · 6px (header bezel) · 10px (panel).

## Assets
No bitmap assets. Everything is CSS gradients, shadows, masks, and one inline SVG
(the scope trace). Fonts are Google Fonts: **Jost** (400/500/600) and **IBM Plex Mono** (400/500/600) —
both OFL, safe to embed as `BinaryData` in the JUCE build.

The tick rings use conic gradients with a radial+conic mask composite; in JUCE draw
these as a loop of short line segments around the arc instead.

## Product icon

Direction: **DECAY STEPS** — the quantized decay tail on scope glass, the same visual
idea as the TANK LIVE scope. Rounded-square plate in the bezel gradient
(`#22304c → #141f34`), corner radius **22% of the icon box** at every size, trace in
the panel accent. If the accent changes, the trace changes with it.

Hand-tuned optical ramp — three cuts, not one artwork scaled:

| Cut | Use | Detail |
|---|---|---|
| Full | 256px and up | Grid (32px), top-left sheen, baseline, gradient fill under the trace, 5px bloom + 3px crisp stroke, **6 steps** |
| Mid | 64–128px | No grid, no fill; baseline kept; 3px bloom + 4px crisp stroke, **4 steps** |
| Small | 32–48px | Trace only — no bloom, no baseline, no plate texture; 5px flat stroke in `#8ff2a4`, **3 steps** |

Glow turns to mush below 48px, which is why the small cut drops it. Trace colors:
`#5ce07a` for the bloom pass, `#a6f5b6` crisp on top at large sizes, flat `#8ff2a4` small.

Exported PNGs in `icon/`: `icon-1024.png`, `icon-256.png`, `icon-128.png`,
`icon-64.png`, `icon-32.png`. `Reflect-84 Icon.dc.html` holds the live source with
all three cuts and an in-situ size row.

## Files
- `Reflect-84 Icon.dc.html` — icon source, all three optical cuts.
- `icon/*.png` — exported icon at 1024 / 256 / 128 / 64 / 32.
- `screenshots/01-panel.png` — full panel at 2× (2400 × 1230).
- `screenshots/02-panel-blank.png` — the bare fascia at 2×: panel gradient, scanline
  texture, top-left sheen, rounded edge and inner highlight, with every control and
  label removed. Use this to match the background material before placing any controls.
- `Reflect-84.dc.html` — the design prototype. Open directly in a browser.
- `support.js` — runtime needed by the prototype. Keep it beside the HTML.
- `BRAND.md` — Neon Foundry shared design DNA. Read first.

## Open items for the developer
- Fine-drag (shift) and double-click-to-default aren't in the mock; add them.
- Program menu is a click-to-cycle placeholder — needs a real menu + user bank + SAVE/DELETE wiring.
- IN/OUT meters are static text in the mock — wire to real peak metering.
- Update the Roster table in `BRAND.md` with REFLECT-84 when work starts.
