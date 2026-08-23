# ABSENT — reflect-84/fonts/

**Nothing is absent from this directory.** This file exists to say so, because for six exports the
absence of a marker here read as "nothing to report" when in fact **§8 declared five faces and the
directory held two**. A directory with no `ABSENT.md` makes no claim, and no claim is what let three
faces sit undelivered without anything recording it.

## What is here, complete as of export 14

| File | Weight | Glyphs | Licence |
|---|---|---|---|
| `BarlowCondensed-Medium.ttf` | 500 | 680 | `BarlowCondensed-OFL.txt` |
| `BarlowCondensed-SemiBold.ttf` | 600 | 680 | ″ |
| `BarlowCondensed-Bold.ttf` | 700 | 680 | ″ |
| `Jost-Medium.ttf` | 500 | 535 | `Jost-OFL.txt` |
| `IBMPlexMono-Regular.ttf` | 400 | 1028 | `IBMPlexMono-OFL.txt` |
| `ShareTechMono-Regular.ttf` | 400 | — | — |

**Three licence files, three families, no sharing.** They are genuinely different documents — 4471,
4478 and 4455 chars, copyright *Barlow Project Authors* / *Jost Project Authors* / *IBM Corp. with
Reserved Font Name "Plex"* — so the single `OFL.txt` that arrived with the Barlow weights in export 13
was renamed `BarlowCondensed-OFL.txt` when the other two landed. **A bare `OFL.txt` in a
multi-family directory is a filename that will eventually be overwritten by the next family's.**

## What was verified, and what it caught

Every face read from its own tables before placing:

- **`Jost-Medium.ttf`** — `usWeightClass` **500**, matching §8's wordmark row. The **static** file, not
  `Jost-VariableFont_wght.ttf`: one weight is drawn and every other face in this suite ships static.
- **`IBMPlexMono-Regular.ttf`** — `usWeightClass` **400**. §8's three mono rows name no weight, and the
  prototype sets Plex Mono once on the panel root with none, so 400 is what the model line, printed
  numerals, units and on-glass scope legends all inherit. **No other weight of Plex is drawn** — which
  is what makes the build's old §2.2 substitution to Plex Medium a face this casting never carried.
- **Its `post.isFixedPitch` reads 0, which for a monospace looks wrong and is not.** 981 of 1028
  glyphs advance **600/1000**; the remaining 47 advance 0 (marks and combining forms, which is correct
  for them). **Every glyph this panel prints is on the 600 grid**, so the numeral columns align. Worth
  the check rather than the assumption — a mono whose widths did not agree would break §3's printed
  scales silently.

**Vertical metrics differ between the three families and that is expected**, not a mixed-cut problem:
Barlow 1000 / −200, Jost 1070 / −375, Plex 1025 / −275, all at 1000 upem. §8 pins a line box per role
rather than inheriting font metrics anywhere on this panel, which is exactly why it can.

## What must not be added

**No italics, no variable files, no further weights.** The Google Fonts packages these came from
carry 14 Plex cuts and 18 Jost statics; **five files ship because five roles exist.** And two of those
packages contain **Bold/SemiBold and ExtraLight/Light in one family** — both collision pairs of the
`check_font_sets.py` substring bug, which makes IBM Plex Mono the natural regression case for the
fixed matcher.
