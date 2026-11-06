# Stage 9 : objet 08, statues 0A, hopper 0B et son os, puce 0D, objets de salle, clé, porte (boss : `stage9-boss.md`)

Code : `sgdk/src/enemies.c` (conditions de stage élargies : canal 08 `v->stage == 9`, statue 0A, hopper 0B stages 7 à 9, puce 0D stages 7 et 9,
base de cellules des puces `fleaBase`), `actors.c` (tuiles `vk_stage9_enemy_tiles`). Aucun objet nouveau : tous les objets ROM de la liste
sont déjà portés (0A = `ENEMY_TYPE_STATUE` 9 et sa boule `FIREBALL` 10, 0B = `HOPPER` 11, os D700 type 4 = `BONE` 13, 0D = `FLEA` 14, 08 = `TYPE_8` 7).
Tuiles : `tools/build_stage9_enemies_md.py` (bloc du stage 8 plus les quatre cellules de puce du stage 7 déplacées en A4, A8, AC, B0, les cellules
84 à 94 du 0C restant celles du stage 8) et contrôle `tools/check_stage9_cells.py` sur les instantanés d'une trace. Bougies, blocs, clés, coffres,
murs : `tools/build_candles_md.py` (`stage > 9` aux deux endroits, `WALL_STAGES = 10`). Tests : `tests/test_stage9_enemies.c` (natif, avec
`tests/stage9_enemy_trace.inc` généré par `tools/gen_stage9_trace.py`), scénario `tests/md_scenarios/zzc_stage9.txt` (+ `zzc_enter9.inc`,
`zzb_next.inc`).

