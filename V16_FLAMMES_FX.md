# V16 — Flame and impact prototype polish

This is a conservative update to V15: no change to movement, stair physics, attack collision timing, scene navigation or the original Simon attack candidate frames.

- Courtyard flame now has **four** distinct 16×16-frame compositions, with a looping eight-tick cadence.
- Successful whip impact produces a **three-step spark burst** during the existing twelve-tick disappearance interval.
- Palette indices 1–3 add darker red/orange/yellow to the existing attack palette, preserving index E/F for legacy whip shapes.
- The generation script `tools/build_flame_fx.py` takes the two **V15 placeholder flame designs** from `sgdk/src/main.c` and derives seven frames in SGDK 4bpp format. The generated, compile-ready header is `sgdk/src/generated/flame_fx_md.h`.
- `tests/test_flame_fx.py` checks frame count, reproducibility, frame uniqueness and source integration.

**Provenance:** These flame/spark graphics are authored prototype artwork, **not** proven MSX2 ROM assets. Actual original flame motifs and the Z80-driven attack sequence still require identification. Do not claim their authenticity.

**BlastEm check:** trigger the courtyard; flame loops, B extinguishes it with a short spark burst; A+B restarts the flame. Repeat tests facing both ways, and verify stairs still move as in V15. SGDK and BlastEm tests must be run in a configured SGDK environment.
