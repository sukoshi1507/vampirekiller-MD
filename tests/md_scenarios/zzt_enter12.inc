# Stage 12 room 0 through the real pause / Konami code / stage selector (as zzp_stage12.txt).
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
tap B
