# (zzq) One image of "walk LEFT to X = tx (variable set by the scenario), whipping what comes close on the way" (mirror of zzq_walk.inc): `set tx 186` then `repeat N zzq_walkl.inc`.
# Takes no image once Simon is at X <= tx, so the repeat count is only an upper bound.
# Stage 11 fleas (type 14) drop onto Simon from above (the whip cannot reach them on the way down): step right under them so that they land behind him.
when game.recoil == 0 and game.player.mode == 0 and any(e.active and e.type == 14 and 20 <= game.player.y - e.y <= 110 and abs(e.x - game.player.x) < 24 for e in enemies.enemy) :: hold RIGHT 1
when game.recoil == 0 and whip.timer == 0 and game.player.mode == 0 and any(e.active and e.type in gt and abs(e.y - game.player.y) < ty and 0 < game.player.x - e.x < 38 for e in enemies.objects) :: hold B 14
when game.recoil == 0 and whip.timer == 0 and game.player.mode == 0 and any(e.active and e.type in gt and abs(e.y - game.player.y) < ty and 0 < e.x - game.player.x < 50 for e in enemies.objects) :: include zzq_turn_whip_right.inc
when game.player.x > tx and game.room == troom :: hold LEFT 1
