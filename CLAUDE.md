# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

REFLECT-84 is its own independent repo and does not depend on `../taperot/`, `../gatecrasher/` or
`../chorus-60/` at runtime or at build time — it is a sibling casting under the shared
[Neon Foundry](../BRAND.md) umbrella, and those repos are read here purely as structural reference
(JUCE/CMake setup, APVTS conventions, DSP folder organization, Tests/ approach, and — from
Gatecrasher — the Program storage architecture, ported rather than redesigned). Read `../BRAND.md`
first for the cross-plugin design system (naming, "Program" not "Preset", the one-accent-colour
rule, component grammar), then this file.

**`design/GUI-SPEC.md` is the authoritative GUI spec** as of the v1.1 conformance pass, and
supersedes `design/README.md` wherever the two disagree — README is the v1.0 document and is kept
for the diff. `design/CHANGELOG.md` states which revision a bundle is; read it first.
`design/screenshots/01-panel.png` is the approved artwork and outranks either doc's prose, on
appearance **and** on geometry — it is 2680 × 1290, exactly 2× the canvas, and is safe to measure
absolute Y off again as of the 2026-08-12 re-cut.

**The canvas is 1340 × 645.13, and 645 is safe as an integer.** That .13 is recorded because we
anchor against it: it comes entirely from the wordmark's `line-height: 0.92` on 42 px — 38.64 px,
the only fractional contributor on the panel — and it sits below the rounding floor at every
supported scale. Knowing its provenance is what makes carrying 645 as an `int` a decision rather
than a rounding someone will later "fix" into a float that infects every derived coordinate.

**For the record, because the shape of it is the lesson:** the 2026-08-11 render was 2680 × 1298,
2× of 1340 × 649, and four pixels taller than the canvas. Exactly one of those four was real — the
body sat 1 px lower because the header band went 33 → 34, a change we had made ourselves — and the
other three were export padding. A row-by-row correlation against the previous render is what
separated them, and without it the honest reading would have been "all four are noise", which would
have been wrong about our own work.

`design/screenshots/header/` holds the five per-state header renders at 3×, which are the acceptance
target for the Program-button lighting — the one part of this design no coordinate settles, and the
reason those renders were asked for. They are 3× of a 1308 × 104 block, **not** of the canvas.

**The bundle is a reference package, not a tree to sync.** Its own `fonts/README.md` says so: it
ships no font binaries, and `CMakeLists.txt` embeds Jost and two IBM Plex Mono faces from
`design/fonts/`, so installing a bundle over `design/` would delete build assets. Copy the documents
and reference renders out; leave the build's own directories alone.

**This casting consumes `neon-foundry-core`, and was the first to.** The pin is a `FetchContent`
line in `CMakeLists.txt` beside the JUCE one — `https://github.com/trudslev/neon-foundry-core.git`
at `GIT_TAG v1.0.0`. All six castings are on it now; core carries shared *behaviour* only, and this
repo takes the whole public surface:

| From core | Stayed here |
|---|---|
| `nf::userProgramDirectory` — the per-OS path and the `Application Support` segment | The `.reflect84program` extension and the 39-character cap |
| `nf::UserProgramStore` — scanning, sorting, naming (`TAKE n`), collision, save, delete | **What a Program contains** — the whole APVTS state and the schema attribute |
| `nf::ParameterSnapshot` — the dirty baseline, keyed by parameter ID and `SpinLock`-guarded | Which events re-capture it: apply, save, session restore |
| `nf::ProgramId` / `nf::ProgramBank` / `programDisplayLabel` | The Factory bank, and resolving a slug to its position |

Core owns files and names; this repo owns meaning. **The collision check that makes "SAVE never
overwrites" true rather than merely unimplemented originated here** and is core's guarantee for all
six now — two sibling castings used to write straight to the composed path, so reusing a name
silently replaced that Program's contents.

One behaviour changed with the move: the empty-name fallback is **`TAKE n`**, not `NEW PROGRAM`.
Six castings had five different fallbacks, and consecutive empty saves now give `TAKE 3`, `TAKE 4`
rather than leaning on `getNonexistentSibling` for `NEW PROGRAM (2)`.

