# VampireKiller V17 — ROM assets / attack integration

## Source of truth
V16 validated by user in SGDK + BlastEm; V17 is an **experimental** animation expansion.
The provided MSX2 ROM must match SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`.
The original ROM is not redistributed in this source ZIP.

## Extracted and integrated
- Original MSX2 pattern streams and signed layer offsets used to assemble **8** SGDK attack candidates.
- The new sequence uses `(lower,upper)` pairs (0,0), (0,2), (0,6), (0,9), (1,9), (1,6), (1,2), (0,0).
- All 8 pairs have fully positioned graphic layers under the existing decoder.
- 8 x 64x32 RGBA previews, 8 x 32 x 8x8 Mega Drive 4bpp tiles (256 tiles total), 8192 32-bit words of packed pixel data? Actual generated table: 2048 words = 8192 bytes.
- An additional 8-step visual animation selector is based on the existing whip timer; 4 gameplay phases and collision ranges are **unchanged**.
- V16 flame sprites (4 looping frames) and sparks (3 frames), hit target, stage transitions, stair behavior are unchanged.
- Atlas of 64 upper/lower cross-compositions remains available under research/simon-assets/crosspairs (56 fully positioned and 8 partial combinations).

## Important provenance / uncertainty
- ROM-derived graphics are authentic data; **their chosen attack sequence and frame duration remain speculative** pending an openMSX trace.
- Animated flame and impact effects are still **prototype artwork**, NOT original ROM flames. No trustworthy MSX flame sprite map has yet been located.
- Attack uses independently drawn, temporary lash segments: it is not the original whip geometry.

## Regeneration
`python tools/build_simon_attack_md.py '/path/to/Vampire Killer (Japan, Europe) (En).zip' --output sgdk/src/generated`

## Testing
`sh tests/run_native.sh`
`python -m unittest discover -s tests -p 'test_*.py'`
Then compile with your configured SGDK environment and inspect B attacks right/left, flame destruction, A+B reset, stair demo A+UP and transitions in BlastEm.
The SGDK/BlastEm checks were not executable in the packaging environment.
