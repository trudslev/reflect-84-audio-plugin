# REFLECT-84 — GUI SPEC

Model **RF-84**, stereo reverb processor. Neon Foundry casting, harmonisation round.
Authoritative for the build; supersedes the v1.1 spec wherever the two disagree.

**Read `shared/HEADER-PART.md` first.** The block, the band, the LCD cell with its budget and
cap, the Program buttons and their state matrix, and the meter wells are the shared part and
are not restated except where this casting's material meets them.

**Asset format: fully code-drawn. No plate, no filmstrips, no bitmap of any panel element**,
and 0.36 MB of fonts. **Call 6 permits this explicitly** and it should stay this way unless the
fascia grows texture that wants baking — it has not. Reflect-84 is the only casting in the
suite with no artwork in its bundle at all besides its icon.

---

## 1 · Canvas

| Figure | Value |
|---|---|
| Canvas | **1340 × 648**, a pinned figure |
| Fascia | `linear-gradient(#efe6d0, #e2d8bd 60%, #d8cdb0)`, radius 10, `inset 0 1px 0 rgba(255,255,255,.75)`, `inset 0 -2px 6px rgba(90,70,40,.18)` |
| Texture | 1 px scanline `rgba(80,60,25,.035)` every 3 px + `radial-gradient(120% 90% at 20% 0%, rgba(255,255,255,.5), transparent 60%)`, full bleed, non-interactive |
| Header block | 16, 16, 1308 × 104 — shared part, material `linear-gradient(#22304c, #1a2740 55%, #142036)` |
| Body | y **120**, height 512, padding 14 / 0 / 12 |
| Column dividers | 1 px vertical gradient, `rgba(120,98,55,.4)` at 20–80 %, with `1px 0 0 rgba(255,255,255,.65)` |

**Reflect-84 gained no width this round** — it was already 1340 and the shared header is
derived from its band. **The canvas height is now a fixed 648**, which closes the round's one
real defect in this casting: the panel previously measured 645.13 before webfonts applied and
648.63 after, because three text blocks had no pinned `line-height`. Call 4 makes every size a
pair, so the two-height render is no longer possible.

### Columns

| Column | x | Width | Contents |
|---|---|---|---|
| 1 | 18 | 300 | ALGORITHM Ø104 · DAMPING HF / LF |
| 2 | 319 | 290 | REVERB TANK — SIZE, DECAY, PRE-DELAY, DENSITY |
| 3 | 610 | 535 | TANK LIVE scope · CHARACTER — MODULATION, DIGITAL GRAIN |
| 4 | 1146 | 176 | OUTPUT — STEREO WIDTH, MIX, OUTPUT TRIM |

---

## 2 · Knobs — three classes

| Class | Ø | Controls | Cell | Numerals |
|---|---|---|---|---|
| Signature | **104** | ALGORITHM only | 104 × 104 + corner labels | none — position names |
| Primary | **76** | MODULATION, DIGITAL GRAIN | 150 × 142, C 75 / 72 | up to **five** |
| Standard | **56** | SIZE, DECAY, PRE-DELAY, DENSITY, DAMPING HF, DAMPING LF, STEREO WIDTH, MIX, OUTPUT TRIM | 120 × 122, C 60 / 58 | **three** |

Sweep 270°, angle = `−135 + 270 f`, 0° pointer up. Ticks: major **2 × 9** at every numeralled
position, minor **1.5 × 5** at the rest, ink `#7a6b4c`. Tick arc starts at **body radius + 8** — 46
primary, 36 standard. Numeral radius **R 67 (primary) / 57 (standard)**, numerals centred on their own
box.

**Both radii now come off the catalogue's clearance chain, and the previously stated 64 / 52 could not.**
The chain is `numeral ring = r + 8 + 9 + 6 + ½ line box` — tick ink 8 px outside the body, 9 px major
tick, numerals 6 px clear of its outer end, half a numeral line box. With this casting's **12 px**
numeral line box that is `r + 29`: **38 + 29 = 67** and **28 + 29 = 57**.

