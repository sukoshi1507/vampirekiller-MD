# Boss du stage 6 (objet 13h, salle 5), clé, porte et passage au stage 7

Code : `sgdk/src/boss.c` (le boss du stage 3 et celui-ci partagent le dégât, la boule D700, la barre, le script de fin
`B6AB` et l'orbe ; `kind` = 1 pour le stage 6), `actors.c` (vue, contacts, effacement du buste), `main.c` (porte).
Tuiles : `tools/build_boss6_md.py` (instantanés VRAM d'une trace openMSX non versionnée, `scratchpad/stage6boss/out/C`).
Tests : `tests/test_boss.c` (natif, valeurs des traces A et D), `tests/md_scenarios/zz_stage6_boss.txt` (apparition, boule,
coups, contacts, mort, orbe, stage 7), `tests/md_scenarios/zy_j_stage6_key_door.txt` (clé, porte, entrée de la salle 5).

Traces (stage 6 forcé : `D000` 6, `D002` 1, `D001` salle, `C000` 4) : A = porte puis 1850 images sans coup, Simon
immobile en (246, 160) ; B = Simon en X 200 puis déplacé par les coups et les contacts, fouet toutes les 64 images, 10
coups portés (`C418` 128 -> 48) ; C et D = `C418` posé à 8, mort, orbe (C : orbe laissé au sol, D : orbe pris), sortie ;
K = clé de la salle 0. Colonne salle de `sat_s2.csv` contrôlée : toujours 5 pendant le combat, la salle 5 n'a aucun lien,
un coup ne renvoie pas Simon dans une autre salle.

## Prouvé (ROM et trace, chiffres)

**Porte, entrée.** `kStageItems` : clé salle 0, porte salle 4 en (12, 80), côté gauche. Trace A : Simon en (22, 128) dans
la salle 4, effets FF puis 1A à l'ouverture (image 821, `C5AC` 1 -> 2), `C5AC` 3 au passage (909), effet 87 (musique du
boss, image 965), salle 5 en (246, 128) (lien gauche de la salle 4 ; `77D1` n'écrit pas Y), chute à 160 (effet 07, image
1001). Le port prend X 246 et garde le Y de la salle de la porte (`vk_boss_entry_x/y` ; stage 3 : lien droit, X 10, Y 176
de sa trace). Scénario `zy_j` : entrée en (246, 128), chute à 160.

**Script de salle `CE01`** (`BCDB`, table `636F` : salle 5, `CE00` = 2) : étape 0 `CE02` = 78h ; étape 1 attend 120
passes ; étape 2 (`BCF9`) crée l'objet 13h en (X 90h, Y 90h) par `5F1D` et remplit de couleur 0 le rectangle (80h, 70h) de
20h x 20h (`490A`, HMMV : le buste de la salle disparaît) ; étape 3 attend `CE15` (mort), puis `BE47` lance le script
`B6AB`. Trace A : `CE01` 1 à l'image 988, 2 à 1228 (120 passes), 3 à 1230 (création et premier pas dans la même passe).
Scénario : création 220 à 250 images après la fin de la chute, Y 9030h X 90F8h, puis 9080h/91F0h et 90F0h/92E8h comme les
images 1230, 1232, 1234.

**Objet 13h** (`BD19`, init `BD26`, `BE0D`, `BD46`, `A549`) : vx = +F8h, vy = 10h, pose 2B, `+14` = 8, `+15` = 3Ch. Chaque
passe : `+15` - 1, à 0 il repart à 3Ch et tire une boule ; puis état 0 : `BD46` (battement : `+14` - 1, pose 2C si le bit
3 de `+14`, vy +- 20h selon Y < A0h, 7FFh au plus vers le bas), et si X < 10h, X >= F0h ou |X Simon - X| < 8 : vx = -vx,
`+12` = 3Fh, `+13` = 60h, état 1 ; état 1 : `+13` - 1, s'il reste non nul `BD46` et le même test avec `+13` = 10h ; à 0 :
arrêt (`+6` = 0), `+12` - 1 par passe, puis vx = F8h (négatif si X Simon < X), `+6` = 1, état 0. Mouvement `99B9` après.
- **Comparaison passe par passe** (`research/stage4-enemies.md`, section Comparaison à la trace) : pour chaque passe on garde
  l'objet complet (Y, X, vy, vx, état, `+6`, pose, `+12`, `+13`, `+15`), en écartant les échantillons pris au milieu d'une
  passe, et on alimente `boss.c` avec le X et le Y de Simon de la même image. Trace A : 923 passes, 0 écart. Trace B
  (Simon mobile, 10 coups, contacts) : 2851 passes, 0 écart. Traces C et D : 334 passes chacune (jusqu'à la mort), 0
  écart. Cela couvre plusieurs allers-retours complets : arrêts de 62 passes, départs vers Simon des deux côtés, demi-tours
  au bord et près de Simon.
- Test natif `test_boss.c` : passes 1, 2, 60 (première boule), 100 (demi-tour, X EEF0h), 196 (arrêt), 258 (reprise) de A.

**Boule** (`BE15` -> `A0E5` -> `9F6D`, type D700 6, `A08E`[13h] = 6) : en (X, Y - 16) de l'objet, sans fractions (trace :
X C900h, Y 8200h avant le pas), visée vers Simon comme au stage 3 (vitesse 80h + 8 D012), premier pas dans la passe de la
création. Poses 27, 28 (vers la gauche) et 29, 2A (vers la droite) par le bit 1 de `+C` (`9E78`) ; `+C` n'est pas remis
à zéro par `9F6D`, le compteur continue d'une boule à l'autre. Suppression par `99E5` après le pas : Y >= E4h, X >= F1h
ou X < 7. Trace A : 196 relevés de boule, 0 écart de position et de vitesse (à une passe près pour les échantillons
partiels) ; trace B : 1073 relevés, 5 écarts, tous des échantillons pris au milieu d'une passe. Aucune boule ne survit côté
MSX à la sortie d'écran ; celles qui durent plus longtemps dans le modèle sont celles qui touchent Simon (contact traité
à part). Scénario : première boule Y 82C8h X CCE8h vy 200 vx 1000 contre Simon (246, 160) comme l'image 1348 ; contact 12
à 20 images après, énergie 32 -> 31 (trace : 1348 -> 1364, 32 -> 31).

