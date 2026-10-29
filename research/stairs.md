# Escaliers de Vampire Killer MSX2 : mécanisme et positions

Statut : **mécanisme prouvé** (dump Z80 et traces openMSX) ; **positions** prouvées pour 2 volées du stage 1 salle 1, prédites pour les autres par une règle prouvée. ROM de référence : `Vampire Killer (Japan, Europe) (En).rom`, SHA-256 `99eabaae...` (vérifié). Adresses CPU Z80, banque active pendant le jeu.

## 1. Résumé

- **Pas de table d'escaliers séparée** dans les données de salle. Les escaliers sont **des tuiles** : les tuiles 12 (`0x0C`) et 13 (`0x0D`) de la carte 32 x 24 déjà décodée par `tools/extract_rooms.py` (`decode_room`).
- Le code reconnaît une marche sous ou devant Simon par son numéro de tuile, puis le déplace en diagonale à pas fixes. Il n'y a pas de bouton dédié : **HAUT** au pied d'une volée déclenche la montée.
- Une volée montante vers la droite utilise la tuile 12 ; une volée montante vers la gauche utilise la tuile 13.
- Le script `tools/analyze_stairs.py` prédit toutes les volées (stages 1 à 18). Les volées de 1 ou 2 cellules sont exclues : elles sont probablement du décor.

## 2. Mécanisme (code et traces)

### 2.1 Entrée par le bas (prouvé par dump et trace)

| Routine | Rôle | Statut |
|---|---|---|
| `6B7D` | Lit `C007` (entrée) ; bit 0 (HAUT, par `RRA`) déclenche `6BDA` | prouvé (dump, trace : entrée à HAUT) |
| `6BDA` | Sonde 1 `7CDB` (tuile 13, pied-8 en X, rangée y-8). Si pas trouvé, sonde 2 `7D05` (tuile 12). Le registre `B` porte la direction (`C421`) | lu + prouvé (`C421` passe de 0 à 1 dans la trace) |
| `6BEE`-`6BF8` | `C420 := 3`, `C435 := C`, `C421 := B`, `C427` aligné sur 8 px (+8 si `B=1`), `C424 := 0`, `C426 := 0` | lu ; `C420` 00->03 et `C421` 00->01 vu dans la trace à l'image 1953 |
| `7D2F` | Recherche de tuile : index `((y-16)>>3)*32 + (x>>3)` dans le tampon `D100` | lu |

Sondes (tuile 12 et 13, rangée y-8) :
- `7CDB` (direction 0, tuile 13) : teste `(x-8, y-8)` puis `(x, y-8)`. Si trouvé en `(x, y-8)`, `x := x+8`.
- `7D05` (direction 1, tuile 12) : teste `(x+8, y-8)` puis `(x, y-8)`. Si trouvé en `(x, y-8)`, `x := x-8`, puis le `+8` de `6BF8` ramène à `x`.

Conséquence (prouvée par lecture du code, appuyée par la trace) :

| Volée | Fenêtre d'entrée (X des pieds, Y = pieds) | X après alignement |
|---|---|---|
| Tuile 12 (montée droite), cellule basse `(cx, cy)` | `cx-8 <= X <= cx+7`, Y = `cy+8` | `cx` |
| Tuile 13 (montée gauche), cellule basse `(cx, cy)` | `cx <= X <= cx+15`, Y = `cy+8` | `cx+8` |

Test : au stage 1 salle 1, `HAUT` à X=36 ne déclenche pas la volée (saut vertical `6D5D`) ; à X=40 et X=48, la volée démarre à X=48 (trace r2, r3).

### 2.2 Montée et descente (prouvé par dump et trace)

