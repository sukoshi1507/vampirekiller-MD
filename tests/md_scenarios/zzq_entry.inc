# LOT12 stage18: all10 rooms via controller debug selector.
# Not a continuous walk: room selection restarts the game. Action fixtures below are explicitly RAM-set.
until front_mode == MODES['TITLE'] max 2000
tap START
tap START
until front_mode == MODES['COURTYARD'] max 500
wait 60
tap START
until ram8('paused') == 1 max 30
tap UP
tap UP
tap DOWN
tap DOWN
tap LEFT
tap RIGHT
tap LEFT
tap RIGHT
tap B
tap A
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap DOWN
tap B
until game.stage == 18 and game.room == 0 max 300
poke Enemies_god 1
until game.player.mode == 0 max 300
wait 30
include zzp_next.inc
until game.stage == 18 and game.room == 1 max 300
poke Enemies_god 1
until game.player.mode == 0 max 300
wait 30
include zzp_next.inc
until game.stage == 18 and game.room == 2 max 300
poke Enemies_god 1
until game.player.mode == 0 max 300
wait 30
include zzp_next.inc
until game.stage == 18 and game.room == 3 max 300
poke Enemies_god 1
until game.player.mode == 0 max 300
wait 30
include zzp_next.inc
until game.stage == 18 and game.room == 4 max 300
poke Enemies_god 1
until game.player.mode == 0 max 300
wait 30
include zzp_next.inc
until game.stage == 18 and game.room == 5 max 300
poke Enemies_god 1
until game.player.mode == 0 max 300
wait 30
include zzp_next.inc
until game.stage == 18 and game.room == 6 max 300
poke Enemies_god 1
until game.player.mode == 0 max 300
wait 30
include zzp_next.inc
until game.stage == 18 and game.room == 7 max 300
poke Enemies_god 1
until game.player.mode == 0 max 300
wait 30
include zzp_next.inc
until game.stage == 18 and game.room == 8 max 300
poke Enemies_god 1
until game.player.mode == 0 max 300
wait 30
include zzp_next.inc
until game.stage == 18 and game.room == 9 max 300
poke Enemies_god 1
until game.player.mode == 0 max 300
wait 30
