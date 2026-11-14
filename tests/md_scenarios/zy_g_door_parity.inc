# Shared by zy_g_door_parity_a/b.txt: left door of the stage 4 (room 3), opened on a chosen frame of the 2-frame pass.
# Needs `set ph V` (enemies.itemPhase seen by the next frame: 1 = first frame of a pass, 0 = second) and `set pre N`
# (idle frames, to change C003 at the opening). Same setup as zy_g_stage4_key_door.
include zy_enter4.inc
poke Enemies_god 1
poke vk_game+0 52
poke vk_game+1 96
poke vk_game+2 1
wait 4
expect ram8('items+4') == 0 and ram8('items') == 0 # key hidden in the block (32, 64)
hold B 2
wait 20
until ram8('items') & 1 max 200 LEFT
expect ram8('items+4') == 1 and ram8('items') == 1 # white key taken (C701 bit 0)
poke vk_game+20 2
poke vk_debug_keys 1
hold A 3
hold A+RIGHT 3
until game.room == 3 max 200
wait 10
poke vk_game+0 34
poke vk_game+1 96
poke vk_game+2 1
wait 4
expect ram8('items+2') == 0 # door closed
set tw frame
until frame - tw >= pre max 40
until enemies.itemPhase == ph max 4
# Touch (X 27, facing left) set up in a frame of the chosen phase: 8580 only runs on the first frame of a pass in the MSX
# (research/door-pose-phase.md), so with ph = 0 the door opens one frame later, still on a first frame.
poke vk_game+0 27
set tp frame
until ram8('items+2') == 1 max 6
expect enemies.itemPhase == 0 # the opening frame was a first frame (the flip to 0 comes after it)
expect frame - tp == (1 if ph == 1 else 2) # ph 0: the touch frame is skipped
set t0 frame
set xo game.player.x
set po player.pose
set c0 enemies.tick # C003 of the opening pass
expect xo == 27 and player.facing == 1 and 3 <= po <= 5
# Every frame of the walk against the trace of 16 openings (6CA4: pose = [2, 1, 0, 1][(C003 >> 2) & 3] + 3, C003 =
# C003 of the opening pass + f / 2, f = frame - t0; first 778A at f = 88: X - 1 then every 4 frames; pose of the opening held before).
wait 86
repeat 74 zy_g_door_parity_row.inc
until game.stage == 5 max 400 LEFT
wait 30
expect game.stage == 5 and game.room == 0 and game.player.x == 232 and game.player.y == 96
