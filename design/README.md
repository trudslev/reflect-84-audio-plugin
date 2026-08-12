# Handoff: REFLECT-84 — Neon Foundry reverb processor GUI

REFLECT-84 (model **RF-84**, stereo) is a reverb processor on a 1980s
cream/parchment fascia with a recessed navy control bezel. Its signature element is
the **live decay-tail scope** — the reverb envelope sweeps left-to-right in real
time and visibly quantises into stair-steps as DIGITAL GRAIN rises.

This bundle is the **v1.1 conformance pass** — see `CHANGELOG.md` to confirm which
revision you are holding. Identity is settled and unchanged:
cream fascia, navy header bezel, engraved-plate wordmark, decay-tail scope,
phosphor green as the single accent. The pass covers printed scales, layout room
and contrast.

**Canvas is 1340 × 645**, four body columns. If you are working to 1200 × 615 or
1200 × 530, or reading a section 6 that describes the scope, you have an earlier
revision — the panel grew in this pass to make room for the printed scales.

## Do not install this bundle over the build tree

It is a **reference package, not a directory to sync**. It makes no claim on
`design/` or any build-owned path — in particular it must never replace or empty
`design/fonts/`. Copy out the two documents and the reference renders; leave
everything else alone.

## Read in this order

0. **`CHANGELOG.md`** — confirms the revision. If the bundle has no `GUI-SPEC.md`,
   it is v1.0 and nothing from this pass reached you.

1. **`BRAND.md`** — the shared Neon Foundry DNA. Read first, from the repo: it is
   not bundled here, deliberately, since a copy starts contradicting the repo as soon
   as the suite document moves. `GUI-SPEC.md` cites it by section throughout.
2. **`GUI-SPEC.md`** — the build contract: every position and size, the palette
   with measured contrast ratios, type metrics per role, per-knob mark and tick
   tables with rotation fractions, the dropdown and naming flow, corrected readout
   formats, and the bypass treatment.
3. **`screenshots/header/`** — the header row rendered once per panel state at 3×,
   five images matching the state matrix in `GUI-SPEC.md` § 9. **Check the built
   header against these, not against the prose** — the legend bloom is the one part of
   this design that numbers cannot settle.
4. **`screenshots/01-panel.png`** — the whole panel at 2×, for everything else.
   `02-panel-blank` is the bare fascia and `03-panel-lit` an edited User Program;
   `06-panel-menu-open`, `07-menu-scroll-top` and `08-menu-scroll-end` are the
   Program list — closed-panel geometry plus both chevron states. All six are
   **2680 × 1298, 2× the 649 canvas**.

The five header renders are the whole row — wordmark, PROGRAM LCD, SAVE, DELETE, IN
and OUT in one image — deliberately, so the shared 34px band and single baseline are
checkable. A crop of the button pair alone cannot show whether the buttons sit level
with the LCD beside them, which is the drift we are trying to make visible.

Because the panel is wholly code-drawn there is no plate to composite against, so
these renders are the only external check that exists. They are references for
comparison and **never build inputs**: nothing in this bundle is a backing image.

## Asset format

**Vector / code-drawn. No exported bitmaps, no filmstrips, no plate.** The fascia
is a gradient with a procedural texture, and every heading, label, legend, scale
numeral and tick is drawn at runtime. Everything in `GUI-SPEC.md` becomes drawing
code. The PNGs here are references for comparison, never backing images.

Target environment is **JUCE / C++** (`Source/`, a `LookAndFeel` subclass plus
custom `Component`s), following the patterns already in `taperot/Source/` and
`gatecrasher/Source/` — their knob, LED and scope components are the precedent.
Gatecrasher has been through this same conformance pass and is the reference for
both convention and density.

## Fidelity

**High-fidelity.** Colours, type, sizes, geometry and behaviour in `GUI-SPEC.md`
are final and should be matched closely. The one intentionally soft area is the
exact gradient/shadow stack that fakes sculpted plastic and metal — reproduce the
*read* (top-lit, slightly domed, soft contact shadow) using whatever the codebase's
existing knob rendering already does, rather than transcribing CSS gradients into
`paint()` calls one for one.

## What changed in v1.1

- **Printed scales on every knob**, replacing the standing value readouts, which
  are removed. The scale is now the only at-rest value reference, so it is
  functional text at 7:1.
- **Damping promoted 44px → 52px.** The tiny variant is retired; the panel now uses
  three knob sizes. Five 10px numerals will not clear a 29px tick radius, and the
  type floor was not negotiable. Spacing was opened at the same time.
- **Damping tapers changed to log.** Linear in Hz put DAMPING HF's midpoint at
  9 kHz; log makes the marks octaves and lands them evenly. The build changes the
  taper to match.
- **Ticks moved onto labelled values.** Linear controls stay evenly spaced; DECAY
  and both damping knobs become irregular, which is correct.
- **Algorithm tick alignment fixed** — ticks are now specified by centre angle,
  coinciding exactly with the pointer detents. The corner ordering stays
  deliberately non-sequential.
- **Contrast audited across every text role**, with measured ratios in the spec.
  The unselected algorithm label was the worst offender at 2.10:1 and disabled
  DELETE was 2.04:1; both are fixed. Opacity-driven hierarchy on the scope legends
  is gone.
