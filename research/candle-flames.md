# Flammes des bougies et des blocs : durée, chute, apparition de l'objet

Code : `sgdk/src/candles.c` (`step_drop`, `flame_landed`, `land_drop`). Tests : `tests/test_candles.c` (4 flammes de traces réelles, cœur, objet 00/15, repos au sol),
scénario `tests/md_scenarios/y_hall_room5_candles.txt`. Traces openMSX (`scratchpad/stage4/tool/run.py`, `harness.tcl` + échantillon `objs.csv` : C500 (96 octets),
C800 à CB00 (24 octets par objet), SAT à chaque image ; non versionnées : `flame/` de la session, scripts `tr.py` (bougie déplacée à Y = sol de Simon - 32, Simon la
frappe au fouet) et `fake.py` (C425 / C422 = 3 posés deux images : coup sur la bougie à sa vraie place, haute au-dessus du sol).

## Prouvé (traces)

Le coup est vu à l'image t0 où l'objet de la flamme apparaît (les échantillons tombent à une image près, la boucle des objets tourne une passe sur deux).

| Objet de la bougie | Flamme | Suite |
|---|---|---|
| 00 (stages 1, 7 et 8, 3 bougies tracées) et 15 (stages 4 et 8, bougie et bloc) | objet C500 type 01, `+4` = 00 ou 15, compteur `+6` de 1 à 0Fh (une passe sur deux) : vu **30 images** (t0 à t0 + 29) | retiré à t0 + 30 (`8AA7` -> `8B1B`) ; objet 15 : `8AD0` crée l'objet 1A/1B/1C |
| 01 (cœur, ou arme déjà tenue, `8A29`) | objet **C800 type 1E** (`89BF`, `C` = 1Eh), compteur `+C` de 10h à 1 : **31 images**, pose 85/86 en alternance | t0 + 31 : le cœur est un **C800 type 24** en (X, Y) de la flamme, il descend de 1 px toutes les 2 images (X oscille) jusqu'au sol, 64 images de 16 px, puis C500 83 (au repos) |
| 02 à 1D (stages 1 à 7, 12 traces) | objet **C800 type 26**, immobile 40 images (compteur `+C` de 14h à 0, passes), pose EC fixe, couleur 0E, 01, 08, 01 en boucle toutes les 2 images | à t0 + 41 elle tombe de 8 px toutes les 2 images (X + 8, Y + 16 de la bougie au départ) jusqu'à toucher le sol ; **aucun objet C500 pendant la flamme** (`89BF` rend Z, `899D` sort avant `8A37`) ; l'objet 83 -> 84 est créé en (X, Y + 8 n) au moment où la flamme se pose, à t0 + 41 + 2 n (n = pas de la chute : 0 à 8 vus, 41 à 58 images) |

La chute de l'objet 82 de `8AAE`/`8AF8` n'est que le repli sans place libre dans C800 (`899D`) : jamais vu en jeu.

Pose de la flamme 26 (`9C46` -> `9C5D` -> `7B98`) : tuile à (X, Y) puis (X - 10, Y) de la flamme, la ligne étant celle de Y - 16 (`7D2F`) : dans le repère du port (x, y) du drop, tuiles
(x + 8, y + 16) et (x - 2, y + 16). Quatre poses relevées (stage 7 salle 1 (96, 80) -> 128 ; stage 6 salle 2 (128, 48) -> 96 ; stage 4 salle 3 (64, 48) -> 112 ; stage 2 salle 2
(160, 112) -> 144), dont deux ne se posent que par le second point (X - 10). Objet créé en `((X - 4) & F8, (Y - 16) & F8)` (`9C54` : HL = 0810h).

Bloc cassé : un objet ordinaire est posé aussitôt en (X, Y + 16), type 83 puis 84, sans flamme (`89E3`, trace : objets 02, 17, 18 dans les stages 1 et 2, 83 vu à l'image du coup), comme
le port ; un bloc d'objet 15 a la même flamme C500 de 30 images que la bougie (trace de l'objet 1C). Repos au sol (`8BA2`) : `+6` = 8 diminue d'un toutes les 16 passes (**32 images**,
pas 16) : disparition 241 images après la création (objet 02 posé à 970, parti à 1211) ; le port décrémentait toutes les 16 images.

## Corrigé (port)

- Objets 00 et 15 : 32 appels de `step_drop` (au lieu de 16 pour l'objet 00 ; l'objet 15 avait déjà 32), objet cœur : 33 (au lieu de 16), les autres : 43 immobiles puis chute
  de 8 px toutes les 2 images avec le test de pose ci-dessus, l'objet apparaît à la pose et n'est plus ramassable en l'air (le port le faisait tomber à 8 px par image dès la 16e image).
