until front_mode == MODES['TITLE'] max 2000
wait 10
tap C
wait 40
tap START
wait 10
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
wait 4
tap A
expect ram8('Enemies_god') == 0
tap DOWN
tap DOWN
tap DOWN
tap RIGHT
tap RIGHT
tap RIGHT
tap B
until enemies.alive == 2 max 200
