# RE-CUT SHEET — REFLECT-84 RF-84

**Every row carries delivered *and* target dimensions.** A target dimension read without its
base is how three figures went wrong in this round: a needle height taken from a placement
offset, a plate "already 3×" of a canvas that no longer exists, a sprite "2×" against an old
frame. All three were true ratios with the base left out. This sheet exists so the target never
travels without it — `../MANIFEST.md` has the same rows for the whole suite.

**No assets, and none pending.** This is the only casting with no artwork in its bundle besides
its icon: the fascia is a gradient with a procedural scanline and corner wash, and every heading,
label, legend, numeral, tick, knob and the scope are drawn at runtime. **Call 6 permits this
explicitly** (GUI-SPEC §9) and it should stay this way unless the fascia grows texture that wants
baking.

| Delivered | State |
|---|---|
| `icons/reflect84-icon-{1024,256}.png` | unchanged by this round |

Fonts total 0.36 MB and all four faces are distributable.


---

## Barlow Condensed 500 and 700 (export 13)

**§2.2 asks for three weights of one family and the casting had one.** `BarlowCondensed-SemiBold.ttf`
came with bundle 2 on 2026-08-17 and serves §8's ten 600 roles; **Medium (500)** and **Bold (700)**
were never delivered here, though both were already cut for siblings — Medium to gatecrasher,
taperot and fifth-member with export 12, Bold to chorus-60, fifth-member and gatecrasher.

**Fetched from `google/fonts` `ofl/barlowcondensed/` and confirmed** — the route
`fonts/README.md` prescribes for this casting, so the build owns its own font provenance, with
`OFL.txt` beside them (4471 chars, *Copyright 2017 The Barlow Project Authors*). **Delivered into
this directory with export 13** — build and bundle are in step, unlike TapeRot's Medium in export 11.

### Read from the delivered files, and they are not what the ask described

| File | Bytes | SHA-256 (16) | Glyphs | `usWeightClass` |
|---|---|---|---|---|
| `BarlowCondensed-Medium.ttf` | 97960 | `27d58d15cead6f76` | **680** | 500 |
| `BarlowCondensed-Bold.ttf` | 104316 | `53550669f93c07de` | **680** | 700 |
| `BarlowCondensed-SemiBold.ttf` (bundle 2) | 103856 | `571bbdabf1bca470` | **680** | 600 |

**All three weights are the 680-glyph cut. There is no mixed cut on this casting** — the ask
described the fetched pair as 694-glyph and the delivered files are not, so **the state it asked to
correct for consistency was already consistent.** Every metric §8 depends on agrees across the
three: 1000 upem, typo 1000 / −200 / 0, win 1075 / 274, cap height 700. Only x-height moves with
weight — 509 / 511 / 514 at 500 / 600 / 700 — which is what x-height does.

**And the Medium is byte-identical to the one delivered to gatecrasher and taperot in export 12**,
same 97960 bytes and same `27d58d15…`. So the suite now runs **one cut of Barlow Condensed across
five castings and three weights**, which is a stronger position than the ruling asked for and was
arrived at without anyone choosing it.

**What could not be confirmed here:** the ask's byte-equality of the Bold against Fifth Member's
delivered copy. `handoff/fifth-member/fonts/` contains only `ABSENT.md` — no Barlow Condensed of any
weight is in that directory — so **the comparison cannot be reproduced from this bundle** and is
carried on the build's word. Recorded rather than repeated as though checked.

### The mixed cut: withdrawn, there wasn't one

**A ruling was written on this in export 13's first pass and is retracted.** It accepted a
680-against-694 mixture, weighed re-cutting the SemiBold, and set a metric-equality condition for a
future convergence — all of it reasoning about a state that does not exist. The premise came from
the ask, was plausible, and was never read off the files until they arrived.

**The lesson is the one this suite keeps relearning from the other direction:** a figure taken from a
report and reasoned over is a figure claimed. It cost nothing here because the answer to a mixture
and the answer to no mixture happened to be the same — keep what was delivered — but the reasoning
was about fiction, and a conditional handed to a future reader (*"compare the 694 cut's OS/2…"*)
would have sent them looking for a file that isn't there.

### `check_font_sets.py` passed this casting twice, for two unrelated reasons

**Both are worth keeping, because the arm was written in export 12 precisely to catch this.**

- **700 — a real defect, and not one bad pair.** The matcher tested `weight_name in filename`, and
  `"bold"` is a substring of `"semibold"`: a SemiBold file satisfied a Bold ask. **The same
  collision sits under ExtraBold/Bold and ExtraLight/Light**, so any suite using those weights was
  exposed the same way. Fixed by longest-match-then-compare-equal; re-run it reports
  `MISS Barlow Condensed 700 — no file, and not declared`. **A substring test on a filename is a
  guess dressed as a check**, and it failed in the direction that reports success.
- **500 — scope, not defect.** The tool reads §8's type table, and §8's ALGORITHM row said
  `600 / 700`. The 500 lived only in §2.2's prose. **§8's row is now split in two** — caption at
  600, corner labels at 500 / 700 — so the weight is where the tool can see it. **A checker that
  reads one table makes that table the contract**; a requirement stated only in prose is outside
  every arm the suite has.

### Not changed

- `BarlowCondensed-SemiBold.ttf` — ten §8 roles measure against it and §8's sizes are conformant.
- **IBM Plex Mono stays.** Call 7 splits the two faces; this casting's printed numerals, units and
  on-glass scope legends are its own mono and none of the above touches them.


## Jost and IBM Plex Mono (export 14)

**§8 declared five faces and `fonts/` held two.** Barlow Condensed's three weights closed in
export 13; **Jost 500 and IBM Plex Mono were still missing**, and the prototype had been drawing both
from the Google Fonts CDN — which renders correctly and delivers nothing. `Jost-Medium.ttf` (500,
535 glyphs) and `IBMPlexMono-Regular.ttf` (400, 1028 glyphs) are now here with their own licence
files, and the Barlow `OFL.txt` is renamed `BarlowCondensed-OFL.txt`.

**This directory now has an `ABSENT.md` saying nothing is absent**, with the full inventory and the
verification. It had none, and that was the real defect: for six exports the missing marker read as
nothing-to-report while three declared faces were undelivered. **A directory that makes no claim
cannot be wrong, which is why every fonts directory in this bundle should carry one** — the other
five say what is absent and why; this one says the set is complete.

**A CDN link is not a font delivery.** It was the mechanism here, and it is worth naming because it
fails in the most comfortable direction: the panel looks right on every machine with a network, and
the bundle ships two families short. The prototype's link also requests `Jost:wght@400;500;600` and
`IBM+Plex+Mono:wght@400;500;600` while drawing exactly one weight of each — **over-requesting is what
made Plex Medium available for the build to substitute into §2.2**. Worth trimming to `wght@500` and
`wght@400` so the link states what ships.
