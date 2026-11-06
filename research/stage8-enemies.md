# Stage 8 : hopper 0B et son os, leaper 0C, objets 08 et 0A, objets de salle, clé, porte et sortie

Code : `sgdk/src/enemies.c` (`hopperStep`, `leaperStep`, `boneStep`, état à part `s8x`, `boxOf`, `isProjectile` ; `statueStep`,
`type8Step` et la boule d'`fireballStep` étendus au stage 8), `actors.c` (tuiles). Tuiles : `tools/build_stage8_enemies_md.py`, sur des
instantanés VRAM de traces openMSX non versionnées (`scratchpad/stage8/out/R` et `L`). Bougies, blocs, clés dorées, coffres et murs :
`tools/build_candles_md.py` (`stage > 8` aux trois endroits, `WALL_STAGES = 9`). Tests : `tests/test_stage8.c` (natif, avec
`tests/stage8_trace.inc` : 45 transitions réelles), scénario `tests/md_scenarios/zzb_stage8.txt` (+ `zzb_enter8.inc`, `zzb_next.inc`).

Forcer le stage 8 dans openMSX : `D000` = 8, `D002` = 2 (table `5E6A`, octet de fichier `1E6A`, indice 8 : 2 ; attendu 2, confirmé), `D001` =
salle, `C000` = 4. Le stage a 8 salles (0 à 7). Aucun boss : `7777[8]` = 0 (seuls les stages 3, 6, 9 et 10 en ont un).

## Objets vus (trace R, 500 images par salle, Simon immobile en (16, 144) ; L : Simon posé en X 48 ; N : Simon posé en X 140, 60 puis 200)

| Salle | Drapeau (85A6) | Objets de la liste ou canal | Fixes (candles_md.c) |
|---|---|---|---|
| 0 | 00 | aucun | bloc caché (192, 112) avec une clé dorée 17 |
| 1 | 20 | 08 par le canal `9DD5` | bloc caché (192, 112) avec une clé dorée 17 |
| 2 | 00 | 0B en (208, 80) ; os D700 type 4 | - |
| 3 | 00 | 0B en (144, 144) ; os | coffre 19 (32, 128) contenu 0C, clé 17 (160, 64) |
| 4 | 00 | aucun | clé 17 (16, 160) |
| 5 | 00 | 0C en (80, 80) | - |
| 6 | 00 | 0A en (144, 144), boule D700 type 3 ; 0C en (80, 192) | - |
| 7 | 00 | 0B en (128, 192) ; os | clé blanche 18 visible en (32, 64) |

Les autres entrées vues juste après un changement forcé (08 dans les salles 3 et 6, 0A en salle 7, 0B en salle 4, 5 et 6) sont des restes de la
salle précédente (20 images). La salle 7 contient aussi trois blocs cachés (192, 64), (192, 128), (192, 160), et les salles 2 et 3 un mur
`CF` / `D3` en (192, 112) (générés, voir plus bas). Types nouveaux : **0B** (routine `B37E`, init `B344`), **0C** (`A67D`, init `A670`), **os
D700 type 4** (`9F07`, créé par `A08E[0B]` = 4) ; les objets 08 et 0A (statue et boule) sont ceux des stages 4 à 6.

## Prouvé (ROM et trace)

**Méthode.** Pour chaque objet on extrait de la trace les transitions entre deux échantillons consécutifs différents du même bloc (32
octets : Y, X avec fractions, vy, vx, état, pose, `+6`, `+10`, `+11`, `+12`, `+18`, `+1E`), puis on lance **une passe** de `enemies.c` à partir du
premier bloc avec la position de Simon de l'image du second ; le résultat doit être le second bloc, octet pour octet (comparaison PASSE PAR
PASSE, pas par image : le MSX échantillonne parfois au milieu d'une passe). Un échantillon pris après le comportement et avant le
déplacement (`99B9`) ou une paire d'échantillons en deux temps compte pour une transition de moins, pas pour un écart. Traces : R (Simon
immobile), L (Simon en X 48), N (Simon posé tous les 24 images en X 140, 60 ou 200), M (Simon posé en Y 192 : la carte chargée n'est plus
celle de la salle, 28 transitions du 0B non retenues, voir Anomalies).

