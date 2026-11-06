# Boss du stage 3 (objet 12h) et fin de stage

Code : `sgdk/src/boss.c` (logique, `tests/test_boss.c`), `actors.c` (vue, contacts), `main.c` (porte,
sortie), `hud.c` (barre ENEMY). Scénario : `tests/md_scenarios/zr_stage3_boss.txt`.
Tuiles : `tools/build_boss_md.py`, lancé sur deux traces openMSX non versionnées (combat, orbe).

## Prouvé (ROM et traces openMSX)

- Porte salle 4 (236,128) : effets FF (fondu) et 1A, sortie à pied, salle 5, Simon (10,176) puis chute
  à 192, musique 87.
- Objet : BE35/BE50 (Y 40h, X 70h, 60 passes au repos), BE70 états 0 à 5, visée A0E5 (vy 2D4h, vx 2C4h
  vérifiés), boule à +10 = 18h, effet 13. Tout avance une image sur deux.
- Fouet : un test par coup, boîte 83F2, -8 par coup (trace). Mort : C445 = 14h, effet 1C, objets vidés.
- Contact 83EA : 4 points (2 et C441 - 1 de face avec l'armure). Boule : 8318, 1 point sans armure ;
  le fouet la détruit (8320, effet 0C).
- Script B6AB : attente, orbe (Y 40h, chute à C0h), prise, musique 8C, soin, sortie vers le stage 4.
- Barre ENEMY : `(C418 & FC) / 2` px, 2 px si 0 < C418 < 4, couleur 8, y 23..26.
- Au passage à l'orbe, les entrées 4 et 6 de la palette de salle deviennent R3 et R5 (instantanés 1161
  et 1282 ; moment rattaché à 57C0, déduit du script).
- Rendu comparé à un instantané MSX : pose 4E, 10 px sur 2700 différents, à la position de la bougie
  (117,130) ; orbe 8F, 40 px sur 46080 sur tout l'écran de jeu.

## Déduit

- Score 2000 : paire 17 de 81CE (`20 02`), non mesuré.
- Armes lancées : `shot_touch` h 18h, l 0Ch, 8 ou 12 de dégâts selon C416.

## Provisoire, ouvert

- BF32 : `40h - (R >> 2)`, R tracé en 60 appels (A < 80h, uniforme) : `Enemies_randomR() >> 2` (`research/hasard-R.md`).
- Armes lancées : réarmement désormais lu dans la ROM (`999F`, `research/stage4-enemies.md`), soit 2 coups par
  hache et 6 haches ; non mesuré en jeu.
- Limites de sortie d'écran de la boule : observées, pas lues dans la ROM.
- Barre ENEMY remise pleine hors de la salle du boss : instant prouvé au passage de stage (`C418` 0 -> 80h dans l'état 8, trace du
  stage 6, `research/stage6-boss.md`) ; le port la remet à la création de la salle suivante.
- Recul sur le boss : à l'opposé de son X ; non mesuré.
- Objet 09 (C434) : la ROM supprimerait le boss sans CE15 (blocage). Le port ignore le contact à la place.
- La bougie passe devant le boss sur le MD (10 px ci-dessus) : cause non recherchée.
- Pas de partie complète jouée à la manette : le scénario donne la clé en mémoire et place le boss par
  `poke` pour les coups et contacts. Les poses de l'orbe 90 et 91 ne sont pas comparées au pixel.