**Company and product stay here**, passed to core as arguments. Core has no defaults for them by
design: a hand-synced copy of one drifted to a dead company name in CHORUS-60 and quietly pointed
saved Programs at a directory nothing reads. The `#error` at the top of `ProgramManager.cpp`
guarantees CMake supplied them.

**Declaring core after JUCE is load-bearing.** Core links `juce::juce_core` and does not fetch its
own when consumed; it fails with an explicit message if JUCE is not already available. Two JUCE
trees in one build link two `juce_core` builds into one binary, and that surfaces as duplicate
symbols a long way from the cause.

**This is not a dependency on a sibling casting**, which stays forbidden. Core is a versioned
library no casting owns, so a change to it forces nothing until this repo moves its own pin — the
same relationship every casting already has with JUCE. See the root `CLAUDE.md` under *Structure*.

## Commands

REFLECT-84 builds on macOS (AU + VST3 + Standalone), Windows (VST3 + Standalone), and Linux
(VST3 + Standalone) — AU is Apple-only. JUCE 8.0.14 is fetched automatically via CMake
`FetchContent` on any platform, no local checkout needed.

Configure once — macOS: `cmake -B build -G Xcode`. Windows: `cmake -B build -A x64`. Linux
(single-config generator, so `CMAKE_BUILD_TYPE` must be set here rather than only at build time):
`cmake -B build -DCMAKE_BUILD_TYPE=Release`. Re-run configure whenever `CMakeLists.txt` changes —
a plain rebuild won't pick up new sources.

Build: `cmake --build build --config Release`. Run the unit tests (JUCE-`UnitTest`-based, console
app target `Reflect84Tests`):
`./build/Tests/Reflect84Tests_artefacts/Release/Reflect84Tests`.

See [BUILDING.md](BUILDING.md) for per-platform requirements and `auval`/pluginval commands.

## Architecture

### Signal chain (fixed order, all in `PluginProcessor::processBlock`)

```
in ──┬─────────────────────────────── dry ──────────────────────────────┐
     │                                                                   │
     └─ PreDelay ─ Input Diffusion ─┐                                     │
                                    │                                     │
        ┌─ Early Reflections ───────┴──────────────┐                      │
        │                                           │                      │
        └─ Late Tank (per-algorithm topology) ─────┤                      │
              ↑                                     │                      │
              └─ feedback: Damping LF → Damping HF → Modulation → GRAIN ──┘
                                                    │
                          Stereo Width ─ Mix ───────┴─ Output Trim ─ out
```

**The grain stage lives inside the tank's feedback path, not on the output.** This is the one
architectural decision everything else hangs off. Vintage digital reverbs got their character from
re-quantizing once per recirculation, so grain accumulates over the tail — early reflections stay
nearly clean and the decay gets progressively coarser as it dies. That is exactly the picture the
TANK LIVE scope draws. Move `GrainStage` to the output and the plugin still makes a noise, but the
display stops telling the truth about it.

`Source/DSP/GrainSpec.h` is the single interpretation of the DIGITAL GRAIN parameter, compiled into
both the processor and the editor. The scope takes `levels` and `stepPx` from it (transcribed
verbatim from `design/README.md` §6); the audio path takes the same `levels`, scaled by the named
`kHeadroom` constant, as its quantizer step count, and `holdRatio` as its sample-and-hold factor.
`kHeadroom` is the one free constant in the mapping — the span of `levels` is fixed at 2.86 bits, and
kHeadroom only chooses where that span sits (currently ~11.4 down to ~8.6 bits). If you change it,
change it there, not in `GrainStage`.

### The four algorithms

Genuinely different networks, not one network with four coefficient tables. Gatecrasher's four tanks
all delegate to a shared `CombAllpassNetwork` differing only in delay lengths, which is precisely
the "Plate with different names" outcome this design brief ruled out.

| Idx | Algorithm | Early reflections | Late tank |
|---|---|---|---|
| 0 | **Plate** (`PlateTank`) | none — 4 series allpasses straight into the tank | Dattorro figure-of-eight, 2 modulated allpasses + 2 delays + in-loop damping per half, 7 output taps |
| 1 | **Digital Room** (`DigitalRoomTank`) | 16 sparse **whole-sample** taps, 20–70 ms, no interpolation | Moorer/Schroeder: 4 parallel damped combs → 2 series allpasses |
| 2 | **Chamber** (`ChamberTank`) | asymmetric, closely spaced, diffused, no gap | FDN-4, Householder mixing |
| 3 | **Hall** (`HallTank`) | wide sparse pattern with a real gap at 45–95 ms | FDN-8, Hadamard mixing, deepest modulation |