**Dégâts** : fouet -8 par coup (trace B : 128, 120, 112, ... 48 ; `7E2C`, table `7E59`[2] = 8) ; contact du corps -4 sans
armure (trace B : 32 -> 28 ; paire 12h de `81CE`, `20 02`) ; boule -1. Mort (`7E17`, trace D, image 1897) : effets 0C et 1C
au même instant, `C445` = 14h, `CE15` = 1, objets vidés. Le scénario reproduit les trois dégâts et la mort.

**Mort, orbe, sortie** : script `B6AB` commun. Trace D : `CE10` 0 à 1901, 2 à 2021 (120 passes ; scénario : 120 à 130 images
après le coup), orbe en (78h, 40h), au sol en Y **B0h** (176, le sol sous X 78h de la salle 5, `7B98`) à l'image 2424 (403
images après la création : 90 passes d'attente + 112 de chute ; scénario : 400 à 410), pris au sol (`CE11`), effet 00 puis
musique 8C (2426), 150 passes, soin de 1 point toutes les 4 images, effet 01 (12 pas de 20 à 32), 60 passes, sortie en Y
E0h (`B772`, 2897), puis stage 7 en salle 0, Simon en (232, 176) (2925, états 8 puis 4), musique 82 (2961). Sans prise
(trace C) : `CE10` = 6 à 2785 (180 passes après l'atterrissage), musique 8C, sortie 144 passes plus tard (2785 -> 3073).
Barre ENEMY : `C418` repasse de 0 à 80h dans l'état 8 (image 3076, trace C) : la remise à pleine est **prouvée pour le
passage au stage suivant** (`research/stage3-boss.md` la donnait provisoire).
- Palette de salle : à la création de l'orbe (`57C0`) les entrées **5 et 7** passent de (5,0,3) et (6,3,4) à (3,0,0) et
  (5,0,0) (instantanés avant et après) ; stage 3 : entrées 4 et 6. Le port applique 5 et 7 à PAL0 et PAL2.
- Orbe : à terre l'objet est en état 2, dont la routine est `B83F` : la pose continue (8F, 90, 91, 90 ; trace C : poses 90
  et 91 relevées posées). Le port figeait la pose à 8F (corrigé pour les deux stages, voir Anomalies).

**Rendu** (VRAM + SAT d'une image MSX rendues avec les couleurs MD `(v + 18) // 36`, comparées pixel à pixel à une capture
du harnais ; le port affiche une passe en retard comme les autres sprites, on compare donc à la passe voisine) : boîte de
82 x 50 autour du boss et de la boule, passe 60 : 38 pixels sur 4100 (recouvrement du boss et de la boule, OU des couleurs
approché) ; tout l'écran de jeu (256 x 180) : 58 pixels sur 46080 ; zone du buste et du piédestal (80 x 116) : 0 ; poses
A et B du boss : 20 à 80 pixels sur 30208 selon l'instantané (6 instantanés, Simon en chute compris) ; orbe, 3 poses (8F
à terre, 90 en chute en Y 45h, 91 en chute en Y 47h) : 0 pixel sur 2250 à 3000. Cellules : 2 x 2 de 16 x 16 à (X - 16, Y -
30) et (X, Y - 30) (SAT - 31, + 1), chacune de deux plans (motif n en couleur 2, motif n + 4 en couleur 5 avec CC, OU = 7) ;
boule en (X - 8, Y - 14), deux plans (motifs 94/90 et 9C/98 vers la droite, 84/80 et 8C/88 vers la gauche).

