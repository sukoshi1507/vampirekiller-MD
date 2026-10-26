# V13 — Attack body/whip synchronization

Baseline: V12, verified compiled and launched by the user; facing and stair behavior verified.

## Root cause

V12 rendered the ROM-derived body frames when B was pressed, but the whip lash was visible only under `if (!attack_frame && segment < count)`. Because `attack_frame` is nonzero whenever the whip phase is active, the actual three lash sprites were always hidden during attacks. This made attacks appear incomplete.

## Change

- The lash now draws when `segment < count`, regardless of whether a ROM body composition is active.
- The upper index pairs (0,0), (0,6), (0,9), (0,6) provide a distinct returning upper-body pose. These are *candidate* ROM-derived compositions; their exact ordering and durations have not been validated by a dynamic openMSX trace.
- The whip hitbox and sprites continue to use the same `vk_whip_phase` / `vk_whip_bounds` mechanics as V12; the hitbox is active only during short/full extension.
- No changes to stairs, room collision or player physics.

## BlastEm checklist

1. Face right, press B: body prepares, whip projects briefly with short then full reach, body returns. No reversal.
2. Face left, repeat. Lash should project left.
3. Check that F1-F4 increment and that no lash is shown in preparation or recovery.
4. A+UP on test stairs; UP/DOWN should retain V3 behavior.

## Validation

Native C gameplay/physics/whip tests and Python attack asset/render/orientation tests passed. SGDK build and BlastEm execution NOT performed in this environment. The frame/attack timing still requires openMSX measurement for exact fidelity.