**Why 64 / 52 was structurally impossible, not merely off.** An additive chain preserves differences:
the bodies are **10** apart and those radii were **12** apart, so `52 = 28 + a + 9 + b` needs
`a + b = 15` while `64 = 38 + a + 9 + b` needs `17`. **No constants satisfy both**, so 64 / 52 could
only ever be transcribed from this spec — and inverting the chain out of them put the ticks' inner ends
**5 px and 3 px** off their bodies instead of 8, differently for each class. **67 / 57 is the only pair
where the body clearance comes out at 8 for both classes at once**, which is what identifies it as the
intended pair rather than a preferred one: this casting's bodies (Ø76 / Ø56) and tick lengths are
Gatecrasher's exactly, and Gatecrasher reproduces at 67.5 / 57.5 on a 13 px line box. **Identical
bodies with identical ticks cannot have different clearances.**

Cost: printed numerals move **3 px outward on the primaries and 5 px on the standard class** — this
casting only, and an intermediate proposal of 64 / 54 is withdrawn with it, since it kept 64 and so
kept a clearance of 26 that the catalogue does not state. **A transcribed figure drifts; a derived one
cannot.**

Body fills: primary `radial-gradient(circle at 50% 22%, #fdf6e0, #ddcb98 52%, #b09a61 76%, #7d6a3b)`
with an inner cap at `inset: 12px`; standard
`radial-gradient(circle at 50% 24%, #f9f1d8, #d4c18e 58%, #9f8a55 82%, #77653c)`. Pointers
3 × 30 % `#2f2617` and 2 × 34 % `#33291a`.

**No mixed-class row exists** — Ø76 sits alone in column 3 and the Ø56 knobs are grouped by
column — so the registration rule does not arise. Stated because a call that cannot apply
should say so rather than be silent.

### 2.1 Standard class dropped from five numerals to three, and this reversed a prior decision

The v1.1 spec **promoted** the damping pair from Ø44 to Ø52 precisely so both could keep their
full mark sets (four for HF, five for LF) rather than dropping to three. **Call 3 outranks
that**: at Ø56 the ceiling is three, and the retired values keep their ticks as minors, so the
resolution is carried without the numerals. Recorded because the earlier reasoning was sound
and is now void — the promotion still happened, its stated purpose no longer applies.

| Knob | Marks (f · printed) | Minors | Unit |
|---|---|---|---|
| SIZE | 0 **0.2** · .5 **0.6** · 1 **1.0** | .25, .75 | — |
| DECAY | 0 **0.4** · .4737 **4** · 1 **8** | .2105, .7368 | s |
| PRE-DELAY | 0 **0** · .5 **90** · 1 **180** | .25, .75 | ms |
| DENSITY | 0 **0** · .5 **50** · 1 **100** | .25, .75 | % |
| DAMPING HF | 0 **2** · .6667 **8** · 1 **16** | .3333, .8333 | kHz |
| DAMPING LF | 0 **40** · .5489 **160** · 1 **500** | .2744, .8233 | Hz |
| STEREO WIDTH | 0 **0** · .5 **100** · 1 **200** | .25, .75 | % |
| MIX | 0 **0** · .5 **50** · 1 **100** | .25, .75 | % |
| OUTPUT TRIM | 0 **−12** · .5 **0** · 1 **+12** | .25, .75 | dB |
| MODULATION · DIGITAL GRAIN | even fifths, **0 / 25 / 50 / 75 / 100** | .125, .375, .625, .875 | % / — |

**DECAY is linear in seconds and DAMPING is logarithmic**, so neither's marks are evenly
spaced. `f = log(v / min) / log(max / min)`; HF is `2 · 8^f`, LF is `40 · 12.5^f`. **The
build's taper and the printed marks must agree exactly** — if the DSP ships anything else, the
artwork is wrong, not the scale. Units print inside the arc's bottom gap; `−` is U+2212.

### 2.2 ALGORITHM

Ø104, **four detents, no continuous travel**, pointer angle `−45 + index × 90`. Body
`radial-gradient(circle at 50% 26%, #1f2b44, #16223a 52%, #0d1526 80%, #070c15)` — **the one
dark cap on a cream fascia**, which is what marks the signature control by material as well as
diameter. Pointer 3 × 36 % `linear-gradient(#f4e8c4, #cdb989)`, **12.98:1** on the cap.

**Ticks: 2 × 9 at radius 65, each centred exactly on its detent angle** — −45, +45, +135, +225.
The tick's *centre* is the contract, not its leading edge; the v1.0 artwork specified
`from 224.45°`, which is the leading edge of a 1.1° wedge, and reading it as a centre drew every
tick 0.55° counter-clockwise of the detent it marks.

