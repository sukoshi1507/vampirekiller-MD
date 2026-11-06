# Logo -> titre -> chateau -> cour (FRONT_COURTYARD), controle du joueur.
# Inclus par les scenarios ; s'appuie sur front_mode (sgdk/src/main.c).
until front_mode == MODES['TITLE'] max 300        # le logo passe seul au titre (120 images de jeu)
tap START                                          # titre -> chateau
until front_mode == MODES['CASTLE'] max 30
tap START                                          # saute l'animation du chateau
until front_mode == MODES['COURTYARD'] max 30
wait 5
