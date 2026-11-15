# HUD 16 bits (PAL1 : entree 10 = (205, 206, 238), 3 = (65, 101, 139), 14 = blanc), verifie sur la capture courante.
# Texte : la police MSX garde ses formes, rangees 1, 2 et 6 en acier clair, rangee 7 en acier sombre, le reste blanc.
expect (205, 206, 238) in colors(8, 1, 100, 3) and (238, 238, 238) not in colors(8, 1, 100, 3) # haut de "SCORE-000000"
expect (238, 238, 238) in colors(8, 3, 100, 6) and (205, 206, 238) not in colors(8, 3, 100, 6) # corps des lettres
expect (65, 101, 139) in colors(8, 7, 100, 8) and (238, 238, 238) not in colors(8, 7, 100, 8) # bas des lettres
expect (238, 0, 32) in colors(176, 1, 183, 6) # le coeur garde son rouge
expect (238, 238, 238) not in colors(8, 0, 100, 1) and (205, 206, 238) not in colors(8, 0, 100, 1) # rangee 0 vide
# Barres PLAYER et ENEMY : cadre blanc, remplissage en degrade (4 rangees).
expect pixel(80, 13) == (238, 238, 238) and pixel(80, 18) == (238, 238, 238)
expect pixel(80, 14) == (238, 170, 139) and pixel(80, 15) == (238, 170, 139) and pixel(80, 16) == (238, 170, 0) and pixel(80, 17) == (172, 101, 65)
expect pixel(80, 23) == (238, 0, 32) and pixel(80, 24) == (238, 0, 32) and pixel(80, 25) == (172, 32, 32) and pixel(80, 26) == (139, 0, 0)
# Cases : filet blanc (rouge pour l'arme) en haut et a gauche, biseau acier a droite et en bas des cases blanches.
expect pixel(130, 11) == (238, 0, 32) and pixel(127, 20) == (238, 0, 32) and pixel(144, 20) == (238, 0, 32) and pixel(130, 28) == (238, 0, 32)
expect pixel(150, 11) == (238, 238, 238) and pixel(147, 20) == (238, 238, 238) and pixel(190, 11) == (238, 238, 238) and pixel(183, 20) == (238, 238, 238)
expect pixel(180, 20) == (65, 101, 139) and pixel(150, 28) == (65, 101, 139) and pixel(200, 28) == (65, 101, 139) and pixel(248, 20) == (65, 101, 139) and pixel(248, 11) == (65, 101, 139)