- Repos au sol : 8 x 32 images.
- `y_hall_room5_candles` : bougie 00 retirée 31 images après le coup (était 15), bougie 1A posée 50 à 58 images après (nouveau).
- `test_candles.c` : cœur 33, repos 8 x 32, quatre flammes 26 de traces (moment et Y de pose).

## Dessin des flammes (traces openMSX, porté)

Trace (`scratchpad/fl` de la session, non versionnée : `fl.tcl` = SAT entière avec couleurs de ligne, objets C500 et C800 à chaque image, instantanés VRAM ; hall salle 5, bougie
(64, 160) frappée avec l'objet +5 forcé à 00, 15, 01, 02) : une seule entrée SAT par flamme, 16 lignes de couleur uniforme (CC = couleur, pas de bit OR), X = X de la bougie, mode sprite 2.

| Flamme | Motif (VRAM, base `F800`) | Y de la SAT | Couleur | Suite |
|---|---|---|---|---|
| C500 type 01 (objets 00, 15) | `F4`, `F8` | Y de la bougie (les deux) | 8 fixe | sprite absent les 3 premières images de l'objet (objet vu 994, sprite 997 à 1024), F4 4 images puis F8 8, F4 8, F8 8 (bit 2 du compteur `+6`, une image de retard) |
| C800 type 1E (objet 01) | `F4`, `F8` (les mêmes octets que la flamme de mort : `build_death_flame_md.py` relancé sur cet instantané ne change rien) | Y (F4), Y + 1 (F8) | 8 fixe | visible 995 à 1026 (objet 994 à 1024) : F4 4 images, F8 8, F4 8, F8 8, F4 4 (comme `9B71` : 85/86) |
| C800 type 26 (objets 02 à 1D) | `EC` (disque 11 x 11, un seul motif) | Y + 1, suit la chute (+ 8 toutes les 2 images) | 0E jusqu'à la 5e image de la flamme, puis 0E, 01, 08, 01 toutes les 2 images (la phase continue pendant la chute) | visible 995 jusqu'à la pose (dernier sprite vu à la même image que l'objet 83, 1039) |

Les couleurs sont des numéros de la palette de la salle (hall : 0E = blanc, 01 = chair (R7 G5 B4), 08 = rouge). Port : `burst_sprite` (`candle_view.c`), T = minuteur du drop (coup = 0) :
objet 00/15 : T >= 4, F4 si bit 3 de T nul ; objet 01 : T >= 2, F8 (Y + 1) si bit 3 de T + 2 ; autres : T >= 2, EC (Y + 1), couleur 0E si T < 6 sinon `[0E, 01, 08, 01][(T - 4) >> 1 & 3]`.
Tuiles : F4/F8 = `DEATH_FLAME_TILE_INDEX` (déjà chargées, `actors.h`) ; EC en trois couleurs (`tools/build_candle_flames_md.py` sur un instantané de trace, 3 cellules de 4 tiles) recopiées
dans les 4 tuiles du bloc `DROP_TILE_INDEX` de l'emplacement quand la couleur change (l'objet n'existe pas encore). Scénario `zy_candle_flames` : trois flammes posées en RAM, couleur et motif aux images clés.
La flamme de la torche de la cour (`room_view.c`, `assets/flame_fx`) n'est pas touchée. Tracé dans une seule salle (hall 5), motifs identiques dans tous les stages pour F4/F8 (death-flame.md) ; EC non recoupé ailleurs. Non reproduit : le retard exact d une image entre l'objet et le sprite (le port dessine l'état du minuteur).

## Non porté ou non vérifié

- Dessin : voir la section précédente (porté, `candle_view.c`). Position : le sprite MSX est en (X, Y) de la bougie (SAT, flamme à (X + 8, Y + 16) moins le décalage du sprite), le port dessine donc le sprite à la bougie.
- La phase des décréments du repos (images 987, 1019, 1051... dans la trace) n'est pas reproduite exactement : le port décrémente quand `clock & 31` = 0 (la phase de C003 n'est pas portée).
- Hache sur un bloc (`vk_candles_drop` -> `8992`, objet 1C du C500) : prend maintenant la flamme de son objet (26 pour 1C), comme `89BF` ; non tracé.
- Cœur : trajectoire de 16 px sur 64 images conforme (Y + 1 toutes les 2 images) ; la chute du cœur d'une bougie plus haute que 16 px au-dessus du sol n'a pas été tracée.
- Flammes de la cour (torches, mode 1) : l'objet 01 donne la même flamme 1E de 31 images (tracé), les torches sont dessinées par `room_view.c`, non modifié.
