set px player.x
wait 1
expect player.x == px or enemies.itemPhase == 0 # 778A runs on the first frames of the passes (itemPhase 1 before the flip, 0 after): X only moves there
expect player.x == xo - (0 if frame - t0 < 88 else 1 + (frame - t0 - 88) // 4) and player.pose == (po if frame - t0 < 88 else [2, 1, 0, 1][((c0 + (frame - t0) // 2) >> 2) & 3] + 3) # f = frame - t0
