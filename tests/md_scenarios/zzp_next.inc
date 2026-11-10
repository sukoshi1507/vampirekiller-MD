# Actual pause/Konami/room selector; this resets the game for the target room.
until game.player.mode == 0 max 400
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
tap RIGHT
tap B
