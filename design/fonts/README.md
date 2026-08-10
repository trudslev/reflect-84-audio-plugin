# Fonts

**This folder intentionally contains no font binaries.**

Both families are Google Fonts under the SIL Open Font License 1.1 and are free to
embed as JUCE `BinaryData`. Fetch them from source rather than taking them from a
design bundle, so the build owns its own font provenance:

| Family | Weights used | Source |
|---|---|---|
| Jost | 500 (Medium) | https://fonts.google.com/specimen/Jost |
| IBM Plex Mono | 400 (Regular), 500 (Medium) | https://fonts.google.com/specimen/IBM+Plex+Mono |

Jost 500 is the wordmark only. IBM Plex Mono 400 carries everything else; 500 is
used in exactly one place — the selected ALGORITHM corner label, where weight is
half of the selected/unselected encoding (see `GUI-SPEC.md` § 4).

`GUI-SPEC.md` § 14 lists 600 as also available. Nothing in the current design uses
it; embed it only if you want headroom.

## Do not install this bundle over `design/`

This bundle is a **reference package, not a tree to sync**. It has no claim on
`design/fonts/` or anything else the build owns. Copy out the two documents and the
reference renders; leave the build's own directories alone. Nothing here should
ever delete a build asset.
