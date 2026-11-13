# Death flame of the bodies of stages 3, 6, 9 (research/boss-body-flame.md; included by zr_stage3_boss, zz_stage6_boss, zzd_stage9_boss right after the
# killing blow, `hold B 1` then `until ram8('vk_boss+192') != 0` = flame[0] set by 7806 -> 9A3A). The caller sets `ex` = the object (X, Y - 18h) of each flame.
# openMSX traces (scratchpad bf, stages 3, 6, 9; 1E block at body Y - 24, X unchanged; SAT entry (X - 8, Y - 16) with Y + 1 on screen, colour 8, second entry
# off screen): visible 34 frames starting 2 frames after 7806, F4 x 6, F8 x 8, F4 x 8, F8 x 8, F4 x 4 (a 3-frame-late second 7806 from B6AB restarts the
# 16 passes). The port sends its sprites one frame after the calculation (frame-timing.md): the sprite table shows the first F4 one frame after the counter.
set fl lambda t: sorted((x, y) for (tl, x, y) in linked_sprites(t, t))
set want lambda dy: sorted((x - 8, y - dy) for (x, y) in ex)
set k frame
until fl(752) + fl(756) != [] max 10
set s frame
expect s - k <= 1 # sprite one frame after the calculation
expect fl(752) == want(15) and fl(756) == [] # pose 85: F4 at (X - 8, Y - 15)
until fl(756) != [] max 20
expect frame - s == 6 and fl(756) == want(14) # F8 at Y - 14
until fl(752) != [] max 20
expect frame - s == 14 and fl(752) == want(15)
until fl(756) != [] max 20
expect frame - s == 22
until fl(752) != [] max 20
expect frame - s == 30
until fl(752) + fl(756) == [] max 20
expect frame - s == 34 # 34 frames in all
expect ram8('vk_boss+192') == 0 and ram8('vk_boss+193') == 0