| Label | Index | Corner | Angle |
|---|---|---|---|
| PLATE | 0 | top-left | −45° |
| DIGITAL ROOM | 1 | top-right | +45° |
| CHAMBER | 2 | bottom-right | +135° |
| HALL | 3 | bottom-left | +225° |

**The corner ordering is deliberate and is not clockwise in index order** — the two digital
algorithms sit right, the two physical ones left. Do not "correct" it. Selection is carried by
**weight and value both**: 700 / `#332b1e` selected against 500 / `#5e5440` unselected.

---

## 3 · TANK LIVE scope

The plugin's one signature live display and its only LED.

| Element | Spec |
|---|---|
| LED | Ø15, `radial-gradient(circle at 40% 32%, accent+white, accent 70%, #2a3a24)`, glow `0 0 14px 3px accent@45%` |
| Bezel | 491 × 182, padding 6, `linear-gradient(#c9bd9c, #b8aa87)`, radius 4 |
| Screen | 477 × 168 content, `linear-gradient(#080d16, #050810)`, `inset 0 3px 12px rgba(0,0,0,.85)` |
| Drawing space | `viewBox 0 0 600 168`, `preserveAspectRatio: none` — **x scales 0.795**, y 1.0 |

### The screen and the plot region are two rectangles

| Rectangle | x | y | w | h |
|---|---|---|---|---|
| Screen | 0 | 0 | 600 | 168 |
| Title strip (reserved) | 0 | 0 | 600 | 20 |
| **Plot region** | 0 | 20 | **520** | 148 |
| Level gutter (reserved) | 520 | 20 | 80 | 148 |

**The trace is clamped to the plot region, not to the screen.** 0 dB is y 26, −60 dB is y 156,
full-scale height 130; the trace touches both and is clipped at x 520. **Do not substitute a
top or bottom margin for the gutter** — 0 dB and −60 dB are the levels being annotated, so the
trace has to reach them and the separation must be horizontal. A build that reads this as one
rectangle and clamps to the screen reintroduces the collision the gutter exists to prevent: at
200 ms/DIV a long decay settles near the baseline, exactly where `−60 dB` sat.

Legends, all 10 px / 12 / .18 em `#8ea0bc`, **7.54:1**, none in the plot region: `DCY ENV`
title-strip left; `GRAIN <n> · <levels> STEP` or `GRAIN OFF · SMOOTH` title-strip right;
`0 dB` and `−60 dB` in the gutter at x 532, with **12 px leader ticks** at
`rgba(142,160,188,.85)` tying each label to its level. Reservation hairlines
`rgba(120,160,200,.20)` under the strip and down the gutter's left edge.

Grid `rgba(120,160,200,.10)` **inside the plot region only** — 11 verticals every 200 ms,
3 horizontals at quarter-points.

**Drawing model:** seeded noise tail (240 hairlines, seeded once at construction, never
per frame) · envelope `exp(−t / (τ×1.6))` with `τ = decay / 6`, modulated and density-scaled ·
**grain quantisation to `max(3, round(30 − grain×26))` steps with a `3 + grain×24` px hold** —
this stair-step is the visual payoff of the plugin and must survive implementation · sweep clip
advancing 0.016 per ~40 ms tick, *in addition to* the plot-region clamp · accent fill at 16 % ·
stroke twice, 2.6 px at 75 % through a `stdDeviation 3.2` bloom then a crisp 1.6 px.

---

## 4 · The Program list — a Component, and the casting's one maintenance hazard

**Not a PopupMenu.** The list is the display continuing downward: it hangs flush off the LCD's
bottom edge, at the LCD's width, and **runs to the panel's bottom edge** in the **display's own
glass**, not the header's navy. No gap under it — the suite contract, `shared/HEADER-PART.md` §12.