Chamber and Hall share `FdnTank` because a chamber and a hall genuinely are the same *kind* of
network at different scales; they differ in order, mixing matrix, delay range, diffusion,
modulation depth and early pattern. Plate and Digital Room are their own topologies.

`Tests/ReverbEngineTests.cpp` asserts the four are measurably distinct on three independent
fingerprints (early echo density, build time, spectral centroid). That test is the guard on the
whole premise — do not weaken it to make a change pass.

**Index order is DSP-canonical (0 Plate, 1 Digital Room, 2 Chamber, 3 Hall) and is NOT the panel's
corner order** — HALL sits bottom-left and CHAMBER bottom-right. `design/README.md` §2 flags this as
a trap. `ReflectTheme::Layout::algorithmCorners` is the single table joining panel position to enum
value; neither is ever derived from the other.

Switching algorithm swaps whole networks, which cannot be click-free by construction. `ReverbEngine`
snapshots what the outgoing tank would have produced for this block's input, swaps, then blends over
~60 ms — Gatecrasher's approach, reused.

### Parameters

`Source/Parameters.h` is the single source of truth for parameter IDs (`ParamIDs::*`), the APVTS
layout, and every value→text mapping (`ParamFormat::*`). `PluginProcessor` caches raw atomic
pointers in its constructor and reads them once at the top of `processBlock` — don't call
`getRawParameterValue` per block, and don't add a parameter without adding both the layout entry and
the cached pointer. New parameters are appended, never inserted.

**All eleven continuous parameters are stored 0-1 normalised**, matching `design/README.md`'s own
state model. This differs from TapeRot and Gatecrasher, which store physical values behind a
`NormalisableRange`. **Two of the mappings are no longer affine** — both damping controls went
logarithmic in v1.1 — so the forms are not interchangeable any more, and the normalised form is kept
for a different reason than it was chosen for: `ParamFormat` is the single place a normalised
position becomes a physical value, and a skew on the range would split that in two. Each
parameter carries a `stringFromValueFunction` built from the same `ParamFormat` helpers the panel
readouts use, so a host's automation lane still reads "40 ms" rather than "0.22".

No DSP class reads the APVTS — the processor converts to physical units and passes plain values
down. `ProgramManager` is the one exception, because APVTS manipulation is its entire job.

### Programs (`Source/DSP/ProgramManager.*`, `FactoryPrograms.h`)

Gatecrasher's architecture, reused directly. Twelve factory Programs; user Programs are one
`.reflect84program` XML file each in a per-OS directory, sorted alphabetically by filename.
Save **always creates a new Program** — naming happens inline on the LCD, there is no dialog
anywhere; Cancel never touches the APVTS; Delete is gated both at the button and inside
`deleteUserProgram`.

Two things here improve on both siblings, deliberately:

- **Name collisions create a distinct file** (`getNonexistentSibling`). Neither sibling checks, so
  saving over an existing name silently replaced that Program's contents — the one way their
  "never overwrites" guarantee could actually be broken.
- **The state schema version is read, not just written**, so a mismatched restore loads the default
  Program instead of producing a silent hybrid. CHORUS-60 fixed this; Gatecrasher still writes a
  version it never checks.

The factory bank's values were authored in panel units and stored normalised; the per-Program
comments in `FactoryPrograms.h` record the authored form, and they are the source of truth rather
than decoration.

**That mattered when the damping taper went log.** Stored normals are only correct relative to a
curve: under the affine mapping a stored 0.5 meant 9 kHz, under `2000 * 8^n` it means 5.66 kHz.
All 24 damping values were re-derived so the twelve Programs still sound as authored; left alone,
damping LF would have roughly halved across the whole bank. `FactoryProgramsTests` asserts the
authored HERTZ round-trip, not the stored normals — a test on the normals would pass through
exactly the change it exists to catch.

