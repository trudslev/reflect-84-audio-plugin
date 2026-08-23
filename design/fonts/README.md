# Fonts

**`ABSENT.md` is the register for this directory** — it arrived with bundle 3 export 14 and states
the five delivered faces with their weights, glyph counts and licences. This file records the one
thing it does not: **which build target reads which file.**

| File | Weight | Read by | For |
|---|---|---|---|
| `BarlowCondensed-Medium.ttf` | 500 | `Font::labelMedium` | §2.2's unselected ALGORITHM corner label |
| `BarlowCondensed-SemiBold.ttf` | 600 | `Font::label` | §8's panel lettering — pills, control labels, ALGORITHM caption |
| `BarlowCondensed-Bold.ttf` | 700 | `Font::labelBold` | §2.2's selected corner label |
| `Jost-Medium.ttf` | 500 | `Font::wordmark` | the wordmark, and nothing else |
| `IBMPlexMono-Regular.ttf` | 400 | `Font::mono` | model line, printed numerals, units, on-glass scope legends |
| `ShareTechMono-Regular.ttf` | 400 | `Font::lcd` | everything on glass |

Every one of the six is named in `CMakeLists.txt`. **There is no seventh file and no unread file** —
which is the state this directory has not previously been in.

## What export 14 changed here, and what it corrected

This file said, for one day, *"Medium and Bold were fetched, not delivered"* and described the
directory as mixing a 680-glyph SemiBold with a 694-glyph fetched pair. **Both halves are now
wrong**, and the second was wrong about the wrong thing:

- Export 13 **delivered** Medium and Bold, so nothing here is fetched.
- The delivered pair is the **680-glyph cut**, byte-identical to the Medium already shipping in
  gatecrasher and taperot. So this casting runs **one cut across all three weights**, which is
  better than the mixture the earlier note accepted.

The 694-glyph files that briefly sat here were a real fetch of a real cut — the figure was measured,
not invented — but they were **a different cut from the one the suite ships**, and reading the
delivered files rather than a report is what settled it.

Export 14 then delivered `Jost-Medium.ttf` and `IBMPlexMono-Regular.ttf`, which this build had been
sourcing for itself into nested `jost/` and `ibm-plex-mono/` folders. **Those were different files,
not just different paths** — Plex 1033 glyphs against the delivered 1028, Jost 102240 bytes against
61652 — so leaving both would have meant the build drawing an undelivered face beside a delivered
one nothing read. The self-fetched copies are removed and `CMakeLists.txt` names the delivered ones.

**Adopting them was measured, not assumed**: captured before and after, the only part of the panel
that moved is the wordmark, by at most 2.5 sum-RGB of 765 across a 12 px block — antialiasing. Every
printed numeral, unit, model line and scope legend is unchanged, which is what says the delivered
Plex renders identically to the fetched one at these sizes.

`IBMPlexMono-Medium.ttf` went with them. It was §2.2's selected corner label until Barlow 700
arrived, and `ABSENT.md` records that no weight of Plex other than 400 is drawn here.

## Do not install a design bundle over `design/`

A bundle is a **reference package, not a tree to sync**. Merge it additively; nothing in a bundle
should ever delete a build asset.