- **Corrected readout formats** — `+0.0 dB` with an explicit sign, `7.9 kHz` not
  `7.9 k`, ` %` on the percentage parameters. SIZE and DIGITAL GRAIN stay bare.
- **LCD takes on live parameter readout** while a control is moved, and the
  dropdown, User-Program saving and DELETE-disabled treatment now follow TapeRot's
  structure exactly, in Reflect-84's palette.
- **Bypass added** — a 0.50 multiply over the panel, host-driven, no on-panel
  control, no caption.
- Layout opened to make room: panel is now **1340 × 645** (was 1200 × 530), four
  body columns instead of three.

## Product icon — unchanged

Not affected by this pass. Direction is **DECAY STEPS**: the quantised decay tail on
scope glass, the plugin's own signature element. Rounded-square plate in the bezel
gradient (`#22304c → #141f34`), corner radius 22% of the icon box at every size,
trace in the panel accent — if the accent changes, the trace changes with it.

Three hand-tuned optical cuts, not one artwork scaled:

| Cut | Use | Detail |
|---|---|---|
| Full | 256px and up | Grid (32px), top-left sheen, baseline, gradient fill, 5px bloom + 3px crisp stroke, 6 steps |
| Mid | 64–128px | No grid, no fill; baseline kept; 3px bloom + 4px crisp stroke, 4 steps |
| Small | 32–48px | Trace only — no bloom, no baseline, no plate texture; 5px flat stroke `#8ff2a4`, 3 steps |

Glow turns to mush below 48px, which is why the small cut drops it. Trace colours:
`#5ce07a` bloom pass, `#a6f5b6` crisp on top at large sizes, flat `#8ff2a4` small.

Exports in `icon/`: 1024, 256 (both JUCE requirements), plus 128 / 64 / 32 review
cuts. `Reflect-84 Icon.dc.html` is the live source.

## Files

| File | What it is |
|---|---|
| `CHANGELOG.md` | Revision log — check this first |
| `GUI-SPEC.md` | The build contract for this panel |
| `fonts/README.md` | Font families, weights, licence and sources. **No binaries — this folder never overwrites the build's own `design/fonts/`.** |
| `screenshots/01-panel.png` | Reference render, 2× (**2680 × 1298, exactly 2× the 649 canvas**) — measurable in X *and* Y |
| `screenshots/header/01-rest-nothing-to-do.png` | **Factory Program, unmodified**, 3×. All four legends dark. **The important one** — it has to read as *"nothing to do here"*, not as a blank or broken button, and it is the case most likely to look wrong while measuring right. |
| `screenshots/header/02-factory-edited-save-lit.png` | **Factory Program, edited**, 3×. SAVE lit; note the ` *` dirty marker in the LCD, which reads the same flag as SAVE's lamp. |
| `screenshots/header/03-user-unmodified-delete-lit.png` | **User Program, unmodified**, 3×. DELETE lit, SAVE dark. |
| `screenshots/header/04-user-edited-save-delete-lit.png` | **User Program, edited**, 3×. SAVE and DELETE both lit — the only state with two lamps on one row. |
| `screenshots/header/05-naming-store-cancel-lit.png` | **Naming a Program**, 3×. STORE and CANCEL lit, SAVE and DELETE dark, badge `NAME`, block caret at the end of the seeded draft. |
| `screenshots/03-panel-lit.png` | Whole panel on an edited User Program, 2× — the same state as header `04`, for context around the row. Judge the bloom from the 3× header renders, not from this. |
| `screenshots/02-panel-blank.png` | Bare fascia at 2× — gradient, scanline texture, sheen, edge and inner highlight, every control and label removed. Match the background material before placing controls. |
| *(no bypassed render)* | The disengaged state is **specified, not rendered** — see `GUI-SPEC.md` § 10. Our capture path does not composite `mix-blend-mode: multiply` faithfully, so any PNG we shipped would read as a dimmer switch rather than a light going out, which is the exact failure § 10 warns against. Build it from the spec: full-bleed `#808080` multiply at 0.50 over every panel layer. |
| `Reflect-84 v1.1.dc.html` | Working prototype of the conformed panel. Opens directly in a browser. |
| `Reflect-84 Icon.dc.html` | Icon source, all three optical cuts |
| `icon/*.png` | Icon at 1024 / 256 / 128 / 64 / 32 |
| `support.js` | Runtime for the prototypes. Keep it beside them. |

The prototypes are design references created in HTML — a working demonstration of
look and behaviour, not production code to port. Recreate the design in the JUCE
codebase from `GUI-SPEC.md`.

## Open items

- Fine-drag (shift = ×0.25) and double-click-to-default are specified but not in
  the prototype.
- ~~The SAVE naming flow is stubbed in the prototype.~~ **Built this pass** — seeded
  field, upper-cased input, Enter commits, Esc cancels leaving the Program edited,
  cap 42. It is render `05`; spec at `GUI-SPEC.md` § 9 *Name entry*.
- IN / OUT meters are sample text in the prototype; wire to real peak metering.
- Reconcile `GUI-SPEC.md` § 13 against the build's actual parameter table —
  especially the two damping tapers — before implementation.
- Update the Roster row for Reflect-84 in `BRAND.md` when this pass lands.
