# Stair animation V3 — BlastEm validation candidate

Based on the last user-validated SGDK/BlastEm build (animations/interactions V2).

## Changes
- `VKPlayer.stair_anim`: separate 8-bit traveled-pixel animation counter, reset during player reset, stair entry and demo placement.
- Count increments only when stair movement actually updates X/Y. Holding a direction at an endpoint does not advance the sequence.
- Four-step cycle `0,1,2,1`, one keyframe every 8 diagonal movement steps. This replaces the previous clock-bit toggling (`clock & 4` / `clock & 8`).
- On idle, render the stand pose while keeping the step counter, so movement resumes consistently. Left/right mirroring and approved stair trajectory unchanged.
- Added assertions covering animation cadence, idle, direction inversion.

## Boundaries
Only the existing walking poses are used. These are not yet verified native MSX2 stair sprites; visual improvement needs testing on BlastEm. The demo stairs remain test-only; there is no claim of genuine MSX2 stair-coordinate reconstruction. No SGDK compilation or emulator run was performed in this environment.

## Test
`./tests/run_native.sh` from the project root.

BlastEm: in stage-1 room-1 use A+UP, then hold UP, release, hold DOWN and try both endpoints. Check sprite frame cadence and orientation. B remains the previously implemented diagnostic/prototype whip.