- Routine `6DE0` (mouvement en escalier, actif quand `C421 != 0` ou selon le cas) : les bits de `C007` choisissent la direction. Pour `C421=1` : bit 0 (HAUT) et bit 3 (DROITE) appellent `6E84` (montée : `Y -= DE`, `X += DE`) ; bit 1 (BAS) et bit 2 (GAUCHE) appellent `6E4B` (descente : `Y += DE`, `X -= DE`). Pour `C421=0` : HAUT et GAUCHE montent vers la gauche (`6EBC`), BAS et DROITE descendent (`6E14`).
- `DE = 0x0103` ou `0x100D` (valeur 8.8 ; observé : 1 px sur X et 1 px sur Y toutes les 2 images).
- **Relâcher la touche arrête Simon sur la marche, mais seulement au bord d'une cellule de 8 px** (prouvé, trace `scratchpad/trace3/out3` du 2026-10-29). `6DE2` : tant que `C435 ≠ 0`, `C435` remplace les touches. Chaque pas écrit `C435 := 0` si `C42B & 7 = 0`, sinon la direction du pas (8 = droite, 4 = gauche, codés comme `C007`). L'entrée (`6BEE`) écrit `C435 := C`, la touche : la première cellule est donc parcourue même touche lâchée. Trace : HAUT tenu 4 images au pied (48,192) donne 8 pas jusqu'à (56,184) (images 1999-2017, touches lâchées dès 2003) ; HAUT ou BAS tenus 4 images sur une marche donnent toujours 8 pas. Sur un bord de cellule, l'ordre de lecture est HAUT, BAS, GAUCHE, DROITE (`6DF6`/`6E07`). L'arrêt de la trace r3 (image 1983) tombait sur un bord de cellule. **Porté le 2026-10-29** : `VKPlayer.stair_move` (`player.c`, `vk_stair_tick`), tests `test_physics.c` et `test_gameplay.c`.
- Pendant l'escalier, `C435` porte le pas en cours (voir ci-dessus) et `C42E/C42F` la pose ; `C42B` compte les pas.
- Poses (lecture du code, portées le 2026-10-29, **prouvées par trace** le même jour) : à chaque pas, `C42E/C42F` = `03/01` (vers la droite, `6E14`, `6E84`) ou `0D/10` (vers la gauche, `6E4B`, `6EBC`), puis `6F09` (descente) ou `6F24` (montée) ajoutent selon `c = (C42B >> 2) & 1`, `C42B` avant son incrément : montée `04/01` puis `05/02`, descente `04/01` puis `03/02` (vers la gauche +10/+15), soit une alternance tous les 4 pas (8 images). Entre deux pas la pose reste. La sortie `6EF5` remet `C42B` à 0 et la pose `00/00` (+10/+15). Port : `vk_stair_pose` (`gameplay.c`), `VKGame.stair_steps` (`C42B`), 8 poses `STAIR_POSES` de `tools/extract_player.py` (tuiles 296-391). Test `test_gameplay.c`.
- Trace des poses (`scratchpad/trace3/out2` et `out3`, `tools/run_stair_recoil_trace.py`, valeurs décimales de `C42E/C42F`) : montée à droite (volée A, salle 1 du hall) `C42B` 1-4 `04/01`, 5-8 `05/02`, et de même à chaque cellule (9-16, 25-48) ; descente à gauche `C42B` 17-20 `14/16`, 21-24 `13/17` ; montée à gauche (volée C, pied (240,96)) 1-4 `14/16`, 5-8 `15/17` ; descente à droite 9-12 `04/01`, 13-15 `03/02` (`C42B` après incrément). C'est la règle `c = (C42B >> 2) & 1` avant incrément, +10/+15 vers la gauche. `C42C` vaut 0 vers la droite, 1 vers la gauche. Au 16e pas de la descente à droite, Simon atteint le pied (240,96) : `6EF5` met `C42B` à 0 et la pose `00/00`.
- `C42B` (prouvé, même trace) : l'entrée `6BEE` ne le remet pas à 0 (déjà vu dans `research/down-stairs.md`) ; il continue de compter à travers les changements de sens (8 → 16 → 24 → 48 sans remise à 0) et les coups avec énergie. Seuls `6EF5` (sortie haute ou basse) et l'initialisation de salle `63C3` (`C000 = 4`, au début de partie, à l'entrée du hall et à la réapparition) l'écrivent à 0. Après un coup mortel sur une marche (trace `out4`, `C42B` = 3), Simon quitte l'escalier par le recul avec `C42B` = 3 inchangé jusqu'à la réapparition (`63C3`, image 2329) ; aucun coup n'est possible entre-temps, et le port, qui remet `stair_steps` à 0 dès la sortie, est équivalent.
- **Pas de saut depuis une marche sur MSX** : l'état 3 (`6DDD`) lit `C422` (fouet) puis `6DE0` ; HAUT, la touche de saut, y fait monter, et aucune autre touche ne quitte l'escalier. Le port garde un saut depuis une marche sur le bouton C (choix de manette, `player.c`, test `test_physics.c`) : il n'a pas d'équivalent MSX ; `stair_steps` y est remis à 0.

### 2.3 Sortie (prouvé en partie)

- Sortie haute : `6E44` appelle la sonde de sol `7B88` après 8 pas (`C42B >= 8`). Si un sol solide est trouvé sous les pieds (`x+-5`, `y`), `6EF5` arrête le mode escalier. Simon se pose donc sur le premier sol solide rencontré.
- Fin de sortie : `6EF5` efface `C42B`, `C42E`, `C435`, puis saute en `6D3B`, qui remet `C420` et `C421` à 0 et aligne `C425` (Y) sur un multiple de 8 (`AND F8`). Les deux sorties mesurées ont un Y multiple de 8 (128 et 96), ce qui concorde.
- Mesures : volée A (stage 1 salle 1) arrêtée à **(112,128)** sur le plancher intermédiaire (trace r4, image 2082). Volée B arrêtée à **(176,96)** sur le plancher supérieur (trace r7, image 2190).
- Sol solide : `7C5E` compare `(tuile - 1) & 255` à une limite par stage. La table commence en `7C78`, indexée par `D000` (stage). Stage 1 : limite 4 (tuiles 1 à 4 solides). Stage 0 : limite 2. Les tuiles 12 et 13 ne sont pas solides.
- Prédiction de sortie retenue : `(cx_haut + 16, cy_haut - 8)` pour une volée montante vers la droite. Vérifiée sur deux volées. Pour la montée vers la gauche, la formule symétrique `(cx_haut - 16, cy_haut - 8)` n'est pas vérifiée.

### 2.4 Cas particulier du stage 6

`7C5E`, `7C8B` et `7CDB` testent `CE00 == 6` et prennent alors un autre chemin (`CE00` est à 0 au stage 1). Non étudié : les escaliers du stage 6 sont à vérifier à part.

## 3. Positions prouvées et prédites

### 3.1 Stage 1 salle 1 (prouvé en partie)

- **Volée A** (tuile 12) : départ **(48,192)** en HAUT, montée vers la droite, arrêt **(112,128)** sur le plancher intermédiaire. Trace : `scratchpad/stairs/runs/r4` (image 2082), dump `dump-up30.bin` (`runs/r3`).
- **Volée B** (tuile 12) : départ **(144,128)** en HAUT depuis le plancher intermédiaire, arrêt **(176,96)** sur le plancher supérieur. Trace : `scratchpad/stairs/runs/r7` (image 2190).
- **Volée C** (tuile 13, 8 cellules) : départ prédit **(240,96)** en HAUT, montée vers la gauche jusqu'à `(176,32)`, sortie prédite `(160,24)`, c'est-à-dire la sortie HAUT vers la salle 5 (lien HAUT 1->5 de la table `decode_room_links.py`). **Non vérifié** : la marche à droite depuis (176,96) n'a pas atteint (240,96) (voir section 5).

Pièges constatés dans les traces (voir section 6) : des mouvements qui ressemblent à une pente ont été pris pour des escaliers ; ce sont des reculs.

### 3.2 Table des volées (stages 1 à 18, générée par `tools/analyze_stairs.py`)

Confiance : **prouvé** = trace openMSX ; **déduit** = règle prouvée appliquée à la carte de salle, non tracé. Les volées de 1 ou 2 cellules ne sont pas listées (88 cas, probablement du décor). Stage 0 (cour) non prédit : `decode_room` échoue pour cet étage (pointeurs de salle à vérifier).

| Stage | Salle | Tuile | Direction | Marches | Marche basse (cellule) | Marche haute | Départ pieds (X,Y) | Fenêtre d'entrée | Déclencheur | Sortie haute prévue (X,Y) | Confiance |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 1 | 12 (0x0C) | montee droite | 3 | (144, 120) | (160, 104) | (144,128) | x dans [cx-8, cx+7], cx = 144 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (176,96) | prouvé (trace r7 : entrée x=144, sortie (176,96)) |
| 1 | 1 | 12 (0x0C) | montee droite | 7 | (48, 184) | (96, 136) | (48,192) | x dans [cx-8, cx+7], cx = 48 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (112,128) | prouvé (trace r3, r4 : entrée x=48, sortie (112,128)) |
| 1 | 1 | 13 (0x0D) | montee gauche | 8 | (232, 88) | (176, 32) | (240,96) | x dans [cx, cx+15], cx = 232 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (160,24) | déduit (non tracé : sortie prévue vers salle 5 par le lien HAUT 1->5) |
| 1 | 2 | 12 (0x0C) | montee droite | 7 | (176, 120) | (224, 72) | (176,128) | x dans [cx-8, cx+7], cx = 176 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (240,64) | déduit (table de tuiles, non tracé) |
| 1 | 2 | 13 (0x0D) | montee gauche | 3 | (104, 120) | (88, 104) | (112,128) | x dans [cx, cx+15], cx = 104 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (72,96) | déduit (table de tuiles, non tracé) |
| 1 | 3 | 12 (0x0C) | montee droite | 4 | (48, 56) | (72, 32) | (48,64) | x dans [cx-8, cx+7], cx = 48 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (88,24) | déduit (table de tuiles, non tracé) |
| 1 | 3 | 12 (0x0C) | montee droite | 7 | (48, 184) | (96, 136) | (48,192) | x dans [cx-8, cx+7], cx = 48 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (112,128) | déduit (table de tuiles, non tracé) |
| 1 | 3 | 13 (0x0D) | montee gauche | 7 | (168, 120) | (120, 72) | (176,128) | x dans [cx, cx+15], cx = 168 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (104,64) | déduit (table de tuiles, non tracé) |
| 1 | 4 | 13 (0x0D) | montee gauche | 7 | (200, 120) | (152, 72) | (208,128) | x dans [cx, cx+15], cx = 200 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (136,64) | déduit (table de tuiles, non tracé) |
| 1 | 5 | 13 (0x0D) | montee gauche | 7 | (136, 184) | (88, 136) | (144,192) | x dans [cx, cx+15], cx = 136 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (72,128) | déduit (table de tuiles, non tracé) |
| 1 | 6 | 12 (0x0C) | montee droite | 7 | (112, 184) | (160, 136) | (112,192) | x dans [cx-8, cx+7], cx = 112 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (176,128) | déduit (table de tuiles, non tracé) |
| 1 | 7 | 12 (0x0C) | montee droite | 3 | (144, 120) | (160, 104) | (144,128) | x dans [cx-8, cx+7], cx = 144 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (176,96) | déduit (table de tuiles, non tracé) |
| 2 | 0 | 12 (0x0C) | montee droite | 7 | (48, 184) | (96, 136) | (48,192) | x dans [cx-8, cx+7], cx = 48 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (112,128) | déduit (table de tuiles, non tracé) |
| 2 | 0 | 12 (0x0C) | montee droite | 8 | (48, 88) | (104, 32) | (48,96) | x dans [cx-8, cx+7], cx = 48 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (120,24) | déduit (table de tuiles, non tracé) |
| 2 | 1 | 13 (0x0D) | montee gauche | 7 | (232, 184) | (184, 136) | (240,192) | x dans [cx, cx+15], cx = 232 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (168,128) | déduit (table de tuiles, non tracé) |
| 2 | 2 | 12 (0x0C) | montee droite | 5 | (96, 200) | (128, 168) | (96,208) | x dans [cx-8, cx+7], cx = 96 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (144,160) | déduit (table de tuiles, non tracé) |
| 2 | 3 | 13 (0x0D) | montee gauche | 11 | (168, 184) | (88, 104) | (176,192) | x dans [cx, cx+15], cx = 168 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (72,96) | déduit (table de tuiles, non tracé) |
| 2 | 4 | 13 (0x0D) | montee gauche | 6 | (88, 72) | (48, 32) | (96,80) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (32,24) | déduit (table de tuiles, non tracé) |
| 2 | 5 | 12 (0x0C) | montee droite | 12 | (112, 120) | (200, 32) | (112,128) | x dans [cx-8, cx+7], cx = 112 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (216,24) | déduit (table de tuiles, non tracé) |
| 3 | 0 | 12 (0x0C) | montee droite | 3 | (48, 88) | (64, 72) | (48,96) | x dans [cx-8, cx+7], cx = 48 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (80,64) | déduit (table de tuiles, non tracé) |
| 3 | 0 | 13 (0x0D) | montee gauche | 7 | (200, 184) | (152, 136) | (208,192) | x dans [cx, cx+15], cx = 200 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (136,128) | déduit (table de tuiles, non tracé) |
| 3 | 1 | 12 (0x0C) | montee droite | 7 | (144, 120) | (192, 72) | (144,128) | x dans [cx-8, cx+7], cx = 144 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (208,64) | déduit (table de tuiles, non tracé) |
| 3 | 1 | 12 (0x0C) | montee droite | 7 | (48, 184) | (96, 136) | (48,192) | x dans [cx-8, cx+7], cx = 48 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (112,128) | déduit (table de tuiles, non tracé) |
| 3 | 1 | 13 (0x0D) | montee gauche | 3 | (232, 184) | (216, 168) | (240,192) | x dans [cx, cx+15], cx = 232 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (200,160) | déduit (table de tuiles, non tracé) |
| 3 | 4 | 12 (0x0C) | montee droite | 12 | (80, 120) | (168, 32) | (80,128) | x dans [cx-8, cx+7], cx = 80 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (184,24) | déduit (table de tuiles, non tracé) |
| 3 | 4 | 13 (0x0D) | montee gauche | 7 | (168, 184) | (120, 136) | (176,192) | x dans [cx, cx+15], cx = 168 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (104,128) | déduit (table de tuiles, non tracé) |
| 3 | 5 | 12 (0x0C) | montee droite | 3 | (176, 152) | (192, 136) | (176,160) | x dans [cx-8, cx+7], cx = 176 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (208,128) | déduit (table de tuiles, non tracé) |
| 4 | 1 | 12 (0x0C) | montee droite | 11 | (48, 184) | (128, 104) | (48,192) | x dans [cx-8, cx+7], cx = 48 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (144,96) | déduit (table de tuiles, non tracé) |
| 4 | 1 | 12 (0x0C) | montee droite | 8 | (80, 88) | (136, 32) | (80,96) | x dans [cx-8, cx+7], cx = 80 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (152,24) | déduit (table de tuiles, non tracé) |
| 4 | 2 | 12 (0x0C) | montee droite | 7 | (112, 184) | (160, 136) | (112,192) | x dans [cx-8, cx+7], cx = 112 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (176,128) | déduit (table de tuiles, non tracé) |
| 4 | 2 | 12 (0x0C) | montee droite | 8 | (144, 88) | (200, 32) | (144,96) | x dans [cx-8, cx+7], cx = 144 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (216,24) | déduit (table de tuiles, non tracé) |
| 4 | 2 | 12 (0x0C) | montee droite | 8 | (16, 88) | (72, 32) | (16,96) | x dans [cx-8, cx+7], cx = 16 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (88,24) | déduit (table de tuiles, non tracé) |
| 4 | 5 | 12 (0x0C) | montee droite | 9 | (64, 200) | (128, 136) | (64,208) | x dans [cx-8, cx+7], cx = 64 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (144,128) | déduit (table de tuiles, non tracé) |
| 5 | 2 | 12 (0x0C) | montee droite | 4 | (48, 56) | (72, 32) | (48,64) | x dans [cx-8, cx+7], cx = 48 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (88,24) | déduit (table de tuiles, non tracé) |
| 5 | 5 | 12 (0x0C) | montee droite | 3 | (64, 200) | (80, 184) | (64,208) | x dans [cx-8, cx+7], cx = 64 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (96,176) | déduit (table de tuiles, non tracé) |
| 6 | 0 | 12 (0x0C) | montee droite | 4 | (112, 56) | (136, 32) | (112,64) | x dans [cx-8, cx+7], cx = 112 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (152,24) | déduit (table de tuiles, non tracé) |
| 6 | 2 | 12 (0x0C) | montee droite | 3 | (96, 136) | (112, 120) | (96,144) | x dans [cx-8, cx+7], cx = 96 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (128,112) | déduit (table de tuiles, non tracé) |
| 6 | 3 | 12 (0x0C) | montee droite | 5 | (128, 200) | (160, 168) | (128,208) | x dans [cx-8, cx+7], cx = 128 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (176,160) | déduit (table de tuiles, non tracé) |
| 7 | 1 | 12 (0x0C) | montee droite | 7 | (160, 136) | (208, 88) | (160,144) | x dans [cx-8, cx+7], cx = 160 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (224,80) | déduit (table de tuiles, non tracé) |
| 7 | 2 | 12 (0x0C) | montee droite | 7 | (96, 136) | (144, 88) | (96,144) | x dans [cx-8, cx+7], cx = 96 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (160,80) | déduit (table de tuiles, non tracé) |
| 7 | 2 | 13 (0x0D) | montee gauche | 6 | (184, 72) | (144, 32) | (192,80) | x dans [cx, cx+15], cx = 184 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (128,24) | déduit (table de tuiles, non tracé) |
| 7 | 3 | 12 (0x0C) | montee droite | 9 | (80, 184) | (144, 120) | (80,192) | x dans [cx-8, cx+7], cx = 80 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (160,112) | déduit (table de tuiles, non tracé) |
| 7 | 5 | 13 (0x0D) | montee gauche | 10 | (152, 104) | (80, 32) | (160,112) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (64,24) | déduit (table de tuiles, non tracé) |
| 7 | 5 | 13 (0x0D) | montee gauche | 3 | (152, 200) | (136, 184) | (160,208) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (120,176) | déduit (table de tuiles, non tracé) |
| 7 | 6 | 13 (0x0D) | montee gauche | 7 | (120, 168) | (72, 120) | (128,176) | x dans [cx, cx+15], cx = 120 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (56,112) | déduit (table de tuiles, non tracé) |
| 7 | 7 | 12 (0x0C) | montee droite | 3 | (128, 168) | (144, 152) | (128,176) | x dans [cx-8, cx+7], cx = 128 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (160,144) | déduit (table de tuiles, non tracé) |
| 7 | 8 | 12 (0x0C) | montee droite | 7 | (64, 168) | (112, 120) | (64,176) | x dans [cx-8, cx+7], cx = 64 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (128,112) | déduit (table de tuiles, non tracé) |
| 7 | 8 | 13 (0x0D) | montee gauche | 3 | (88, 200) | (72, 184) | (96,208) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (56,176) | déduit (table de tuiles, non tracé) |
| 8 | 2 | 12 (0x0C) | montee droite | 7 | (128, 136) | (176, 88) | (128,144) | x dans [cx-8, cx+7], cx = 128 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (192,80) | déduit (table de tuiles, non tracé) |
| 8 | 2 | 13 (0x0D) | montee gauche | 6 | (184, 72) | (144, 32) | (192,80) | x dans [cx, cx+15], cx = 184 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (128,24) | déduit (table de tuiles, non tracé) |
| 8 | 3 | 12 (0x0C) | montee droite | 7 | (160, 136) | (208, 88) | (160,144) | x dans [cx-8, cx+7], cx = 160 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (224,80) | déduit (table de tuiles, non tracé) |
| 8 | 4 | 12 (0x0C) | montee droite | 11 | (96, 168) | (176, 88) | (96,176) | x dans [cx-8, cx+7], cx = 96 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (192,80) | déduit (table de tuiles, non tracé) |
| 8 | 5 | 13 (0x0D) | montee gauche | 7 | (152, 200) | (104, 152) | (160,208) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (88,144) | déduit (table de tuiles, non tracé) |
| 8 | 7 | 13 (0x0D) | montee gauche | 5 | (72, 184) | (40, 152) | (80,192) | x dans [cx, cx+15], cx = 72 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (24,144) | déduit (table de tuiles, non tracé) |
| 9 | 5 | 13 (0x0D) | montee gauche | 3 | (56, 200) | (40, 184) | (64,208) | x dans [cx, cx+15], cx = 56 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (24,176) | déduit (table de tuiles, non tracé) |
| 9 | 5 | 13 (0x0D) | montee gauche | 6 | (88, 72) | (48, 32) | (96,80) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (32,24) | déduit (table de tuiles, non tracé) |
| 9 | 5 | 13 (0x0D) | montee gauche | 7 | (88, 168) | (40, 120) | (96,176) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (24,112) | déduit (table de tuiles, non tracé) |
| 9 | 6 | 12 (0x0C) | montee droite | 12 | (32, 168) | (120, 80) | (32,176) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (136,72) | déduit (table de tuiles, non tracé) |
| 9 | 6 | 13 (0x0D) | montee gauche | 3 | (56, 200) | (40, 184) | (64,208) | x dans [cx, cx+15], cx = 56 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (24,176) | déduit (table de tuiles, non tracé) |
| 9 | 8 | 13 (0x0D) | montee gauche | 6 | (88, 72) | (48, 32) | (96,80) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (32,24) | déduit (table de tuiles, non tracé) |
| 9 | 8 | 13 (0x0D) | montee gauche | 7 | (120, 168) | (72, 120) | (128,176) | x dans [cx, cx+15], cx = 120 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (56,112) | déduit (table de tuiles, non tracé) |
| 10 | 1 | 13 (0x0D) | montee gauche | 3 | (120, 104) | (104, 88) | (128,112) | x dans [cx, cx+15], cx = 120 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (88,80) | déduit (table de tuiles, non tracé) |
| 10 | 1 | 13 (0x0D) | montee gauche | 6 | (184, 72) | (144, 32) | (192,80) | x dans [cx, cx+15], cx = 184 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (128,24) | déduit (table de tuiles, non tracé) |
| 10 | 3 | 12 (0x0C) | montee droite | 7 | (32, 136) | (80, 88) | (32,144) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (96,80) | déduit (table de tuiles, non tracé) |
| 10 | 5 | 13 (0x0D) | montee gauche | 6 | (184, 72) | (144, 32) | (192,80) | x dans [cx, cx+15], cx = 184 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (128,24) | déduit (table de tuiles, non tracé) |
| 10 | 6 | 13 (0x0D) | montee gauche | 3 | (152, 200) | (136, 184) | (160,208) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (120,176) | déduit (table de tuiles, non tracé) |
| 10 | 8 | 13 (0x0D) | montee gauche | 3 | (152, 200) | (136, 184) | (160,208) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (120,176) | déduit (table de tuiles, non tracé) |
| 13 | 0 | 13 (0x0D) | montee gauche | 6 | (120, 72) | (80, 32) | (128,80) | x dans [cx, cx+15], cx = 120 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (64,24) | déduit (table de tuiles, non tracé) |
| 13 | 1 | 12 (0x0C) | montee droite | 7 | (96, 168) | (144, 120) | (96,176) | x dans [cx-8, cx+7], cx = 96 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (160,112) | déduit (table de tuiles, non tracé) |
| 13 | 1 | 13 (0x0D) | montee gauche | 3 | (152, 104) | (136, 88) | (160,112) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (120,80) | déduit (table de tuiles, non tracé) |
| 13 | 2 | 12 (0x0C) | montee droite | 3 | (160, 200) | (176, 184) | (160,208) | x dans [cx-8, cx+7], cx = 160 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (192,176) | déduit (table de tuiles, non tracé) |
| 13 | 2 | 12 (0x0C) | montee droite | 6 | (96, 72) | (136, 32) | (96,80) | x dans [cx-8, cx+7], cx = 96 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (152,24) | déduit (table de tuiles, non tracé) |
| 13 | 2 | 13 (0x0D) | montee gauche | 7 | (88, 136) | (40, 88) | (96,144) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (24,80) | déduit (table de tuiles, non tracé) |
| 13 | 3 | 13 (0x0D) | montee gauche | 3 | (88, 200) | (72, 184) | (96,208) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (56,176) | déduit (table de tuiles, non tracé) |
| 13 | 3 | 13 (0x0D) | montee gauche | 7 | (184, 136) | (136, 88) | (192,144) | x dans [cx, cx+15], cx = 184 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (120,80) | déduit (table de tuiles, non tracé) |
| 13 | 4 | 13 (0x0D) | montee gauche | 7 | (152, 136) | (104, 88) | (160,144) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (88,80) | déduit (table de tuiles, non tracé) |
| 13 | 5 | 12 (0x0C) | montee droite | 3 | (128, 104) | (144, 88) | (128,112) | x dans [cx-8, cx+7], cx = 128 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (160,80) | déduit (table de tuiles, non tracé) |
| 13 | 5 | 12 (0x0C) | montee droite | 3 | (128, 200) | (144, 184) | (128,208) | x dans [cx-8, cx+7], cx = 128 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (160,176) | déduit (table de tuiles, non tracé) |
| 13 | 5 | 12 (0x0C) | montee droite | 7 | (32, 168) | (80, 120) | (32,176) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (96,112) | déduit (table de tuiles, non tracé) |
| 13 | 6 | 13 (0x0D) | montee gauche | 6 | (120, 72) | (80, 32) | (128,80) | x dans [cx, cx+15], cx = 120 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (64,24) | déduit (table de tuiles, non tracé) |
| 13 | 7 | 12 (0x0C) | montee droite | 3 | (64, 200) | (80, 184) | (64,208) | x dans [cx-8, cx+7], cx = 64 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (96,176) | déduit (table de tuiles, non tracé) |
| 13 | 7 | 12 (0x0C) | montee droite | 6 | (128, 72) | (168, 32) | (128,80) | x dans [cx-8, cx+7], cx = 128 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (184,24) | déduit (table de tuiles, non tracé) |
| 13 | 7 | 13 (0x0D) | montee gauche | 11 | (152, 168) | (72, 88) | (160,176) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (56,80) | déduit (table de tuiles, non tracé) |
| 13 | 8 | 12 (0x0C) | montee droite | 3 | (96, 104) | (112, 88) | (96,112) | x dans [cx-8, cx+7], cx = 96 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (128,80) | déduit (table de tuiles, non tracé) |
| 13 | 9 | 12 (0x0C) | montee droite | 6 | (32, 72) | (72, 32) | (32,80) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (88,24) | déduit (table de tuiles, non tracé) |
| 13 | 9 | 12 (0x0C) | montee droite | 7 | (160, 168) | (208, 120) | (160,176) | x dans [cx-8, cx+7], cx = 160 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (224,112) | déduit (table de tuiles, non tracé) |
| 13 | 9 | 13 (0x0D) | montee gauche | 3 | (152, 104) | (136, 88) | (160,112) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (120,80) | déduit (table de tuiles, non tracé) |
| 13 | 10 | 12 (0x0C) | montee droite | 3 | (32, 104) | (48, 88) | (32,112) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (64,80) | déduit (table de tuiles, non tracé) |
| 13 | 10 | 13 (0x0D) | montee gauche | 3 | (152, 200) | (136, 184) | (160,208) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (120,176) | déduit (table de tuiles, non tracé) |
| 13 | 11 | 13 (0x0D) | montee gauche | 3 | (88, 200) | (72, 184) | (96,208) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (56,176) | déduit (table de tuiles, non tracé) |
| 13 | 11 | 13 (0x0D) | montee gauche | 6 | (184, 72) | (144, 32) | (192,80) | x dans [cx, cx+15], cx = 184 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (128,24) | déduit (table de tuiles, non tracé) |
| 13 | 11 | 13 (0x0D) | montee gauche | 7 | (152, 168) | (104, 120) | (160,176) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (88,112) | déduit (table de tuiles, non tracé) |
| 14 | 0 | 12 (0x0C) | montee droite | 6 | (64, 72) | (104, 32) | (64,80) | x dans [cx-8, cx+7], cx = 64 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (120,24) | déduit (table de tuiles, non tracé) |
| 14 | 0 | 12 (0x0C) | montee droite | 7 | (64, 168) | (112, 120) | (64,176) | x dans [cx-8, cx+7], cx = 64 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (128,112) | déduit (table de tuiles, non tracé) |
| 14 | 0 | 13 (0x0D) | montee gauche | 3 | (120, 104) | (104, 88) | (128,112) | x dans [cx, cx+15], cx = 120 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (88,80) | déduit (table de tuiles, non tracé) |
| 14 | 2 | 12 (0x0C) | montee droite | 7 | (96, 136) | (144, 88) | (96,144) | x dans [cx-8, cx+7], cx = 96 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (160,80) | déduit (table de tuiles, non tracé) |
| 14 | 3 | 12 (0x0C) | montee droite | 11 | (64, 168) | (144, 88) | (64,176) | x dans [cx-8, cx+7], cx = 64 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (160,80) | déduit (table de tuiles, non tracé) |
| 14 | 3 | 12 (0x0C) | montee droite | 3 | (96, 200) | (112, 184) | (96,208) | x dans [cx-8, cx+7], cx = 96 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (128,176) | déduit (table de tuiles, non tracé) |
| 14 | 4 | 13 (0x0D) | montee gauche | 7 | (88, 136) | (40, 88) | (96,144) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (24,80) | déduit (table de tuiles, non tracé) |
| 14 | 5 | 13 (0x0D) | montee gauche | 7 | (216, 168) | (168, 120) | (224,176) | x dans [cx, cx+15], cx = 216 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (152,112) | déduit (table de tuiles, non tracé) |
| 14 | 6 | 13 (0x0D) | montee gauche | 7 | (152, 168) | (104, 120) | (160,176) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (88,112) | déduit (table de tuiles, non tracé) |
| 14 | 7 | 13 (0x0D) | montee gauche | 11 | (152, 168) | (72, 88) | (160,176) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (56,80) | déduit (table de tuiles, non tracé) |
| 15 | 0 | 12 (0x0C) | montee droite | 6 | (128, 72) | (168, 32) | (128,80) | x dans [cx-8, cx+7], cx = 128 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (184,24) | déduit (table de tuiles, non tracé) |
| 15 | 1 | 13 (0x0D) | montee gauche | 11 | (152, 168) | (72, 88) | (160,176) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (56,80) | déduit (table de tuiles, non tracé) |
| 15 | 2 | 12 (0x0C) | montee droite | 3 | (160, 200) | (176, 184) | (160,208) | x dans [cx-8, cx+7], cx = 160 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (192,176) | déduit (table de tuiles, non tracé) |
| 15 | 2 | 12 (0x0C) | montee droite | 6 | (128, 72) | (168, 32) | (128,80) | x dans [cx-8, cx+7], cx = 128 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (184,24) | déduit (table de tuiles, non tracé) |
| 15 | 3 | 12 (0x0C) | montee droite | 7 | (128, 136) | (176, 88) | (128,144) | x dans [cx-8, cx+7], cx = 128 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (192,80) | déduit (table de tuiles, non tracé) |
| 15 | 4 | 12 (0x0C) | montee droite | 3 | (160, 200) | (176, 184) | (160,208) | x dans [cx-8, cx+7], cx = 160 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (192,176) | déduit (table de tuiles, non tracé) |
| 15 | 4 | 12 (0x0C) | montee droite | 6 | (64, 72) | (104, 32) | (64,80) | x dans [cx-8, cx+7], cx = 64 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (120,24) | déduit (table de tuiles, non tracé) |
| 15 | 4 | 12 (0x0C) | montee droite | 7 | (32, 168) | (80, 120) | (32,176) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (96,112) | déduit (table de tuiles, non tracé) |
| 15 | 7 | 12 (0x0C) | montee droite | 3 | (96, 200) | (112, 184) | (96,208) | x dans [cx-8, cx+7], cx = 96 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (128,176) | déduit (table de tuiles, non tracé) |
| 15 | 9 | 12 (0x0C) | montee droite | 3 | (32, 168) | (48, 152) | (32,176) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (64,144) | déduit (table de tuiles, non tracé) |
| 15 | 9 | 13 (0x0D) | montee gauche | 3 | (216, 168) | (200, 152) | (224,176) | x dans [cx, cx+15], cx = 216 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (184,144) | déduit (table de tuiles, non tracé) |
| 16 | 0 | 12 (0x0C) | montee droite | 3 | (64, 200) | (80, 184) | (64,208) | x dans [cx-8, cx+7], cx = 64 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (96,176) | déduit (table de tuiles, non tracé) |
| 16 | 0 | 13 (0x0D) | montee gauche | 7 | (152, 168) | (104, 120) | (160,176) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (88,112) | déduit (table de tuiles, non tracé) |
| 16 | 6 | 12 (0x0C) | montee droite | 6 | (32, 72) | (72, 32) | (32,80) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (88,24) | déduit (table de tuiles, non tracé) |
| 16 | 6 | 13 (0x0D) | montee gauche | 11 | (184, 168) | (104, 88) | (192,176) | x dans [cx, cx+15], cx = 184 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (88,80) | déduit (table de tuiles, non tracé) |
| 17 | 0 | 12 (0x0C) | montee droite | 6 | (96, 72) | (136, 32) | (96,80) | x dans [cx-8, cx+7], cx = 96 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (152,24) | déduit (table de tuiles, non tracé) |
| 17 | 0 | 13 (0x0D) | montee gauche | 7 | (88, 168) | (40, 120) | (96,176) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (24,112) | déduit (table de tuiles, non tracé) |
| 17 | 2 | 12 (0x0C) | montee droite | 7 | (160, 168) | (208, 120) | (160,176) | x dans [cx-8, cx+7], cx = 160 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (224,112) | déduit (table de tuiles, non tracé) |
| 17 | 2 | 13 (0x0D) | montee gauche | 10 | (216, 104) | (144, 32) | (224,112) | x dans [cx, cx+15], cx = 216 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (128,24) | déduit (table de tuiles, non tracé) |
| 17 | 3 | 12 (0x0C) | montee droite | 3 | (128, 200) | (144, 184) | (128,208) | x dans [cx-8, cx+7], cx = 128 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (160,176) | déduit (table de tuiles, non tracé) |
| 17 | 3 | 12 (0x0C) | montee droite | 7 | (128, 168) | (176, 120) | (128,176) | x dans [cx-8, cx+7], cx = 128 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (192,112) | déduit (table de tuiles, non tracé) |
| 17 | 4 | 13 (0x0D) | montee gauche | 3 | (88, 104) | (72, 88) | (96,112) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (56,80) | déduit (table de tuiles, non tracé) |
| 17 | 5 | 12 (0x0C) | montee droite | 7 | (96, 136) | (144, 88) | (96,144) | x dans [cx-8, cx+7], cx = 96 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (160,80) | déduit (table de tuiles, non tracé) |
| 17 | 5 | 13 (0x0D) | montee gauche | 3 | (152, 200) | (136, 184) | (160,208) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (120,176) | déduit (table de tuiles, non tracé) |
| 17 | 5 | 13 (0x0D) | montee gauche | 6 | (120, 72) | (80, 32) | (128,80) | x dans [cx, cx+15], cx = 120 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (64,24) | déduit (table de tuiles, non tracé) |
| 17 | 6 | 13 (0x0D) | montee gauche | 11 | (120, 168) | (40, 88) | (128,176) | x dans [cx, cx+15], cx = 120 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (24,80) | déduit (table de tuiles, non tracé) |
| 17 | 6 | 13 (0x0D) | montee gauche | 3 | (88, 200) | (72, 184) | (96,208) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (56,176) | déduit (table de tuiles, non tracé) |
| 17 | 6 | 13 (0x0D) | montee gauche | 4 | (72, 56) | (48, 32) | (80,64) | x dans [cx, cx+15], cx = 72 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (32,24) | déduit (table de tuiles, non tracé) |
| 17 | 7 | 12 (0x0C) | montee droite | 6 | (32, 72) | (72, 32) | (32,80) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (88,24) | déduit (table de tuiles, non tracé) |
| 17 | 8 | 12 (0x0C) | montee droite | 6 | (64, 72) | (104, 32) | (64,80) | x dans [cx-8, cx+7], cx = 64 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (120,24) | déduit (table de tuiles, non tracé) |
| 17 | 8 | 13 (0x0D) | montee gauche | 11 | (216, 168) | (136, 88) | (224,176) | x dans [cx, cx+15], cx = 216 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (120,80) | déduit (table de tuiles, non tracé) |
| 17 | 9 | 13 (0x0D) | montee gauche | 3 | (56, 200) | (40, 184) | (64,208) | x dans [cx, cx+15], cx = 56 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (24,176) | déduit (table de tuiles, non tracé) |
| 17 | 10 | 12 (0x0C) | montee droite | 3 | (64, 200) | (80, 184) | (64,208) | x dans [cx-8, cx+7], cx = 64 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (96,176) | déduit (table de tuiles, non tracé) |
| 17 | 10 | 13 (0x0D) | montee gauche | 3 | (88, 168) | (72, 152) | (96,176) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (56,144) | déduit (table de tuiles, non tracé) |
| 17 | 11 | 12 (0x0C) | montee droite | 3 | (96, 200) | (112, 184) | (96,208) | x dans [cx-8, cx+7], cx = 96 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (128,176) | déduit (table de tuiles, non tracé) |
| 17 | 11 | 13 (0x0D) | montee gauche | 7 | (120, 168) | (72, 120) | (128,176) | x dans [cx, cx+15], cx = 120 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (56,112) | déduit (table de tuiles, non tracé) |
| 18 | 0 | 13 (0x0D) | montee gauche | 7 | (152, 168) | (104, 120) | (160,176) | x dans [cx, cx+15], cx = 152 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (88,112) | déduit (table de tuiles, non tracé) |
| 18 | 1 | 12 (0x0C) | montee droite | 7 | (128, 136) | (176, 88) | (128,144) | x dans [cx-8, cx+7], cx = 128 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (192,80) | déduit (table de tuiles, non tracé) |
| 18 | 2 | 12 (0x0C) | montee droite | 6 | (32, 72) | (72, 32) | (32,80) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (88,24) | déduit (table de tuiles, non tracé) |
| 18 | 2 | 13 (0x0D) | montee gauche | 11 | (184, 168) | (104, 88) | (192,176) | x dans [cx, cx+15], cx = 184 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (88,80) | déduit (table de tuiles, non tracé) |
| 18 | 2 | 13 (0x0D) | montee gauche | 6 | (216, 72) | (176, 32) | (224,80) | x dans [cx, cx+15], cx = 216 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (160,24) | déduit (table de tuiles, non tracé) |
| 18 | 3 | 12 (0x0C) | montee droite | 3 | (64, 200) | (80, 184) | (64,208) | x dans [cx-8, cx+7], cx = 64 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (96,176) | déduit (table de tuiles, non tracé) |
| 18 | 3 | 12 (0x0C) | montee droite | 4 | (144, 56) | (168, 32) | (144,64) | x dans [cx-8, cx+7], cx = 144 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (184,24) | déduit (table de tuiles, non tracé) |
| 18 | 3 | 12 (0x0C) | montee droite | 7 | (32, 168) | (80, 120) | (32,176) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (96,112) | déduit (table de tuiles, non tracé) |
| 18 | 3 | 13 (0x0D) | montee gauche | 3 | (184, 200) | (168, 184) | (192,208) | x dans [cx, cx+15], cx = 184 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (152,176) | déduit (table de tuiles, non tracé) |
| 18 | 3 | 13 (0x0D) | montee gauche | 5 | (216, 104) | (184, 72) | (224,112) | x dans [cx, cx+15], cx = 216 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (168,64) | déduit (table de tuiles, non tracé) |
| 18 | 3 | 13 (0x0D) | montee gauche | 7 | (216, 168) | (168, 120) | (224,176) | x dans [cx, cx+15], cx = 216 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (152,112) | déduit (table de tuiles, non tracé) |
| 18 | 3 | 13 (0x0D) | montee gauche | 9 | (104, 104) | (40, 40) | (112,112) | x dans [cx, cx+15], cx = 104 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (24,32) | déduit (table de tuiles, non tracé) |
| 18 | 4 | 12 (0x0C) | montee droite | 14 | (32, 136) | (136, 32) | (32,144) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (152,24) | déduit (table de tuiles, non tracé) |
| 18 | 4 | 12 (0x0C) | montee droite | 6 | (32, 72) | (72, 32) | (32,80) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (88,24) | déduit (table de tuiles, non tracé) |
| 18 | 5 | 12 (0x0C) | montee droite | 7 | (160, 200) | (208, 152) | (160,208) | x dans [cx-8, cx+7], cx = 160 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (224,144) | déduit (table de tuiles, non tracé) |
| 18 | 6 | 12 (0x0C) | montee droite | 3 | (160, 104) | (176, 88) | (160,112) | x dans [cx-8, cx+7], cx = 160 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (192,80) | déduit (table de tuiles, non tracé) |
| 18 | 6 | 12 (0x0C) | montee droite | 3 | (64, 200) | (80, 184) | (64,208) | x dans [cx-8, cx+7], cx = 64 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (96,176) | déduit (table de tuiles, non tracé) |
| 18 | 6 | 12 (0x0C) | montee droite | 7 | (128, 200) | (176, 152) | (128,208) | x dans [cx-8, cx+7], cx = 128 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (192,144) | déduit (table de tuiles, non tracé) |
| 18 | 6 | 12 (0x0C) | montee droite | 7 | (32, 168) | (80, 120) | (32,176) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (96,112) | déduit (table de tuiles, non tracé) |
| 18 | 6 | 13 (0x0D) | montee gauche | 3 | (200, 136) | (184, 120) | (208,144) | x dans [cx, cx+15], cx = 200 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (168,112) | déduit (table de tuiles, non tracé) |
| 18 | 6 | 13 (0x0D) | montee gauche | 3 | (88, 104) | (72, 88) | (96,112) | x dans [cx, cx+15], cx = 88 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (56,80) | déduit (table de tuiles, non tracé) |
| 18 | 6 | 13 (0x0D) | montee gauche | 6 | (216, 72) | (176, 32) | (224,80) | x dans [cx, cx+15], cx = 216 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (160,24) | déduit (table de tuiles, non tracé) |
| 18 | 7 | 12 (0x0C) | montee droite | 3 | (32, 104) | (48, 88) | (32,112) | x dans [cx-8, cx+7], cx = 32 | HAUT au pied ; puis HAUT ou DROITE pour monter, BAS ou GAUCHE pour descendre | (64,80) | déduit (table de tuiles, non tracé) |
| 18 | 7 | 13 (0x0D) | montee gauche | 3 | (184, 200) | (168, 184) | (192,208) | x dans [cx, cx+15], cx = 184 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (152,176) | déduit (table de tuiles, non tracé) |
| 18 | 7 | 13 (0x0D) | montee gauche | 7 | (216, 168) | (168, 120) | (224,176) | x dans [cx, cx+15], cx = 216 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (152,112) | déduit (table de tuiles, non tracé) |
| 18 | 8 | 13 (0x0D) | montee gauche | 11 | (184, 168) | (104, 88) | (192,176) | x dans [cx, cx+15], cx = 184 | HAUT au pied ; puis HAUT ou GAUCHE pour monter, BAS ou DROITE pour descendre | (88,80) | déduit (table de tuiles, non tracé) |

## 4. Méthode

1. Décodage des cartes : `tools/extract_rooms.py` (`decode_room`), sans écriture dans le dépôt.
2. Recherche des tuiles 12 et 13 et chaînage en volées : `tools/analyze_stairs.py`. Commande : `python3 -I tools/analyze_stairs.py "scratchpad/stairs/Vampire Killer (Japan, Europe) (En).rom"`.
3. Dump Z80 pendant une montée (commande `dump` de `scratchpad/stairs/work/stairwalk.tcl`) et désassemblage avec `z80dis` (`scratchpad/stairs/work/dis2.py`).
4. Trace openMSX : `scratchpad/stairs/work/run_stairs.py` (plans `plan3` à `plan8`). Journal des écritures de `C425`, `C426`, `C427` avec PC, et diff de `C400-C43F` par image (`blocks.txt`).
5. Recherche des références aux octets d'état (`C420`, `C421`, `C422`, `C435`, `C42B`) dans le dump.

## 5. Inconnues

1. **Entrée par le haut (BAS)** : la routine `6C10` appelle `7C8B` et `7CB3`, qui testent la tuile **4** à la rangée des pieds (`(x, y)` puis `(x+8, y)`). Non tracé. Stage 1 salle 1 n'a aucune tuile 4 : le test ne peut pas être vérifié dans cette salle. D'autres salles en ont (voir `analyze_stairs.py`, colonne tuile 4 du survey).
2. **Sortie** : position exacte d'arrêt sur un sol déjà solide, et comportement après la sortie. Dans la trace r8, DROITE sur le plancher supérieur, juste après la sortie de la volée B, a relancé une montée. L'état `C421` n'est pas remis à zéro immédiatement. À vérifier.
3. **Volée C** (salle 1, stage 1) : entrée et sortie non tracées.
4. **Stages 2 à 18** : aucune volée tracée ; la prédiction repose sur la règle. La règle suppose que les numéros de tuile 12 et 13 ont le même sens dans tous les tilesets. Non vérifié.
5. **Stage 6** (`CE00 == 6`) : chemin différent, non étudié.
6. **Bits de `C007`** : mapping BAS/GAUCHE/DROITE déduit du code (`6DE0`), seul HAUT a été vérifié par trace.
7. **Pas exact** : `DE` vaut `0x0103` ou `0x100D` selon l'appel ; la dérive sur une volée complète n'a pas été mesurée au pixel.

## 6. Pièges (faux escaliers)

- **Recul ennemi** : sur la salle 1 du stage 1, à X=40 et Y=192, un contact ennemi produit une trajectoire diagonale vers la gauche (trace r1, images 1769-1803 : `C420` 00->05, `C423` 00->01, `C42D` 00->59, `C415` 20->1E, puis `6C98` et `6D5D`). Ce mouvement n'est pas un escalier. `research/room-links.md` §4 (« pente » vers la gauche en salle 0 et salle 1) est probablement le même phénomène : à reprendre avec la vérification de `C415` et `C420`.
- **Saut** : HAUT hors escalier déclenche un saut vertical (`6D5D`, table de saut : 32 px de hauteur, image 1944-1962 de `runs/r2`).

## 7. Fichiers

- `tools/analyze_stairs.py` : prédiction des volées (texte, lecture seule).
- `scratchpad/stairs/` (ignoré par Git) : ROM extraite, traces `runs/r1` à `runs/r8`, dumps `dump-*.bin`, rendus `renders/hall-s1-r*.png`, scripts `work/`.