| Objet | Transitions comparées | Écarts |
|---|---|---|
| 0B, R + L + N | 744 + 1420 + 393 = 2557 | 0 (6 paires d'échantillons en deux temps en L ; quatre cas de N demandent la position de Simon de l'image précédente : le poke du plan tombe entre la passe et l'échantillon) |
| 0C, R + L + N + M | 242 + 453 + 228 + 67 = 990 | 0 (la seule transition refusée est la création de l'objet en salle 5, N image 780 : le bloc d'avant porte encore la pose de l'objet précédent) |
| os D700 type 4, R + L + N | 662 + 1011 + 458 = 2131 | 0 (8 paires d'échantillons en deux temps ou « comportement fait, pas encore déplacé » ; un dernier échantillon de ce genre juste avant la sortie d'écran de l'os) |

États couverts : 0B 0, 1, 2 (R et L : 1089, 339 et 736 transitions) ; 0C 0 à 6 (R, L et N : état 3 : 10, état 4 : 4, état 5 : 90, état 6 : 19) ; le 0C ne passe jamais
à l'état 7 dans les traces (voir Déduit). Les 45 transitions les plus variées sont dans `tests/stage8_trace.inc` et rejouées par `test_stage8.c`.

**Objet 0B** (init `B344` : vx = +-240h vers Simon avec la difficulté, vy 0, mobile ; pas `B37E`). Trois états. Chaque passe : pose 47 si Simon est
à gauche, 49 sinon, +1 quand le bit 3 de `+12` (compteur incrémenté à chaque passe) est nul (`B364`). État 0 (`B38D`) : Simon à moins de 30h en
X : il recule (+-240h) ; entre 30h et 4Fh : il garde sa vitesse ; plus loin : il s'approche. Après un changement de vitesse, si les bits 1 et 2 de
`+12` sont nuls, il s'arrête (`+6` = 0), `+10` = 10h, état 2. Sinon : mur de face (4 points X +- 8, Y - 12, - 20, - 28, - 36, `7BBE` / `7C1A`)
= demi-tour ; pas de sol devant (X + 9, X - 4 ou X + 4 et X - 10, `7B98`) : saut vx +-240h, vy -471h (`FB8F`), état 1 ; sinon X < 10h ou X >= EEh :
retour vers l'intérieur. État 1 (`B451`) : mur = vx 0, gravité +68h (`A549`), à la descente le sol à X + 8, X - 3 ou X + 5 le pose
(Y &= F8, vy 0, état 0). État 2 (`B4A9`) : 10h passes, puis un os (`9F61`) en (X, Y - 10h) avec vx +-180h vers Simon et vy -500h ou -800h
(bit 0 du registre R de `LD A,R` : hasard, voir Non vérifié), état 0, mobile. Points de vie 4, 200 points, dégât 2 au contact, boîte (5, 18h)
(classe 1 de `82CB`), taille SAT 4 (2 cellules de 2 couches en X - 8, Y - 32 et Y - 16 ; couleurs 2 et 12).
- Trace R image 1845 : X D0, puis CD.C0, CB.80, C9.40 : le port donne les mêmes valeurs (scénario). Os de l'image 1970 : X 66.80, Y 78.50, vy
  F850 pour un lanceur en X 68h : le test natif reconstruit les mêmes octets. Les deux vitesses d'os (F850 et FB50 après le premier +50h)
  sont vues dans R (images 1970, 2004 et 2420, 2454).

**Os (D700 type 4, `9F07`)** : `+C` + 1 modulo 12 (à 0 au retour), pose 4B + (`+C` >> 2) & 3, `A549` avec DE = 0050 (gravité +50h par passe),
déplacement `99B9`, suppression `99E5` (Y >= E4h, X >= F1h ou X < 7). Créé et déplacé dans la même passe (`9E31` suit `98E5`). Boîte
(6, 0Ch) (`830D[3]` = 2, `8358`), dégât 1 sans armure (`7F33`, `462B` avec B = 1), détruit par le contact, le fouet et les armes lancées pour 100 points.
`+C` appartient à l'emplacement D700 (huit, premier libre) : le port tient un `+C` par emplacement (`s8BoneC`).

**Objet 0C** (init `A670` : `+10` = 18h, pose 89, `+12` = 0, immobile ; pas `A67D`, `+18` + 1 à chaque passe). Huit états :
0 attend `+10` passes puis bondit ; 1 bond (`A7BD` : vx 200h vers Simon, toujours à gauche si X >= C0h, à droite si X < 40h ; vy -220h, -280h, -300h ou -480h si
Simon est plus haut, +220h, +280h, +300h ou +380h sinon, indice `+18` & 3, inversé si le bond part de Y < 40h ou Y >= C0h), vy ramené à 0 de 20h par passe,
puis état 2 ; 2 pause de 18h passes (`+6` = 0) ; 3 : si Simon est à moins de 18h sous lui en Y, charge (état 4), sinon nouveau bond ; 4 vx +-300h, vy 0,
`+10` = 30h ; 5 charge, après 30h passes nouveau bond, ou état 6 quand il a dépassé Simon de 8 px ; 6 demi-tour par pas de 20h jusqu'à +-200h ; 7
marche à +-200h. Aux états 1, 5, 6 et 7 un bord d'écran (Y < 20h vers le haut, Y >= C0h vers le bas, X >= E8h à droite, X < 10h à gauche) renvoie à
l'état 2. Pose 87, 88 (gauche) ou 8A, 8B (droite) par le bit 3 de `+12` (`A797`), 89 à l'arrêt. Points de vie 1, 100 points, dégât 2, boîte (8, 10h)
(classe 2), une cellule de 2 couches en X - 8, Y - 15 ; couleurs 2 et 1. `+18` garde la valeur de l'objet précédent du même bloc : le port
tient ces octets dans `s8x`, remis à zéro au début de la partie seulement.
- Trace R salle 5, premier bond : X 82, Y 82 (Y frac 20), puis 84/84, 86/86, 88/87, 90/89, 92/90 : identiques en scénario (Simon posé en (212, 144)).

**Dégâts relevés en trace** (colonne énergie, R + L + N, 19 pertes) : objet 08 -4 (2), 0B -2 (4), 0C -2 (4), statue -2 (1), -1 (8, l'os ou la boule de la statue
étant l'objet voisin de l'instantané, pas attribué avec certitude). Conformes aux tables `81CE` doublées et à `7F3D`. Scénario : énergie 32 -> 30 au contact du 0B.

**Décor** : les huit salles comparées au MSX (instantané VRAM, couleurs MD `(v + 18) // 36`, instantané de la bonne salle : D001 relu dans le
fichier, les instantanés `r1`, `r2` et `r5` de fin de salle montrent la salle voisine, Simon ayant été renvoyé). Hors HUD (32 premières lignes), dessins
parasites en (64, 144) et (192, 144), sprites du MSX (+8 px) : 0 pixel (salles 6, 7) à 20-60 (salles 0, 3, 4), 150 (salle 1), 293 (salle 2) et 415 (salle 5) ;
le reste est Simon et les flammes du port, que le masque ne couvre pas (cellules de Simon (16, 144) en salles 2 et 5). Chests, clés, blocs et la porte de la
salle 7 sont conformes à l'oeil.

**Clé blanche, porte, sortie** (salle 7 ; `kStageItems` : clé visible (32, 64), porte (236, 64) côté droit). Trace A (C701 posé à 1, Simon posé en (228, 112) puis
DROITE) : le stage change 170 images plus tard (`D000` 9, `C000` 4, Simon en (248, 112) qui marche), puis état 5 en salle 0, Simon en (16, 112) : c'est
l'entrée `kStageEntries[8]`. Scénario : clé prise au contact en (40, 66), porte ouverte, stage 9 salle 0, (16, 112).

## Déduit (code lu, pas mesuré en jeu)

- Fouet et armes lancées sur 0B (4 coups de 1, ou 2 de 2 avec la chaîne ; hache 4), sur 0C et sur l'os (`7DE6`, `8066`, `82A4`) : testés en natif et pour
  un coup de fouet en scénario (200 points pour un 0B ramené à 1 point de vie), pas en trace.
- Boîtes de contact : classes de `82CB` (0B classe 1, 0C classe 2) et de `830D` (os classe 2, `8358` : H = 0Ch, L = 06) ; les mesures de dégâts ci-dessus
  prouvent le contact, pas la limite exacte des boîtes.
- Score 200 (0B), 100 (0C, os) : paires de `81CE` et `822A`, non lus sur le HUD MSX.
- État 7 du 0C : le code est lu (`A793`), aucune trace n'y est arrivée (la charge 5 -> 6 -> 7 demande 40 passes sans bord d'écran).
- Contenu 0C du coffre de la salle 3 : `8DF5` (C431 bit 3, scénario : octet 8 et dernier objet 0C), effet porté et tracé (`chests-stage1-9.md`).
- Clé blanche visible en (32, 64) : prise par `items.c` comme au stage 6 (règle `847A`), non tracée ici. Le `0x18` de la liste fixe (même objet)
  n'est pas généré (il ferait doublon : `build_candles_md.py`).
- Mur `CF` (salle 2) et `D3` (salle 3) en (192, 112), blocs cachés de la salle 7 : générés par `5A49` / `5B1B`, mécanisme de `wall.c`, non rejoués.

## Non vérifié

- Hasard du lancer : le bit 0 de R (`B4AD`) donne vy -500h ou -800h ; le port tire le bit 0 d'un petit générateur (`s7Random`, partagé avec le stage 7). Les deux valeurs et leur écart
  de 300h sont vus en trace, pas l'ordre.
- Phases initiales hors boot propre : `+12`, `+18` et `+C` des os valent ce qu'ils valaient au dernier usage du bloc ; le port part de 0 au début de la partie.
- Temps arrêté (objet 0A de la liste d'objets, `D010`) : `992F` fige les objets du stage 8 comme les autres, non rejoué.
- Armure (C701 bit 4) : l'os n'est pas divisé par deux (`462B`), le corps l'est ; non rejoué.
- Un coup de fouet qui touche en même temps plusieurs objets : voir `research/castle-candles.md` (un contact par passe) ; non rejoué ici.

## Après la sortie

Stage 9 salle 0, Simon en (16, 112), barre d'énergie gardée, cartes, bougies, clés et blocs générés, les objets du stage 9 sont portés depuis (`stage9-enemies.md`, `stage9-boss.md`), pas d'écran noir. Les objets du stage 7 sont générés dans `candles_md.c` (bougies, blocs, objets fixes, murs) par le
changement de `build_candles_md.py` : aucun comportement du stage 7 n'est écrit.

## Anomalies relevées

- Piège de comparaison (voir `stage4-enemies.md`) : le plan M (Simon posé en Y 192, sous le sol) a chargé une carte qui n'est plus celle de la salle 7 (le 0B saute dans
  le vide en X 128 là où la carte du port montre un sol) : 4 écarts, écartés avec toute la trace M pour le 0B. Les traces R, L et N, Simon sur le sol, n'en ont aucun.
- La porte de la salle 7 est derrière un bloc : Simon posé en (214, 112) ne peut plus avancer (le bloc caché de la salle est solide dans la carte du port) ; le
  scénario pose Simon en (228, 112), comme `zy_h` pose Simon près de la porte.
- Le décor des salles 1, 2 et 5 en fin de trace montre la salle voisine (Simon renvoyé par un coup) : voir Décor.
- Les deux dessins parasites en (64, 144) et (192, 144) sont toujours là (déjà signalés aux stages 4 à 6).
- Défaut antérieur, corrigé : une boule créée par un objet d'indice plus petit que le sien (statue en 0, boule en 1) marchait une image avant la statue. Confirmé en scénario (X de la boule changeait aux images impaires, le compteur de la statue aux paires) puis aligné sur la trace R2 du stage 6 (X de la boule et `+12` de la statue changent aux mêmes images, 1910, 1912, ...). `statueFire` fait comme `boneSpawn` (age 0 si l'indice est plus grand, sinon déplacement immédiat). Même code aux stages 6 et 8 ; scénario `zx_stage6_enemies`.
