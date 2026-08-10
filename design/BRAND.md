# Neon Foundry — Brand & Design System

This document is the shared reference for every plugin under the Neon Foundry
umbrella. Each plugin gets its own distinct visual identity — but they share a
common DNA. This file is that DNA: the rules that stay constant so a new plugin
feels like family without copying an existing one wholesale.

When starting or extending any plugin in this suite, read this file first, then
read that plugin's own `design/` folder and GUI source for its specific
execution of these rules.

## Philosophy

Neon Foundry plugins emulate the *feeling* of specific eras and classes of
outboard hardware — not a single shared skin. Each plugin should look like it
could plausibly have shipped from a real 1980s hardware manufacturer, with its
own fascia material, its own nameplate treatment, its own signature display —
chosen because it fits that plugin's function, not because it matches its
siblings.

What ties them together instead:

- The same underlying *component grammar* (below) — how knobs, LEDs, displays
  and headers behave, even when their finish differs
- The same commitment to specificity — real hardware spec-sheet language,
  plausible model numbers, functional-not-decorative detail
- The same restraint — one signature element per plugin, everything else quiet
  and disciplined around it
- Shared naming, terminology and legibility conventions

"Old process, new signal" — hand-forged craft, cast in electric material.

## Naming & terminology

- The umbrella brand is **Neon Foundry**. It appears in marketing, on the site
  and in this document — **never on a plugin's own panel.** Each plugin's
  interface is signed only by its own wordmark and model line; it doesn't need
  to credit the umbrella to belong to it.