| Figure | Value |
|---|---|
| Surface | `linear-gradient(#0a0f18, #070c14 45%, #05080e)`, **no top border, no top radius** |
| Sides | 1 px `rgba(0,0,0,.7)` left and right only, `inset 3px 0 7px rgba(0,0,0,.55)` each side |
| Scanline | `repeating-linear-gradient(0deg, rgba(198,222,255,.028) 0 1px, transparent 1px 3px)` |
| **Height** | **553 px** = **648 − 95** — canvas minus the LCD's bottom edge (61 + 34). A measurement, not a max-height. **The earlier 537 is retired**: it needed a 16 px bottom margin no document states, and the one-pixel canvas change it was attributed to cannot move a list seventeen. The old **554 = 649 − 95** reproduced the same method exactly, which is what proved the method was never in dispute |
| Rows | item **26** · caption **22** · separator **9** · placeholder **26** · chevron band **20** |
| Text inset | **16 px** left — the LCD's own inset, so the list reads as one column with the display |

**Row heights are pinned and never grow to the host's standard.** Order: `INIT` (unnumbered) ·
9 px separator · `FACTORY` caption · factory items · `USER` caption · user items or
`— none saved —`. The USER group is never hidden.

**Current-Program marker: a 3 px lit bar at the row's left edge**, `#f2e6c2` with
`0 0 8px rgba(242,230,194,.65)`, the row's field lifting to `rgba(120,160,200,.09)` and its text
to `#fdf7e6` — **17.93:1**. Not a tick glyph and not a gutter: a tick costs a character cell on
every row to serve one, and JUCE's own tick is the most OS-looking mark available.

**Scroll chevrons, no scrollbar in any state.** 20 px bands, opaque, with the list inset 20 px
top and bottom so no row passes under them. One click scrolls **104 px — four item rows**;
the wheel scrolls by platform delta; both clamp. At the end of travel a chevron **steps back to
`#8ea0bc` rather than disappearing** — 7.55:1, above the state floor.

**The list height this round: 554 → 553**, one pixel, because the canvas is now a pinned 648. With
twelve factory Programs the fixed rows come to 391 px, so **scrolling begins at the seventh User
Program**, where it began before — the row count does not move on one pixel. **An earlier draft said
554 → 537 and put it down to the pinned canvas**; a one-pixel canvas cannot move a list seventeen, and
537 required a bottom margin no document states. Retired.

### 4.1 The maintenance note — read this before changing anything shared

**Because the list is a Component rather than a PopupMenu, nothing carries the shared
look-and-feel into it.** The chevron construction, the ink states and the lit bar are drawn here
by hand. A change made centrally — to the chevron glyph, to an ink, to a state colour —
**propagates to five castings and silently not to this one.** That is exactly what happened to
the chevron this round: nine sites drew a rotated box and one drew the path, and this list was
one of the sites that had to be edited by hand.

`shared/HEADER-PART.md` §10 enumerates the propagation sites for this reason. **This list is
inside site 4 and is not separately visible in that table** — when a shared change lands, this
component needs checking on its own, not as part of the panel.

---

## 5 · Palette and measured contrast

Computed in one pass from this panel's own hexes against each ground **by name**. Fascia figures
are against `#d8cdb0`, the **bottom** of the gradient, not the `#efe6d0` top. Header figures are
against `#22304c`, the **lightest** point of the bezel, since that is the worst case for pale
ink. Functional 7:1, flavour 4.5:1, state 3:1.

### On fascia (worst `#d8cdb0`)

| Ink | Role | Ratio | Class |
|---|---|---|---|
| `#332b1e` | TANK LIVE, CHARACTER labels, selected algorithm label | **8.82** | functional |
| `#3e3527` | control labels, printed scales, units, ALGORITHM caption, scope header data | **7.62** | functional |
| `#5e5440` | unselected algorithm labels, version stamp | **4.71** | flavour |
| `#7a6b4c` | tick marks | 3.29 | **graphic — no text floor** |

The previous `#4a4132` (6.34) and `#5c5241` failed the functional floor and are retired. The
previous unselected-algorithm `#9a8e74` measured **2.10** — the worst offender on the old panel.
Unselected labels stay dimmer than selected on purpose, as redundant encoding alongside the
pointer, but they now clear the flavour floor and the distinction is carried by **weight as well
as value**.

### On the navy block (worst, i.e. lightest, `#22304c`)

| Ink | Role | Ratio |
|---|---|---|
| `#f0e2ba` | wordmark | **10.21** |
| `#b7c2d8` | function descriptor, model line | **7.35** |
| `#d8c18a` | PROGRAM caption, chevron | **7.47** |
| `#d8c18a` | section pill text, on the pill's `#16223a` | **9.00** |

