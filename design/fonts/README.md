# Fonts

**This folder holds the build's font binaries.** It used to open with the line *"This folder
intentionally contains no font binaries"* — written when the two families below were fetched at
build time and true until bundle 2 landed Share Tech Mono and Barlow Condensed SemiBold here on
2026-08-17. It said the opposite of the truth for six days, in the file a reader opens to find out
what is in the folder.

| File | Weight | Used for | Source |
|---|---|---|---|
| `jost/Jost-500-Medium.ttf` | 500 | the wordmark, and nothing else | Google Fonts, OFL |
| `ibm-plex-mono/IBMPlexMono-Regular.ttf` | 400 | printed numerals, units, on-glass scope legends | Google Fonts, OFL |
| `ibm-plex-mono/IBMPlexMono-Medium.ttf` | 500 | — see the note below | Google Fonts, OFL |
| `ShareTechMono-Regular.ttf` | 400 | everything on glass — the suite's LCD face | design bundle 2 |
| `BarlowCondensed-Medium.ttf` | 500 | §2.2's UNSELECTED ALGORITHM corner label | Google Fonts, OFL — see below |
| `BarlowCondensed-SemiBold.ttf` | 600 | §8's panel lettering | design bundle 2 |
| `BarlowCondensed-Bold.ttf` | 700 | §2.2's SELECTED ALGORITHM corner label | Google Fonts, OFL — see below |

## Medium and Bold were fetched, not delivered

§2.2 states ALGORITHM's selection as **weight and value both — 700 / `#332b1e` selected against
500 / `#5e5440` unselected**. Only SemiBold has ever been delivered to this casting, so the build
encoded that pair as IBM Plex Mono Medium against Regular: 500 against 400, in a monospace, which
is nearly the same colour on the panel. The selected algorithm did not read as selected.

The two missing weights were fetched from the OFL source this file already prescribes — *"fetch
them from source rather than taking them from a design bundle, so the build owns its own font
provenance."* `BarlowCondensed-OFL.txt` sits beside them.

**The Bold comes back byte-identical to the one the designers delivered to Fifth Member**, which is
what confirms the source rather than merely making it plausible. The SemiBold already here is an
earlier cut — 680 glyphs against the fetched pair's 694 — so this casting mixes two cuts of one
family. Same 1000 upem, so em sizes are consistent; the difference is glyph coverage, none of it
on this panel.

`design-asks/OPEN.md` carries the ask for both weights to be delivered properly, which is what
would retire this section.

## IBM Plex Mono 500 has no consumer

It was the *selected* corner label until the weights above landed, and `Font::monoMedium` is now
unreferenced. Left in place rather than removed in the same pass: retiring a face is its own change
and wants its own check that nothing else reaches for it.

## Do not install a design bundle over `design/`

A bundle is a **reference package, not a tree to sync**. It has no claim on `design/fonts/` or
anything else the build owns. Nothing in a bundle should ever delete a build asset.