- Individual plugins are called **castings** in marketing copy ("the foundry's
  third casting"). Copy language only, never a UI label.
- Stored parameter snapshots are **Programs**, never "Presets" — period-correct
  hardware terminology (Lexicon, dbx and AMS-era units all called them
  programs). Applies to the UI label, the code, factory bank documentation and
  marketing copy alike.
- Each plugin carries a **model line** in hardware spec-sheet voice: a function
  descriptor and a model number with a real anchor behind it, e.g.
  `BUS COMPRESSOR / MODEL GL-87 · STEREO` (Glue, 1987 — the SSL 4000 G year).
  Numbers should mean something, not just evoke the decade.

## Shared component grammar

Consistent across every plugin, whatever the finish:

- **Knobs.** Sculpted body, tick marks around the travel arc, a single clean
  pointer line. Size communicates importance — chunkier for primary character
  controls, smaller for secondary ones.
- **Knobs never change appearance to indicate per-control state.** A knob body
  renders identically regardless of which mode is selected or whether that
  particular control is currently relevant — it never dims, greys out or goes
  "inert" for that reason; an LED communicates it instead. Whole-effect
  disengagement is a separate case with its own rules — see *Bypass and
  disengaged states* below.
- **LEDs indicate conditional activity.** Where a control's relevance depends
  on another switch — a mode selector, a sync toggle, an auto position — a
  small LED beside its label shows whether it is currently live. Controls that
  are *always* live get no LED; adding one there dilutes the signal. A plugin
  may have one LED or many depending on how much genuine conditionality its
  panel has, or none at all.
- **Where a control has several possible identities** across modes, print
  *all* its labels permanently and stack them, each with its own LED. The
  panel never relabels itself — only the lit LED changes. Printed text on a
  physical panel cannot rewrite itself; a display can.
- **One signature live display per plugin.** Something worth watching in real
  time, rendered as a real instrument rather than decoration. This may be an
  oscilloscope-style readout (grid, division-rate label, e.g. "50 ms / DIV") or
  an analog moving-coil meter — whichever suits the process. No two plugins
  should use the same kind.
- **Dynamic text belongs in displays, never on the panel.** Anything that
  changes at runtime must live inside a screen and use that screen's
  LCD/segment typeface. A printed panel label that swaps its own text breaks
  the object.
- **Nameplate.** Each plugin gets a distinct physical-application metaphor for
  its wordmark. Already used: hand-applied label-maker strip (TapeRot),
  distressed spray stencil (Gatecrasher), printed silkscreen (Chorus-60),
  engraved plate (Reflect-84), hand-marker on gaffer tape (Fifth Member),
  paint-filled moulded relief (Elmer). Pick a new one; don't reuse.
  Relief-based treatments must be **paint-filled** — pure highlight-and-shadow
  on a same-value panel is unreadable.
- **Header layout.** Wordmark, function descriptor and model line at the left;
  PROGRAM LCD centre with a single FACT/USER indicator built into the LCD
  itself; SAVE and DELETE beside it; numeric IN/OUT dB meters at the right.
  The LCD must be wide enough to hold a parameter name and value comfortably,
  since it doubles as the live value readout (see below).
- **Program management.** SAVE always creates a *new* named Program and never
  overwrites, even when a User Program is loaded — so there is no separate
  "New Program" control. DELETE works only on User Programs and is visibly
  disabled on Factory ones.
- **Canvas.** Each plugin uses a fixed aspect ratio suited to its control
  count; it does not have to match its siblings. Whatever the ratio, the GUI
  must scale proportionally, and the scaling range must go far enough up to be
  a genuine accessibility lever — not a token 10–15%.

## Legibility

The audience for 80s-themed gear skews heavily toward people who were there in
the 80s, which means a large share of buyers have age-related near vision
changes. Faint small text is a bad bet for this suite specifically. These are
requirements, not preferences:

- **Functional text** — control labels, printed scales, section headers,
  readouts, the model/function line — sits at roughly **7:1 contrast** against
  the panel.
- **Flavour text** never falls below about **4.5:1**. Below that it is not
  "subtle," it is decoration pretending to be information — either make it
  legible or remove it.
- **No functional text below ~9–10px** at the plugin's default window size. If
  something has to be smaller than that, it isn't functional and should be cut.
- **Hierarchy comes from size and weight, never from opacity.** Do not fade
  text to push it down the hierarchy.
- **Colour never carries meaning on its own.** Colour-coded controls must also
  be labelled — colour is organisation, not information.
- **Check at 100%.** Every string on the panel must be readable without
  zooming. Anything that isn't, isn't finished.

## Parameter values & readouts

- **No standing numeric readouts under knobs.** Hardware panels print a
  *scale*, not a live value; you read the pointer against it.
- **Every continuous knob carries a printed scale** with several values, not
  just endpoints — enough marks that the pointer position is actually
  readable. Printed scales are legend, which real panels have; explanations of
  what a control does are not, and don't belong on a faceplate.
- **Ticks sit at the labelled values, not at even angles.** A tick marks a
  number; it isn't a decorative ring. On linear controls that comes out evenly
  spaced anyway, but on a skewed one it must not — a numeral sitting visibly
  off its nearest tick reads as an error even when the numeral is correct. Any
  minor ticks between majors must also fall on real values.
- **Live values appear in the plugin's PROGRAM LCD while a control is being
  moved**, reverting to the program name shortly after release. Digitally
  controlled hardware of this era did exactly this, and it reuses a display
  already on the panel rather than floating a tooltip that has no hardware
  equivalent. Use the display's own LCD/segment typeface. **Only direct user
  manipulation triggers it** — host automation must not drive the readout, or
  the display flickers through values for the length of a song.
- **Printed scales and actual parameter mappings must agree exactly** — the
  pointer sitting on a printed mark must report that value. Watch logarithmic
  controls in particular: evenly *spaced* marks with unevenly *valued* steps
  need a log response, or the endpoints will look right while every
  intermediate mark is wrong.

## Bypass and disengaged states

A disengaged panel is not a different object — it is the same object with the
lamp switched off. That single idea settles what the treatment may and may not
do.

- **It is a lighting change only.** No blur or defocus, no desaturation, no
  flattened or redrawn controls, nothing implying the hardware itself changed.
  Pointers stay exactly where they are; a real panel's knobs don't move when a
  lamp goes out.
- **Apply it as a multiply over the panel**, not an alpha blend toward the
  background colour. Multiplying preserves relative contrast and reads as
  darkness; blending toward the panel colour reads as fog laid over it.
- **Pitch it dark enough to read as off, not merely dimmed.** Chorus-60 lands
  at 0.50; 0.70 read as a dimmer switch rather than a light being out.
- **Don't drain identity colour.** Where a control's colour distinguishes it
  from its neighbours, desaturating removes information rather than signalling
  state.
- **The legibility floor does not apply here.** It exists so the plugin can be
  operated, and this is a state where it deliberately can't be. The job is to
  convey *not usable*, not to stay readable.
- **Don't add a caption explaining the state.** If a panel needs to print
  something like "settings retained", the visual is misleading and should be
  fixed instead.

## Colour system

There is no shared palette — each plugin's fascia is a deliberate choice tied
to its character. What is shared is the rule:

- **Exactly one accent colour per plugin**, reserved for its live-state
  indicators, and used nowhere else on the panel. If it shows up as decoration
  it stops meaning anything.
- **Accents should not be duplicated across the suite** where avoidable. Check
  what siblings already own before picking.
- Everything else stays in a neutral range appropriate to that plugin's base
  material.

## Typography

- **Wordmark**: a bold, characterful display treatment specific to that
  plugin's nameplate metaphor — never a neutral system sans.
- **Function and model line**: restrained, wide letter-spacing, hardware-manual
  voice — but primary identification, so it must be clearly legible, not
  tertiary.
- **LCD/segment face** for anything inside a display — program names, meter
  values, live readouts — visibly distinct from the panel's label typeface.
- **Section and control labels**: neutral, wide letter-spacing, sized below the
  wordmark but fully legible per the Legibility section above.

## Deliverables

Each plugin's design handoff includes:

- The panel design and its `design/` folder (assets, GUI spec, notes)
- Asset format stated explicitly — pre-rendered bitmap filmstrips or
  vector/code-drawn — decided per plugin and matched by the build
- **A product icon**, delivered as part of the main handoff rather than an
  afterthought. Exported at **1024×1024 and 256×256** (JUCE requirement), and
  reviewed at **32px**, which is the size a plugin browser actually renders.
  Icons should reuse *that plugin's own signature element*, not a generic
  category symbol — a plain knob reads as "some audio plugin" and distinguishes
  nothing.

### Before commissioning artwork

Take a read-only parameter inventory from the build first: ID, display name,
minimum, maximum, unit, taper **including the exact skew factor**, default, and
whether the parameter is quantised. Printed scales are drawn against those
numbers, so a wrong range or an unknown skew becomes wrong artwork — and once a
legend is baked into a plate, it is expensive to correct.

"Logarithmic" is not sufficient. The exponent is required before a single mark
can be placed, and a skewed control's marks must be positioned by computed
rotation fraction rather than evenly by value.

### Stating coordinates

The spec's numbers are the contract between the artwork and the build, so a few
of them need saying precisely rather than approximately.

- **A rotary control's centre is the centre of its printed tick arc** — the point
  the needle pivots about — and must be stated as such, not measured off the
  whole control *element*. Where a control name is printed beneath the dial, the
  element's centre sits below the arc's, and a spec quoting that instead puts
  the needle's pivot off-centre in the ring it sweeps. TapeRot shipped 7.27px out
  for exactly this reason; the needle then lands past the printed end mark and
  the knob reads as showing a small negative value at minimum. Give the dial
  centre, the tick arc radius, and the sweep angle explicitly.
- **State the sweep angle in the spec and match the exported filmstrip to it.**
  If prose and artwork disagree, whichever the build trusts will be wrong.
- **Say which controls have states**, not just which are two-state toggles. A
  button that is sometimes disabled, or that relabels itself, needs a sprite for
  every face it wears. A single baked look silently becomes wrong half the time,
  and can't be fixed in code.
- **Say what is baked into the plate and what is drawn at runtime**, and don't
  bake anything carrying a live value. A legend baked with the mock's sample
  numbers overprints the real one and freezes at whatever the mockup happened to
  show.
- Where a parameter's rotation is **non-linear**, the printed tick positions have
  to follow that curve. The build supplies the curve; the ticks are placed under
  it. Evenly spaced marks on a skewed control are wrong by design.

## Roster

Nothing has been released yet — every plugin below is pre-release, and there is
no installed base whose sessions or automation a change could break. That makes
parameter IDs, ranges and naming free to correct until first release; after
that they aren't.

| Plugin | Character | Status |
|---|---|---|
| **TapeRot** | Warm, worn tape degradation | Conformance redesign complete; by-ear Program tuning outstanding |
| **Gatecrasher** | Cold steel rack, gated reverb | Redesign approved; implementation pending |
| **Chorus-60** | Dark synth panel, BBD chorus | Redesign approved; implementation pending |
| **Reflect-84** | Cream fascia, navy bezel, plate/digital reverb | In build; conformance pass outstanding |
| **Fifth Member** | Road-worn touring rack, tempo-synced delay | In build; conformance pass outstanding |
| **Elmer** | Warm grey console module, VCA bus compressor | In build; conformance pass outstanding |

Update this table as castings progress, and revise the note above at first
release.

## Folder structure

```
neon-foundry/
  BRAND.md                 This file
  taperot/
    Source/, Tests/, design/, BUILDING.md, prompts/
  gatecrasher/
  chorus-60/
  reflect-84/
  fifth-member/
  elmer/
  <next-plugin>/
```

Each plugin folder is its own independent git repository with its own
`CLAUDE.md`, scoped to that plugin only. Nothing is created at the
`neon-foundry/` root except this file. Giving Claude Code root-level access
lets it read this file plus any sibling's `design/` and GUI source when making
decisions for a new casting — so new plugins share DNA with what already exists
without repeating instructions from scratch.