**The descriptor and model line were `#a9b6cd` at 6.44 against this point and are now
`#b7c2d8`** — the one outstanding contrast item carried over from v1.1, now closed. The earlier
spec quoted 7.96 for `#a9b6cd`, which was true against the bezel's *darkest* point and not
against the point the text sits on.

### On LCD glass, list glass and the scope

| Ink | Role | Ratio |
|---|---|---|
| `#f2e6c2` | program name, bank tag, live readout, list rows | **15.93** LCD · **15.42** list |
| `#fdf7e6` | current list row | **17.93** |
| `#e8dcba` | IN / OUT meter values | **14.52** |
| `#8ea0bc` | scope legends, list placeholder, stepped-back chevrons | **7.54** scope · **7.55** list |
| `#5ce07a` | decay trace | 11.82 — graphic |

Scope legends were `rgba(190,205,225,.4–.5)` — opacity-driven hierarchy, which the brand
forbids. They are solid at one value now, with hierarchy carried by position.

### Pointers against their caps

| Cap | Pointer | Ratio |
|---|---|---|
| Primary `#ddcb98` | `#2f2617` | **9.27** |
| Standard `#d4c18e` | `#33291a` | **8.02** |
| Signature `#16223a` | `#f4e8c4` | **12.98** |

### On the Program cap (`#23282c → #14181b`)

| Ink | State | Ratio |
|---|---|---|
| `#f4f8fa` | lit | **13.93** light end · **16.71** dark end |
| `#9aa1a6` | idle | **5.68** light end · **6.82** dark end |

### Accent

**One accent: `#5ce07a`** (phosphor green). The TANK LIVE LED, its glow, and the decay trace
with its 16 % fill. Nowhere else — not on knobs, labels, buttons or dividers. Alternates
reserved and unused: `#ffb02e`, `#4ec9ff`, `#ff5c5c`.

---

## 6 · Readout formats

The LCD is the only place a live numeric value appears. While a control is moved it shows
`<NAME>: <value>` in the LCD face, reverting to the Program name **900 ms after release**.
**Only direct user manipulation triggers it** — host automation must not, and there are no
tooltips or floating bubbles.

| Parameter | Format | Example |
|---|---|---|
| SIZE | 2 dp, no unit | `0.71` |
| DECAY | 1 dp + ` s` | `4.8 s` |
| PRE-DELAY | integer + ` ms` | `40 ms` |
| DENSITY · MODULATION · MIX · STEREO WIDTH | integer + ` %` | `72 %` |
| DAMPING HF | 1 dp + ` kHz` | `7.9 kHz` |
| DAMPING LF | integer + ` Hz` | `169 Hz` |
| DIGITAL GRAIN | integer, no unit | `46` |
| OUTPUT TRIM | **explicit sign** + 1 dp + ` dB` | `+0.0 dB`, `−6.0 dB` |
| ALGORITHM | name | `PLATE` |

Parameter names are the printed control names in full. Minus is U+2212 in both scales and
readouts.

---

## 7 · State matrices

### 7.1 Program legends — shared part

| Panel state | SAVE | STORE | DELETE | CANCEL |
|---|---|---|---|---|
| Factory Program, unmodified | idle | idle | idle | idle |
| Factory Program, edited | **lit** | idle | idle | idle |
| User Program, unmodified | idle | idle | **lit** | idle |
| User Program, edited | **lit** | idle | **lit** | idle |
| Naming a Program | idle | **lit** | idle | **lit** |

**Esc out of naming leaves the Program edited, because nothing was stored.** Enter commits,
clears the flag and lands on *User Program, unmodified*. Naming suppresses the rest of the
header: the LCD is an entry field, so clicking it does not open the list, and SAVE and DELETE
are inert even where they would otherwise be live. The dirty flag drives SAVE's lamp **and**
the LCD's trailing ` *` from one source, so they cannot disagree.

### 7.2 Name entry

Seed is the current name less its factory index and any numeric suffix, plus the lowest free
suffix — `01 RAIN ALL DAY` → `RAIN ALL DAY 2`. The field is the program name's own centred slot
at the same size and glow, **not a left-aligned input**, so nothing in the band shifts when
naming opens. Caret is a solid block `▌`, **steady, not blinking** — a blink makes the five
state renders undiffable and buys nothing. **Cap 47**, enforced on input; upper-cased as typed;
committing empty falls back to the seed.