Forcer le stage 9 dans openMSX : `D000` = 9, `D002` = 2 (table `5E6A`, octet de fichier `1E6A`, indice 9 : 2, comme les stages 7 et 8 : mêmes
routines d'objet), `D001` = salle, `C000` = 4. Le stage a 9 salles (0 à 8). Le filtre `D000` de `t5_extra.tcl` accepte 8 et 9.

## Objets (liste `86B6`, stage 9 = troisième sous-liste du groupe 2 ; drapeaux `85A6`[9] = `8606` : `A0 A0 20 20 00 00 00 00 00`)

| Salle | Drapeau | Liste fixe (X, Y) | Fixes et bougies (`candles_md.c`, générés) |
|---|---|---|---|
| 0 | A0 | - | coffre (128, 96) contenu 09 |
| 1 | A0 | 0A (192, 112) | bougie (224, 80) |
| 2 | 20 | 0A (80, 112), 0A (176, 112) | bougies (48, 64) et (96, 64) d'objet 15, (144, 64), (192, 64) |
| 3 | 20 | - | - |
| 4 | 00 | 0A (80, 112), 0B (176, 112) | bougies (96, 64), (144, 64), bloc caché (192, 48), coffre (224, 160) contenu 09, clés (176, 96) et (224, 96) |
| 5 | 00 | 0D (80, 112), 0D (112, 80) | bougies (16, 144), (112, 144), clé (160, 96), coffre (144, 160) contenu 02 |
| 6 | 00 | 0B (64, 80) | bougies (64, 48), (176, 48), (64, 160), coffre (32, 64) contenu 05, clé (144, 128) |
| 7 | 00 | - (salle du boss) | bougies (32, 96) d'objet 14, (96, 144), (144, 144), (192, 144) |
| 8 | 00 | 0B (48, 176), 0D (112, 80) | bougies (144, 48), (144, 128), bloc caché (192, 144), coffre (112, 64) contenu 1E, clé blanche visible (208, 96), porte (12, 128) |

Les bits 7 des drapeaux n'appartiennent à aucun canal (`5EF4`-`5F19` testent les bits 0 à 6) ; le bit 5 (`20`, canal `9DD5`, objet 08) est actif en
salles 0 à 3. Chaque entrée de liste a la forme (type, position) : Y = position & F0, X = (position & 0F) x 16 (`61BB`).

## Prouvé (ROM et trace R : 9 salles, 500 images chacune, Simon immobile ; trace tr_* extraites de `sat_s2.csv`)

**Méthode** de `stage8-enemies.md` : transitions entre deux échantillons consécutifs différents du même bloc, une passe du port à partir du premier,
résultat comparé octet par octet (comparaison PASSE PAR PASSE). Cartes de collision du stage 9 (`vk_stage9_collision`).

| Objet | Transitions | Reproduites en une passe | Autres |
|---|---|---|---|
| hopper 0B (salles 4, 6, 8) | 640 | 634 | 6 échantillons pris en cours de passe (compteur `+12` incrémenté, position pas encore déplacée) |
| os D700 type 4 (salles 4, 8) | 259 | 232 | 27 échantillons en cours de passe (comportement fait, pas déplacé ; ou X seulement déplacé : écriture 16 bits coupée en deux) |

0 écart hors échantillons en cours de passe. Le test natif rejoue 24 transitions du hopper et 6 de l'os sur les cartes du stage 9.

**Objet 08** (canal `9DD5`, salles 0 à 3) : trace R image 843 (salle 0) : X 237, 235, 232, 230, Y 100, 96, 93, 90 ; le scénario donne les mêmes valeurs.
**Statue 0A** : salle 1 en (192, 112), boule en (188, 92) puis X - 4 par passe (trace R images 1378-1386 : BC, B8, B4, B0, AC, A8) : identique en scénario.
**Hopper** : salle 4 en (176, 112) : X 173, 171, 169 (trace R images 2910-2914 : AD.C0, AB.80, A9.40) ; salle 6 en (64, 80) : 61 à la première passe
(trace 3972 : 40.00 -> 3D.C0, Simon en X 16) ; salle 8 en (48, 176) : 50, 52 (trace 5040 : il recule, Simon à moins de 30h). Scénario identique.
**Puces 0D** : salle 5 en (80, 112) et (112, 80) (trace image 3441 : Y 70h X 50h, Y 50h X 70h), salle 8 en (112, 80). Le saut de la puce
dépend du registre `R` du Z80 (`B216`) : non comparé passe par passe ; routine déjà prouvée au stage 7 (ROM exécutée, `research/stage7-enemies.md`).

**Tuiles** (`check_stage9_cells.py`, 400 instantanés VRAM de la trace R) : toute paire de motifs vue en salles 0 à 8 hors boss (08, statue et son
reflet, hopper gauche et droit, os, puces) existe comme masque dans le bloc du stage 9 ; les paires restantes sont celles du boss et de sa boule
(`stage9-boss.md`). Les cellules de puce : A8/AC = cellule A4, B0/B4 = cellule A8.

**Dégâts** (colonne énergie de la trace, tous plans) : voir `stage8-enemies.md` pour les objets 0B, os et boule (mêmes tables `81CE`, `7F3D`).

**Clé blanche, porte** : `kStageItems[8]` = clé visible (208, 96) en salle 8, porte (12, 128) à gauche, sortie vers la salle du boss : voir
`stage9-boss.md` (trace C2 : porte forcée, salle 7 en (246, y)). Scénario `zzd_stage9_boss.txt` : clé prise au contact en (216, 98), porte ouverte, salle 7.

## Déduit ou non vérifié

- **L'objet 1C des bougies d'objet 15 est porté** : voir `stage9-candle-object.md` (salle 2, deux bougies ; mêmes bougies et blocs aux stages 7 et 8).
- Contenus des coffres (09, 02, 05, 1E) : tracés, voir `chests-stage1-9.md` ; blocs cachés (192, 48) en salle 4 et
  (192, 144) en salle 8 : générés par `5A49` / `5B1B`, mécanisme de `wall.c`, non rejoués. Objet 1D de la salle 8 (`DE00` : `vk_wall_items`).
- Hasard de la puce et du lancer d'os (registre `R`) : le port tire un petit générateur ; les valeurs sont celles de la ROM, pas leur ordre.
- Statue et 08 : routines des stages 4 à 8 (mêmes objets, même groupe `D002` = 2) ; une seule séquence par objet comparée ici (scénario), pas
  une comparaison passe par passe de la trace du stage 9.
- Décor des 9 salles : comparé à la VRAM de la trace R, voir `chests-stage1-9.md` (0 tuile différente).
- Fouet et armes lancées sur 0B et l'os : comme au stage 8 (testés en natif, non tracés).
