# Objets 1A (stages 4 à 6) et 1C (stages 7 à 9) des bougies d'objet 15 : `8AD0`, init `B4D2`, routine `B4EA`

Code : `sgdk/src/enemies_ghost.inc` (état, spawn `Enemies_candleObject`, pose), `enemies.c` (type `ENEMY_TYPE_GHOST` 15 : boîte, score, contact,
dessin), `candles.c` / `candles.h` (flamme de 32 images, événement `VK_CANDLE_EVENT_GHOST`), `main.c` (`step_candles`), `actors.c` (tuiles de la salle),
`tools/build_ghost_md.py` (cellules). Tests : `tests/test_ghost.c` (+ `tests/ghost_vectors.h`, `tools/gen_ghost_vectors.py`), `tests/test_candles.c`,
scénarios `zzf_ghost9.txt` (stage 9 salle 2) et `zzg_ghost8_block.txt` (stage 8 salle 0, bloc).

## Prouvé (traces openMSX `scratchpad/o1c`, `tr.py` : stage et salle forcés, Simon placé, C42D = 255 contre les coups)

- **Création** : la bougie d'objet 15 (`898F`) crée comme l'objet 00 un objet C500 de type 01 (`+4` = 15, flamme rouge). `8A99` compte 16 appels
  de la boucle C500, qui tourne **une passe sur deux** : la flamme dure 31 images (stage 9 salle 2, bougie (48, 64) : coup à l'image 843, objet C500 vu de 844
  à 873, retiré à 874). À ce moment `8AD0` crée l'objet de type `8AF0[(D002 - 1) & 7]` (`1A 1C 1B 1B 1B 1B 1C 1C`) en (X, Y + 16) de l'objet C500 : D002 = 2
  (table `5E6A` : 0 aux stages 0 à 3, 1 aux stages 4 à 6, 2 aux stages 7 à 9, 3 au stage 10) donne **1C**. Même chose pour un **bloc** caché d'objet 15
  (stage 8 salle 0, bloc (96, 112), trace `b80` : flamme C500 en (96, 128) de 925 à 956, objet 1C en (96, 144) à 956 : le bloc donne +16 pour la flamme, +16
  pour l'objet).
- **Routine** (`B4EA` pour les types 19 à 1C, `B611` d'abord : `+C` + 1, pose = base de la salle + 1 si `+C` bit 4) :
  état 0 (`B4FB`) : 8 passes sans mouvement (`+10` = 8), `+E` = 0 (ni contact ni fouet) ; à 0 : `vy` = 200h, `+E` = 7, état 1 (trace : 889, Y 82).
  État 1 (`B50D`) : mur à 16 px (`7BB7` / `7C13`, deux points Y - 4 et Y - 12) arrête `vx`, sol en (X, Y) (`7BA3`, seulement si `vy` >= 0) : Y &= F8, état 2 ; sinon
  `vy` + 40h (Y 82, 84, 86, 89, 92, 95, 99, 103, 107, 111, 115 puis posé en 112, 11 passes). État 2 (`B513`) : 30 passes arrêté, puis `vx` = +1C0h si
  C427 >= X, sinon FE40h (`B600`), `+10` = 1Eh. État 3 (`B526`, `B611` une seconde fois : `+C` + 2 par passe) : Simon au-dessus (C425 - 10h < Y) et pas de
  sol en X +- 8 : bond (état 1, `vy` = FD00h, `vx` vers Simon) ; pas de sol sous lui : chute (`vy` = 200h, `vx` = 0) ; mur : `vx` = 0 ; bord (X < 10h vers la
  gauche, X >= E8h vers la droite) ou `+10` à 0 : repos (`B54B`, état 2). Trace de 3000 images (salle 2 stage 9, Simon immobile) : va-et-vient X 14 à 65.
  Durée de vie : illimitée, il disparaît avec la salle ou au coup (aucune sortie d'écran autre que `99E5`).
- **Contact** : 7E67 exige `+E` bit 1 (état >= 1). Classe 2 de `82CB` (boîte 8 x 16 comme la puce), paire `81CE` (01, 02) : énergie 32 -> 28 (trace), 100 points.
  **Fouet** : 1 point de vie ; `84A7` : C425 - B - Y + 10h dans [0, 14h) avec B = 12h debout, 0Ah accroupi : debout le fouet passe au-dessus d'un objet qui
  marche sur le même sol (trace : debout manqué, accroupi tué à C422 = 3, score 000000 -> 000100, l'objet devient un type 1E de 16 passes).
- **Dessin** : SAT (X - 8, Y - 15), deux couches (couleur 14 et couleur 2 en CC). Poses 9B/9C (patterns D0/D4 + D8/DC), 9F/A0 (70/74 + 78/7C), A1/A2 (A0/A4 +
  D8/DC), la base vient de la salle (`B63F` + 2 x D000 -> tables de nibbles `B665`...) : stage 7 : A1 A1 9B 9B - 9B 9B 9F 9F -, stage 8 : 9F 9F - 9B 9B 9F 9F 9B,
  stage 9 : 9F 9F 9F 9F - A1 9B 9B - -. Les images 9B, 9F, A1 viennent des stages 7 salle 3, 9 salle 2, 7 salle 1 (même image dans les autres salles de même base :
  empreintes des cellules des instantanés VRAM).
- **Comparaison passe par passe** : `gen_ghost_vectors.py` exécute `B4D2`, `B4EA`, `99B9`, `99E5` dans l'émulateur Z80 sur les cartes des stages 7 à 9 (47 scénarios, 16 291
  passes, Simon au-dessus et en dessous : 40 bonds, 1 chute, 7 arrêts de bord) ; le port donne la même sortie (état, X, Y, pose, vx, vy, `+10`) à chaque passe.
  Scénario `zzf_ghost9` : bougie éteinte -> objet 31 images, 15 images d'attente, Y de la chute, contact, fouet accroupi, score.

## Hache sur un bloc

La hache lancée qui touche un bloc (`74A5` -> `8992`, `candles.c vk_candles_drop(..., 0x1C)`) crée un objet **C500** d'identifiant 1C (`8A37`), c'est
l'objet « hache » de la table des objets ramassables, pas l'objet C800 de type 1C : tables et boucles différentes (C500 : `8A4A`, C800 : `98EC`). Lu dans le code
(`8992` appelle `8A37`), non tracé, aucun code modifié.

## Objet 1A, stages 4 à 6 (D002 = 1) : tracé et porté

Traces openMSX (`scratchpad/obj1a`, hors dépôt : `tool/` = harnais `stage4/tool` + un journal des objets C500/C800 et de la SAT, `runall2.py` choisit par
la carte de collision du port (`floors.py`, `pick.py`) une position de Simon d'où le fouet touche la bougie, debout ou en saut). Chaque bougie ou bloc
d'objet 15 des stages 4 à 6 a été éteint un par un, stage et salle forcés (`D000`, `D001`, `D002` = 1, `C000` = 4, `C42D` = 255).

**Liste des 17** (stage, salle, X, Y ; `candles_md.c`) : stage 4 : salle 0 (128, 64), salle 1 (192, 48), salle 2 (80, 48), salle 4 (16, 48) ; stage 5 :
salle 0 (64, 32), salle 4 (208, 64), salle 5 (80, 48), (208, 48), (144, 144) ; stage 6 : salle 0 les trois **blocs** (96, 160), (128, 160), (160, 160),
salle 1 (96, 32), (32, 128), (96, 128), salle 4 (96, 96), (224, 96). Le stage 10 est dans `candles_md.c` (26 bougies et blocs, aucun de type 15 : voir `stage10-level.md`).

- **Même routine** : `9942` donne `B4EA` pour les types 19 à 1C ; `81CE` (01, 02), classe `82CB` 2 et points de vie (`60E1`) 1 identiques. Les 17 objets sont
  créés en (X, Y + 16) de la bougie (bloc : Y de la bougie + 32, comme au stage 8) 30 passes après le coup (flamme C500 de 30 passes), attendent 8 passes
  (`+10` 8 -> 0, `+E` = 0), tombent avec la même suite Y + 2, 4, 6, 9, 12, 15, 19 (`vy` 200h + 40h par passe), se posent à `Y & F8` (état 2, 30 passes), puis
  marchent vers Simon (état 3) : mêmes valeurs d'état, Y, `vy` et `+10` que le 1C, à la passe près (dans les 17 traces : `+10` 8, 7, ... 1 puis chute dY 2, 4, 6, 9, 12, 15, 19 de `vy` 200h, 240h, ... ; les trois blocs du stage 6 salle 0 sont créés à Y 192 sur le sol : un pas de 2 px, puis posé en 192, état 2 deux passes plus tard). Aller-retour entre deux arrêts (états 2 et 3) vus dans les 17 traces, un bond vers Simon (état 1, `vy` FD40h) vu au stage 4 salle 0. Cas du trou (stage 5 salle 5, bougie (144, 144), pas de sol sous X 144) : l'objet tombe (Y 162 à 222) et disparaît à Y >= E4 (`99E5`).
- **Base de pose** (table `B63F + 2 x D000` : stage 4 `B665` = 9F 9D 9F 9D 9F 9F, stage 5 `B668` = 9F x 6, stage 6 `B66B` = 9B 9B 9B 9F 9F 9B) : lue dans la ROM,
  puis vérifiée sur chacune des salles tracées (stage 4 salles 0, 2, 4 : 9F, salle 1 : **9D**, stage 5 : 9F, stage 6 salles 0, 1 : 9B, salle 4 : 9F).
- **Dessin** : SAT (X - 8, Y - 15), deux couches. Motifs 70/74 + 78/7C (9F/A0), C0/C4 + C8/CC (9D/9E), D0/D4 + D8/DC (9B/9C) : l'image est **la même** (masques
  des 4 motifs égaux octet à octet dans les 6 salles comparées et au stage 9 salle 2) ; seules la place en VRAM et la **couleur** changent : couches **15** et **2** en CC
  (table des couleurs de la SAT, 16 lignes identiques) aux stages 4 à 6, **14** et 2 aux stages 7 à 9 (trace stage 9 salle 2 refaite : 0E / 42). Les tuiles du
  port (`ghost_a_md.c`, `tools/build_ghost_a_md.py`) sont celles du 1C en couleur 15 ; cellules 70 et 74 (stages 4, 5 : libres, la plateforme est en 60) ou 80 et 84
  (stage 6 : 70 à 7B pris) du bloc, chargées par la salle (`actors.c`, `Enemies_ghostCell`).
- **Contact** : énergie 32 -> 28 aux trois stages (stage 4 salle 0, stage 5 salle 0, stage 6 salle 1). **Fouet** : accroupi, un coup tue (score 000000 -> 000100),
  debout il passe au-dessus (stage 4 : score et objet inchangés) ; comme le 1C.
- **Comparaison passe par passe** : `gen_ghost_vectors.py` (type 1A pour les stages <= 6) exécute `B4D2`, `B4EA`, `99B9`, `99E5` sur les cartes des stages 4 à 6 : les 17
  bougies et blocs (Simon au-dessus et en dessous), puis chaque salle ; au total 99 scénarios, 28 734 passes (62 bonds, 13 chutes de rebord, 7 arrêts de bord), le port
  donne la même sortie à chaque passe. `tests/test_ghost.c` : suites Y des traces (9 salles), trou, bases de pose, cellules, contact, fouet.
  Scénarios harnais : `zzi_ghost4.txt`, `zzj_ghost5.txt`, `zzk_ghost6.txt`.
- Lag : les « images » de ces traces sont des passes de la boucle principale (point d'arrêt `4028`) ; la comparaison est faite passe par passe.

## Non porté ou non vérifié

- **1B** (D002 = 3, stage 10) : même routine, mais **inatteignable** : aucune bougie ni bloc d'objet 15 dans les 9 salles du stage 10 (`stage10-level.md`, lu dans 5A49 + 5B1B et `DE00`) ; rien tracé, pas de code,
  `Enemies_candleObject` ne fait rien au stage 10. Pointeur de pose du stage 10 : `B653` -> `B67C` (non relevé).
- Les flammes des objets 00 (30 images vues, retrait 31 images après le coup), cœurs (31 images) et autres objets (41 images + chute) sont tracées et portées : `research/candle-flames.md`.
- Effet de mort 1E (flamme de mort `9A3E`) : porté, voir `death-flame.md` (l'objet 1C tué laisse aussi une flamme, Y - 0).
- Cas non rencontré par la trace : plusieurs objets 1C à la fois (places SAT : 14 entrées, `newObject`), reste couvert par la logique commune.