**`ParamDefaults` deliberately did NOT get the same treatment.** The defaults keep their normalised
positions and now mean 4.8 kHz / 81 Hz, which is what GUI-SPEC §13 lists and what the reference
render shows. Defaults were authored as positions, the bank was authored in hertz; each keeps its
own authoring intent, which is why the two files' damping numbers no longer look alike.

**Bypass is a parameter but not Program state.** `FactoryProgram` has no field for it and should
never gain one — a Program that recalled "bypassed" would be a Program you cannot hear. The
parameter-count guard states that as a rule rather than a number, so a second non-Program parameter
still fails it. Heaven's 9.0 s decay was clamped to the
control's 8.0 s maximum. The bank has **not** had a by-ear pass — the numbers are deliberate
per-Program choices, but nothing has been listened to.

### GUI (`Source/GUI/`)

Entirely vector/code-drawn — no bitmap assets at all. This is a deliberate divergence from
Gatecrasher and CHORUS-60, which use filmstrip PNGs and a baked background plate; TapeRot's
`drawRotarySlider`/`FailLamp`/`Scope` are the precedent followed here, per `design/README.md`.

`ReflectTheme.h` holds every colour, size, position and typographic constant. Components pull from
`ReflectTheme::Layout` / `::Colour` rather than carrying their own numbers.

**Canvas is 1340 × 645 with a four-column body** as of v1.1, up from 1200 × 615 and three columns.
All three v1.1 screenshots are 2680 × 1290, so that is the artwork's own figure rather than prose.

The fourth column exists because DAMPING was lifted out of the tank column into its own home beside
ALGORITHM. That is what let the damping pair be promoted from the 44px tiny variant to 52px small -
five 10px numerals will not clear a 29px tick radius, and the 10px floor is not negotiable - and the
tiny variant is retired from the panel entirely. Three sizes now: 98 / 60 / 52.

**Column 1's content centre is 158, not its geometric 168.** The rotary and both damping dials
centre there. Both numbers are named in `Layout`; "the column centre" is ambiguous once they differ.

**Dial centres are TICK-ARC centres**, per BRAND.md's "Stating coordinates" - the point the needle
pivots about, not the centre of the control cell. The label sits below the arc, so the cell is not
centred on the pivot; measuring the pivot off the cell is what shipped TapeRot 7.27px out.

Two things that will bite anyone editing layout:

- The design's inner elements are CSS **content-box**, so a declared `width: 336px` plus
  `padding-right: 22px` occupies 358px. Getting that wrong moves every column.
- CSS `font-size` is an em size, which is **not** `juce::Font::withHeight()` (that is
  ascent+descent). Use `ReflectTheme::Font::mono/monoMedium/wordmark`, which go through JUCE 8's
  `withPointHeight()`. Both siblings needed a calibration constant for this; JUCE 8 expresses it
  directly.

`juce::String`'s `const char*` constructor decodes as **Latin-1, not UTF-8**, so a `"\xc2\xb7"`
literal renders as a stray `Â·` on the panel. Use `Text::middleDot()` / `charToString` from a
codepoint for any non-ASCII character.

Scaling is handled once: `PluginEditor::resized()` applies a single uniform transform to the whole
`ReflectEditorContent`, with the constrainer locking the aspect ratio. Every other component draws
in untransformed 1200 × 615 space and has no `resized()` at all.

The TANK LIVE scope's envelope is **synthesised from parameter values**, not measured from audio —
that is what `design/README.md` §6 specifies, and it keeps the display deterministic and drawing
when nothing is playing. The only audio-thread data the GUI reads is three atomics: tank energy (the
lamp), and input/output peak dB. The noise hairlines behind the trace are drawn well below the
doc's quoted alpha; see the comment in `TankScope.cpp` for why the prototype cannot arbitrate that
particular value.

### Build system

