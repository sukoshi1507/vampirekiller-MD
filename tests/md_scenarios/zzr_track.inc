# Suivi par image des aigles (type 18) et du premier coup : spl = images de creation, syl = Y de creation, hit = image du premier coup.
set cur any(enemies.objects[k].active and enemies.objects[k].type == 18 for k in range(15))
when cur and not prev :: set spl spl + [frame]
when cur and not prev :: set syl syl + [[enemies.objects[k].y for k in range(15) if enemies.objects[k].active and enemies.objects[k].type == 18][0]]
set prev cur
when hit == 0 and enemies.energy < 32 :: set hit frame
wait 1
