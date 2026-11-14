# (zzq) One image of "wait here, whipping what comes close, until no enemy of the types in the variable ct (stage 10: fish men, 17) is alive": `repeat N zzq_clear.inc` (takes no image once the room is clear).
# Stage 11 fleas (type 14) drop onto Simon from above (the whip cannot reach them on the way down): step right under them so that they land behind him.
when game.recoil == 0 and game.player.mode == 0 and any(e.active and e.type == 14 and 20 <= game.player.y - e.y <= 110 and abs(e.x - game.player.x) < 24 for e in enemies.enemy) :: hold RIGHT 1
when game.recoil == 0 and whip.timer == 0 and game.player.mode == 0 and any(e.active and e.type in gt and abs(e.y - game.player.y) < ty and 0 < e.x - game.player.x < 38 for e in enemies.objects) :: hold B 14
when game.recoil == 0 and whip.timer == 0 and game.player.mode == 0 and any(e.active and e.type in gt and abs(e.y - game.player.y) < ty and 0 < game.player.x - e.x < 50 for e in enemies.objects) :: include zzq_turn_whip.inc
when any(e.active and e.type in ct for e in enemies.objects) :: wait 1