`CMakeLists.txt` fetches JUCE via `FetchContent` (pinned to `8.0.14`, matching all three siblings).
`PLUGIN_MANUFACTURER_CODE` (`Nfdy`), `PLUGIN_CODE` (`Rf84`, referencing the design's own "MODEL
RF-84" tagline), `BUNDLE_ID` (`com.neonfoundry.reflect84`) and `COMPANY_NAME` ("Neon Foundry") are
settled, not placeholders — changing them breaks saved projects in both AU and VST3, since JUCE
derives the VST3 class ID from the manufacturer and plugin codes together.

The CMake target is `Reflect84` (targets cannot contain a hyphen) while the product is `REFLECT-84`.
Those are two separate name axes and anything deriving a path from the wrong one silently looks in a
directory that does not exist.

`Tests/` is a separate `juce_add_console_app` that compiles the DSP `.cpp` files directly rather than
linking the plugin target — **a new DSP `.cpp` must be added to both** `target_sources(Reflect84 ...)`
and `target_sources(Reflect84Tests ...)`.

`Tests/TestMain.cpp` creates a `ScopedJuceInitialiser_GUI`. This is not optional: `ProgramManager`
defers through `juce::AsyncUpdater`, and without a MessageManager `triggerAsyncUpdate()` silently
clears its own pending flag, so any test of that path passes while proving nothing.

Plain stereo in/out, no sidechain bus.

### The Program list's group caption

**22px, and it is the designers' own figure from GUI-SPEC §9 — not derived from the 26px row.**

This casting is the one that does **not** go through `nf::captionHeight`. Its list is a
`juce::Component` (`ReflectProgramList`), not a `PopupMenu`, so there is no `MenuMetrics` to fill in
and the caption is a row kind carrying its own height in `ReflectProgramList::captionHeight`.

**The suite rule still binds, and this satisfies it the stronger way.** The rule is that a caption is
sized from its own type plus padding and never derived from the row — `nf::captionHeight (font,
topPad, bottomPad)` is how the four `PopupMenu` castings compute one, with 3/4 as the suite's adopted
default padding where no design states one. Here a design *does* state one, so the authored figure
wins over any computed default; that is the same precedence, not an exception to it. What the rule
rules out is 26 × 1.5 = 39, JUCE's inherited `rowHeight + rowHeight / 2`, which four castings had by
omission and which makes a caption half again *taller* than a row.

Reflect-84 and Elmer are the suite's only two designer-authored caption paddings, and they disagree
— 9/4 here against Elmer's 3/4. That disagreement is why 3/4 is recorded as *adopted* rather than
*derived*: there was no suite constant to derive one from.

## Status

- **DSP**: all four topologies implemented, no stubs. Delay lengths, diffusion coefficients and
  early-reflection tables are a structurally-reasoned first pass rather than a tuned one — build,
  load, listen, adjust. Tests cover RT60 tracking, stability at maximum settings, algorithm
  distinctness, switch continuity, and CPU (~0.3–0.5% of the real-time budget per algorithm).
- **GUI**: conformant to `GUI-SPEC.md` v1.1 and verified against `01-panel.png` region by region.
  Printed scales replace the standing readouts as the only at-rest value reference; live values
  appear in the PROGRAM LCD while a control is moved and nowhere else; the scope clamps its trace to
  the plot region rather than the screen; the panel has a bypass state.

  **The takeover is `nf::describeParameter`** with the deadline in `nf::ReadoutTimer`, reverting
  900 ms after release — the value this panel already used, now single-sourced in
  `nf::ReadoutFormat::revertMs`. `ReflectTheme::Layout::readoutFormat()` holds the spelling, not
  `ProgramHeader`, because `ProgramHeader.h` reaches the processor and its `JucePlugin_*` macros
  exist only in the plugin target — so a test reading the format from there cannot link, and one
  declaring its own copy would assert against itself.

  **Nothing is re-cased, and this casting is why the flag that used to do it was named
  `ValueCase::all`.** The parameters here bake their unit into the value text (`ParamFormat`'s
  `dampHFText`, `decayText`, `trimText`), so upper-casing it printed `DAMPING HF: 4.8 KHZ`,
  `DECAY: 4.6 S` and `OUTPUT TRIM: +2.5 DB`. A capital S is a different unit from a lowercase one
  and KHZ is not a unit at all. `readoutDefects` passes the baked-unit arrangement deliberately: the
  label is empty, so nothing doubles.

  `ValueCase` is gone from core as of 2026-08-13 — case belongs at the source, never at a display
  site — and the parameter **name** is no longer upper-cased either. **The caps re-authoring in
  `Parameters.h` that ruling requires is still outstanding here**; see the root `../CLAUDE.md` under
  "Case belongs at the source".

  `enterNamingMode` now cancels the takeover rather than letting paint order hide it — hidden, it
  returned the moment naming ended if the revert had not yet fired.

  **Ticks sit at the labelled values, stored as ROTATION FRACTIONS.** A mark's angle is
  `-135 + f * 270` with no inverse mapping in the drawing code, so a taper change moves the ring
  with the pointer instead of leaving it pointing where the pointer never goes.

  **Two spec deviations are deliberate and must not be "fixed"**, both recorded beside the code:
  SAVE stays gated on modification (§9 says "never disabled", but that predates a decision taken
  here), and the dropdown opens flush to the LCD rather than §9's 4px below, because the whole suite
  was changed to flush and the root `CLAUDE.md` carries that as the shared contract.

  **The header row is one 34px band and the Program buttons carry two legends each.** Every part —
  LCD, SAVE, DELETE, IN and OUT — is 34px *border-box*, which is the suite's figure from BRAND.md
  rather than this panel's: the castings are differently-sized units, not scales of one design, and
  a manufacturer uses the same physical part across a product line. `meterWellH` and `headerButtonH`
  follow `programWellH` rather than repeating 34, because they are one decision.

  The whole row was measured off `screenshots/header/04-user-edited-save-delete-lit.png` at 3×, and
  it closes on itself: 357 + 641 = 998, +8 → SAVE 1006 + 62 = 1068, +8 → DELETE 1076 + 70 = 1146,
  +16 → IN at **1162**, which is where the render puts the meter well to the pixel. A chain landing
  on an independently measured edge is the check that the row is right rather than merely plausible.

  **The buttons never relabel and have no disabled face.** SAVE above STORE, DELETE above CANCEL,
  permanently printed, on one dark face in every state; only the legends' illumination changes. The
  brass cap and the separate disabled face are both gone and neither should return — a printed panel
  legend cannot rewrite itself, and a pale cap leaves lit type with no headroom to read as lit. Each
  legend's `lit` flag is exactly its region's `isRegionEnabled`, so a glowing legend cannot sit on
  something that will ignore the click.

  The bloom is eight offset copies per radius at three radii, not the spec's five: JUCE has no
  text-shadow, and the 18px and 30px layers would wash a 34px face entirely and spill onto the
  bezel. Alphas are tuned against the render rather than taken from the spec, because eight
  overlapping copies at alpha *a* reach 1−(1−*a*)⁸ where they coincide.

  **The name cap grew 35 → 39** and that direction is the rule: a budget may grow, a cap may never
  shrink. The LCD widened 586 → 641 because the second legend cost no button width and the gaps
  either side came in 10px → 8px. Lowering the cap would orphan names already on disk — they would
  load and then fail to save back under their own name. `DisplayBudgetTests` measures the budget
  from the drawn font each run and asserts the cap is the budget less the larger of the marker and
  the cursor, so the pair cannot drift apart.

  **Verify by clicking, and composite at true 1:1.** The composite against `01-panel.png` is what
  found SAVE/DELETE drawn under the meter wells and the ALGORITHM caption pushed off its column;
  the click pass is what found the bank indicator still a bordered badge, the USER group hidden when
  empty, and the menu 35px over its height cap. None of that shows in a build log.
- **Program bank**: twelve Programs with authored values, no by-ear pass yet.
- **Icon**: `design/icon/` holds a hand-tuned optical ramp — three cuts, not one artwork scaled, with
  the 32/64 px versions dropping the glow because it turns to mush below 48 px. `ICON_BIG`/`ICON_SMALL`
  point at the 1024 and 256 px cuts and JUCE generates the `.icns` from those two; don't hand-maintain
  an icon bundle. JUCE embeds only the two sizes it is given, so macOS downscales the 256 px cut for
  16/32 px display rather than using the design's small cut. **Settled: leave it.** Closing it would
  need a post-build `iconutil` step over all five PNGs — macOS-only, and it fights the generator.
  Don't re-raise this.
- **Not yet done**: registration in `../manifest/suite.toml`, which needs a public repo and a tagged
  release first, plus a freshly generated `windows_appid` GUID. Nothing has been committed yet.