**Clé blanche** (salle 0, bloc caché (192, 160) ; trace K) : Simon en (188, 192) face à droite, fouet à l'image 954 (effet
05), bloc cassé à 957 (effets 0C et 0E), clé prise à 968 : `C701` = 1 (`8E77`), effet 14. Scénario `zy_j` : clé prise par le
fouet, porte ouverte en (34, 128) vers la gauche, clé consommée, salle 5 en (246, 128), boss en jeu.

## Déduit (code lu, pas mesuré en jeu)

- Score 2000 (paire 18 de `81CE`, `20 02`, comme le stage 3) : 002000 affiché par le scénario, non lu sur le HUD MSX.
- Boîte du corps (classe 4, h 18h, l 0Ch : `82CB`[12h] = 4 comme l'objet 12h) et du fouet (`83F2`) ; boule : boîte (6, 6) (corrigé, voir boss-boxes.md)
  de `8338`/`8340`, dégât 1, détruite par le fouet pour 100 points : mêmes routines qu'au stage 3, non remesurées ici.
- Armes lancées : 12 de dégât (`7E60`[2] = 0Ch), réarmement `999F`, comme au stage 3 : non jouées en stage 6.
- Musique 87 à l'entrée, FF à l'ouverture de la porte (`7763`, table `7777`[6] = 1) : mêmes appels qu'au stage 3 ; la trace
  montre FF, 1A, 87 aux images 821 et 965.
- Instant du changement de palette (`57C0`, même passe que la création de l'orbe) : rattaché au code, pas à une image.
- Temps arrêté : boss et boule figés (`98E5`), non rejoué.

## Non vérifié ou non porté

- Phase initiale de la pose de la première boule : `+C` de l'emplacement D700 vaut 0 dans le port ; sur MSX il garde la
  valeur d'objets précédents. Seule la pose de la première boule peut différer.
- Prise de la clé : sur MSX la clé est prise 11 images après le coup qui casse le bloc (957 -> 968), sur MD 6 images après le
  début du coup. Cause : `7FE2` sort tant que `C422` != 0 (pas de ramassage pendant l'attaque) ; le bloc casse au minuteur 11 du
  fouet, la clé est prise à 0, 11 images plus tard. Corrigé (`VKItems.attacking`, `items.c`, scénario `zzd_key_pickup_after_whip`).
- Transition MSX de 28 images entre la sortie (`C409`, image 2897) et le chargement du stage 7 (2925) : **portée**. État 8
  (`4324`, `C000` = 8 de 2898 à 2924, retrouvé à l'identique par `poke C409` : 835 -> 863) : `47B9` retire les sprites (`47F0`),
  coupe l'affichage et efface la page par une commande VDP ; instantané 22 images après : écran entièrement noir. Le port
  cache les sprites, noircit les palettes et attend 28 images (`BOSS_EXIT_FRAMES`, `main.c`), sans lire de touche, puis
  `next_stage()`. Même code pour la sortie du boss du stage 3 (mesuré au stage 6 seulement). Scénario `zz_stage6_boss`.
- **Après l'orbe** : le port charge le stage 7 salle 0, Simon en (232, 176), musique 82, barre ENEMY pleine, avec les cartes,
  bougies, clés et blocs du prototype du stage 7 ; les ennemis et le boss du stage 7 ne sont pas portés. Pas d'écran noir.

## Anomalies relevées

- Stage 3 (non corrigé) : `fire()` garde les fractions de position du boss pour la boule ; la ROM les met à zéro (`9FE6`-`9FF7`,
  `BF44` -> `9F61`). Le stage 6 les met à zéro. Écart de moins d'un pixel.
- Stage 3 (non corrigé) : la boule disparaît à Y < 10h, X < 8 ou Y >= E0h (observé) ; `99E5` dit Y >= E4h, X >= F1h, X < 7. Le stage
  6 suit la ROM.
- Orbe posé (corrigé, code partagé) : la pose continue à `B83F` en état 2. Le stage 3 n'a pas été retracé (même code, `B7A1`).
- Pièges de comparaison : l'échantillon de la trace prend parfois l'objet au milieu d'une passe (`+15` rechargé avant le pas ;
  `+14` et `+15` mis à jour avant le déplacement ; `+E` à 06 pendant un coup) ; l'écart d'une passe qui en résulte n'est pas
  une erreur du port.
- Le test natif `test_boss.c` affirmait `!vk_boss_door(6, 4)` ; le stage 6 a maintenant une porte de boss (`!vk_boss_door(5, 4)`).
