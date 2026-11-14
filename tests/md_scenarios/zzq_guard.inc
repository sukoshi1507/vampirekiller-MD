# (zzq) One image of "wait, but whip what comes close": used with `repeat N zzq_guard.inc` for timed waits without invulnerability.
# Targets: the flying type 2 (stage 10 room 0) and the fish men (type 17) that leap out of the water and walk along the ledges (stage 10 rooms 1-5).
# What a human does: the whip reaches ~31 px in front of Simon (and only catches the flying type 2 while it is high enough, y <= ~122, the middle of its wave), so the
# whip is started a little early and stays out ~14 images; an enemy that comes from behind needs a turn first. Only standing (mode 0) and not in recoil.
# Stage 11 fleas (type 14) drop onto Simon from above (the whip cannot reach them on the way down): step right under them so that they land behind him.
when game.recoil == 0 and game.player.mode == 0 and any(e.active and e.type == 14 and 20 <= game.player.y - e.y <= 110 and abs(e.x - game.player.x) < 24 for e in enemies.enemy) :: hold RIGHT 1
when game.recoil == 0 and whip.timer == 0 and game.player.mode == 0 and any(e.active and e.type in gt and abs(e.y - game.player.y) < ty and 0 < e.x - game.player.x < 38 for e in enemies.objects) :: hold B 14
when game.recoil == 0 and whip.timer == 0 and game.player.mode == 0 and any(e.active and e.type in gt and abs(e.y - game.player.y) < ty and 0 < game.player.x - e.x < 50 for e in enemies.objects) :: include zzq_turn_whip.inc
wait 1