### 7.3 List

| State | Treatment |
|---|---|
| Current Program | 3 px lit bar, field lifted, text `#fdf7e6` |
| Hover, selectable | `rgba(120,160,200,.10)` |
| Hover, current | unchanged — already lifted |
| Captions, placeholder | no hover response |
| Chevron, travel available | `#d8c18a` + `drop-shadow(0 0 4px rgba(216,193,138,.55))` |
| Chevron, at end | `#8ea0bc`, no glow — **not hidden, not removed** |

### 7.4 Bypass

Host-driven, no on-panel control. **0.50 `#808080` multiply**, pointers unmoved, accent not
drained — the LED and trace darken with everything else. No caption. The legibility floors do
not apply.

**Pending amendment:** `shared/BRAND-AMENDMENT-BYPASS.md` proposes the multiply cover only what
is disengaged — the header never dims and **the scope never dims**. If adopted, this casting
takes two one-line changes: the multiply moves off the header block, and the TANK LIVE scope
leaves it with its trace holding flat.

---

## 8 · Type

Every size is a CSS px em size with a pinned line box (call 4). **This is the casting where
that call bites**: three blocks previously had no pinned line-height and the canvas measured
645.13 or 648.63 depending on font-load state.

| Role | Face | Size / line box | Tracking | Ink |
|---|---|---|---|---|
| Wordmark | Jost 500 | 42 / 40 | .10 em | `#f0e2ba` |
| Function descriptor | Barlow Condensed 600 | 11 / 13 | .30 em | `#b7c2d8` |
| Model line | IBM Plex Mono | 11 / 13 | .30 em | `#b7c2d8` |
| PROGRAM caption | Barlow Condensed 600 | 10 / 13 | .24 em | `#d8c18a` |
| Section pill | Barlow Condensed 600 | 11 / 13 | .24 em | `#d8c18a` |
| Control label — primary | Barlow Condensed 600 | 12 / 14 | .20 em | `#332b1e` |
| Control label — standard | Barlow Condensed 600 | 11 / 14 | .16 em | `#3e3527` |
| ALGORITHM caption · corner labels | Barlow Condensed 600 / 700 | 11 / 13 | .26 / .16 em | `#3e3527` · see 2.2 |
| TANK LIVE | Barlow Condensed 600 | 12 / 14 | .26 em | `#332b1e` |
| Scope header data | Barlow Condensed 600 | 10 / 13 | .18 em | `#3e3527` |
| Printed numeral · unit | IBM Plex Mono | 10 / 12 | .02 / .14 em | `#3e3527` |
| Scope legend | IBM Plex Mono | 10 / 12 | .18 em | `#8ea0bc` |
| LCD · meter value · list item | Share Tech Mono | 17 / 20 · 13 / 16 | .10 em | `#f2e6c2` / `#e8dcba` |
| Program legend | Barlow Condensed 600 | 11 / 13 | .12 em | see 7.1 |
| Version stamp | Barlow Condensed 600 | 10 / 13 | .10 em | `#5e5440` |

**Control labels of the two size classes share a line box of 14** so they bottom-align wherever
they meet. The wordmark stays **Jost** — nameplate metaphor, outside call 7. **Printed numerals,
units and the scope's on-glass legends stay IBM Plex Mono**, this casting's own mono, per call
7's split. If Jost is unavailable, substitute a geometric grotesque with a tall x-height —
never a neutral system sans.

---

## 9 · Conformance — calls this casting already satisfied

**§9 and §10 together account for every call.** A call appearing in neither this section nor the
changelog is a gap by construction, not an omission.

