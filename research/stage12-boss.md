# Stage12 boss: body15h and rider18h

Port: boss.c / boss12.inc, kind3 in the existing VKBoss. The old VKBoss fields retain their offsets; a collision-map pointer is appended. EnemyState remains 166 bytes on m68k.

## Executed entry points

- Room BA26 (CE00=4), boss room6, door room5. 77D1 places Simon at X=10, preserves Y, requests music87.
- Body15h: BA4F init at (D0,C0), BA64 update, BAB9 pose79/7A/7B/7A. +10 reaches zero after48 passes and sets CE07. CE09 holds the last chosen horizontal speed, independently of edge reversals.
- Rider18h: BADD init at (D0,A0), BAF6 update. BBE1 shot counter48; BBD4/A0E5/9F61 creates D7000Ah. This firing path does not request a sound.
- BBA8 reads R and computes HL, but calls A55D with unchanged DE. The resulting jump speed is the base DE (-0780 or -0580), not HL. The differential test executes the original LD A,R without patching it.
- 7DD4/7DE6 and 9AB3 on rider hits: HP changes with 8-bit wrap; apparent lethal hits grant200 points and set +10=8 without removing the object. The subsequent eight update passes are motionless. This is not a generic enemy death.
- Body damage7E0C/7E2C: 4 or6 energy, 128 total, 32 or22 hits. Terminal7E17 requests1C and awards3000 points through81AB/44EE. Body class6: H=28h,L=5; contact damage6, or3 with frontal armor. Rider class2: H=10h,L=8; contact damage2, or1 with frontal armor.
- D7000Ah has class0 (H=6,L=3), not the wider class1 fireball used by the stage6 boss.
- 9A1A transforms a removed shot into D7000Ch through the first free slot; 9B60/9B71 supplies the sixteen-pass disappearance flame. +7E=0, so the flame continues during time stop.
- End B6AB/B6C5: death flames, sixty-pass wait, orb22h, landing at Y=C0, pickup7E67 or timeout, music8C, optional healing, C409 exit. B725 explicitly stops sound before music on pickup. Orb +7E=0 also makes it independent of time stop.

## Graphics

build_boss12_md.py executes5780 to load the stage12 room6 sprite patterns and6445 to obtain all pose layouts. Its synchronous VDP adapter implements VRAM reads used by the loader's mirror operation. The three mirrored rider pairs are checked against the source layers. Fifteen composed16x16 cells use60 MD tiles; each pair has colors02/45, combined with OR. The already validated common fireball and death-flame cells occupy tiles60..71 in the same block. Orb graphics/palette reuse the existing non-stage3 orb path.

## Replay and limits

Run tests/run_boss12_trace.sh with PYTHON pointing to the venv containing z80/Pillow/numpy. It regenerates research/boss12-trace.json (not delivered).

The harness executes ROM routines with a mapper and synchronous BIOS/VDP adapters, not a full interrupt-driven MSX machine. One game pass corresponds to two held video images. It compares all32 object bytes, live projectile slots, sprite positions/patterns, Simon coordinates, score, energy and script/audio events within the stated scenarios. It does not prove hardware video timing or a full playthrough.

This lot connects the boss module, rendering and combat callbacks. Enabling the stage12 room graph, progression and debug selection remains LOT11. The boss receives its real collision map through vk_boss_set_collision; a caller must supply it after vk_boss_begin_stage.