| Call | State |
|---|---|
| **1** — 1340 frame | **already conformed.** Reflect-84 was already 1340 and is the casting the shared header's band was derived from; nothing moved for width. |
| **3's signature class** | **already conformed.** ALGORITHM is a four-detent selector at Ø104 with the only dark cap on the panel — material as well as diameter. |
| **5** — code-drawn, cached, no filmstrips | **already conformed.** Ticks and numerals were always placed from rotation fractions; there were no sheets to retire. `setBufferedToImage` is the build's to add. |
| **6** — plates export at 3× | **checked, and it stays plateless.** The fascia is a gradient plus a procedural scanline and corner wash; nothing wants baking, so call 6's per-casting permission applies. If texture is added it becomes a plate and the call binds. |
| **7's split** | **already conformed.** Panel lettering was Barlow Condensed; the wordmark is the nameplate metaphor; numerals, units and screen legends stay in the casting's own mono. |
| **§4B shoes** | **not applicable, checked.** No two- or three-state shoe exists on this panel — ALGORITHM is a detented selector and the Program list is a Component. |
| **Registration** | **cannot apply** — no mixed-class row exists (§2). |
| **List construction** | **the catalogue agrees with this panel rather than the reverse.** The 26 px rows, 22 px caption, 9 px separator, 3 px lit bar and 20 px chevron bands are the only list in the suite built and verified against a running panel, and they are carried unchanged. |
| **Lamps** | **already conformed** — one LED, light stopping at the lens edge. |

---

## 10 · Changelog and outstanding

### This round

1. **Canvas height pinned at 648** (call 4), closing the 645 / 649 font-load split. Width
   unchanged at 1340.
2. **LCD to Share Tech Mono 17 / .10 em** (call 2) in the shared 641 cell; bank cell 72,
   chevron trim 30 with a 16 px inset. Budget 44 → **49**, cap 42 → **47**.
3. **Ø98 → Ø76 primary; Ø60 and Ø52 → Ø56 standard** (call 3). ALGORITHM keeps Ø104.
4. **Standard-class numerals cut to three**, reversing v1.1 §2's promotion rationale (§2.1);
   demoted values keep their ticks as minors.
5. **Program buttons to the shared dark cap** `#23282c → #14181b` with 11 / 13 / .12 em legends,
   replacing this casting's navy face and five-layer bloom.
6. **Meter wells 76 → 64**, captions to 10 / 13 / .28 em, `dB` unit added.
7. **Panel lettering to Barlow Condensed 600** (call 7) — labels, pills, captions, TANK LIVE,
   scope header, corner labels; corner-label selection now weight 700 / 500.
8. **Descriptor and model line `#a9b6cd` → `#b7c2d8`**, 6.44 → 7.35 against the bezel's lightest
   point, closing v1.1's one outstanding contrast item.
9. **Chevron re-drawn as the shared 14 × 8 path at all three sites** — the LCD mark and both
   scroll chevrons. The up chevron is the path **mirrored, not rotated**: a rotated V puts its
   round caps on the wrong axis.
10. **List height 554 → 553**, one pixel, tracking the pinned canvas; the User-Program scroll
    threshold is unchanged.
11. **Meters idle at `−99.0`**, matching the build's display floor.
12. **Numeral radii 64 / 52 → 67 / 57** (§2), off the catalogue's clearance chain `r + 8 + 9 + 6 + ½
    line box` at this casting's 12 px line box. 64 / 52 were 12 apart on bodies 10 apart, which no
    additive chain can produce; 67 / 57 is the only pair clearing both bodies by the catalogue's 8.
    Primaries move 3 px outward, standard 5 px. Tick arcs start at 46 / 36.
13. **List height 537 → 553** = `648 − 95`, restoring the stated method (§4). 537 needed a 16 px bottom
    margin no document states, and a one-pixel canvas change cannot move a list seventeen. A proposed
    contract change making that margin the fascia inset in all six is **withdrawn** —
    `HEADER-PART.md` §12 records why: nobody ever wrote 16, and a derivation found after the fact to
    explain a figure is a reconstruction rather than its base.
14. **A retired claim:** the v1.1 spec said the list's chevrons were "the LCD's caret rotated."
    The caret is a block `▌` and always was — the sentence described a construction that existed
    on neither side. Recorded here rather than deleted silently, because a removed sentence
    leaves no trace of why.

### Outstanding

- Fine-drag (shift ×0.25) and double-click-to-default are specified and in the panel; confirm
  against the build.
- Wire both meter wells to real peak metering.
- Reconcile the parameter inventory — ranges, tapers and defaults — against the build's
  `AudioProcessorValueTreeState`; where they disagree the build wins.
- **`shared/HEADER-PART.md` revision 3** — three figure items waiting on build answers (meter
  clamp, format at both ends, sign convention). Its fourth item, the propagation process, is
  already written and does not wait.
- **`shared/BRAND-AMENDMENT-BYPASS.md`** — two one-line changes here if adopted (§7.4).
