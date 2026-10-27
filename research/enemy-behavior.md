# Comportement des ennemis du hall (stage 1, salles 0 à 7)

Statut : **positions de spawn et trajectoires mesurées en RAM (openMSX) sur les salles 0, 1, 2, 3, 4, 5, 6 et 7** ; mécanique de spawn par drapeau partiellement vérifiée ; points de vie, points et drops **inconnus** (aucun ennemi détruit observé). Ce document complète `research/enemy-spawn.md` (minuterie et garde) et `research/player-health.md` (dégâts). Il corrige des points de `enemy-spawn.md` (sections 2, 3 et 4, voir §6).

ROM de référence : `Vampire Killer (Japan, Europe) (En).zip`, SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2` (vérifié sur la copie de travail). Adresses CPU. Les `D000/D001` s'écrivent `stage/salle` ; ici `D000 = 1` (hall), `D001` = numéro de salle 0 à 7.

Légende de confiance : **prouvé** = valeur ou événement lu en RAM dans une trace ; **déduit** = lecture de code ou raisonnement sur des mesures ; **inconnu**.

---

## 1. Méthode

- Trace openMSX 21.0 (`C-BIOS_MSX2`, ROM en cartouche), entrée au titre par SPACE, puis plans d'actions. Lecture seule à chaque VBLANK (point d'arrêt `0x4028`) : `D000`, `D001`, `C000`, `C420`, `C427/C425` (position de Simon), `C415` (énergie), `C42D` (invulnérabilité), `C422/C42F` (fouet), table SAT `D600-D67F` et blocs objets `C800-C8FF`, `D700-D7FF`, **uniquement quand ils changent**.
- Écritures surveillées (`tools/enemy_spawn_writers.tcl`) : `D638`, `D63C`, `D648`, `D658` (entrées SAT ennemies) et `C800`, `C880` (objets), avec le PC et l'image. C'est ainsi qu'une création est datée et qu'on lit le type écrit.
- Routes : runs A, B, D, E, F, G (plans `scratchpad/enemy-behavior/plan-*.txt`, non versionnés). Pour éviter que le recul des ennemis fasse sortir Simon de la salle, les observations sont faites avec `poke C427=120`, `poke C425=192`, `poke C415=32` toutes les 30 images. Le poke de position fausse donc un peu le déplacement de Simon, pas celui des ennemis.
- Déterminisme : les runs rejoués à l'identique donnent des `frames.csv` identiques (`cmp`).
- Analyse : `tools/analyze_enemy_behavior.py RUNDIR` (segments de salle, créations, pistes, baisses d'énergie).
- Les pistes suivent une entrée SAT. Une entrée peut être réutilisée pour un autre ennemi : on ne compte que les créations écrites par `5F82` (PC) dans `C800/C880`.
- Lecture de ROM (lecture seule, `tools/disasm_rom_window.py` avec le venv du scratchpad) : `9CE6`, `9D97`, `5F1D`, `5EB8-5F19`, table `85CF`, table `9D43`, table `9DBB`, table `8AF0`.

---

## 2. Positions de spawn par salle

### 2.1 Table des drapeaux (ROM, prouvé)

`85CF + D001` (`5EE1` puis `4066`), pour le stage 1 : `01 01 00 00 08 01 01 00` (salles 0 à 7). Chaque bit appelle une routine de spawn (`5EF6` : bit 0 appelle `9CE6`, bit 1 `9D4B`, bit 2 `9D52`, bit 3 `9D97`, bit 4 `9DC3`, bit 5 `9DD5`, bit 6 `9DE7`). Seul le bit 3 (salle 4) et le bit 0 (salles 0, 1, 5, 6) ont été mesurés ici.

Il n'y a **qu'un seul appel** de `9CE6` dans la ROM (`5EF6`) ; `9CE6` fixe `C = 1` (type 1). Le type 4 vient de `9D97` : `C = 4` est passé par `PUSH BC`/`POP BC` autour de l'appel à `9CC3` (`9D9D`-`9DA3`), puis `JP 5F1D` (`9DB8`).

### 2.2 Créations observées par salle

Colonnes : **image** (VBLANK de la création), **côté** (X de la position réelle : `E5` = droite, `0A` = gauche), **Y** (Y de sprite), **N** (premier motif de l'entrée SAT).

| Salle | Drapeau (ROM) | Type écrit en C800/C880 | Créations observées (exemples, runs) | Cadence | Confiance |
|---|---|---|---|---|---|
| 0 | `01` | 1 (type 1) | 1586 droite (N40, X E5, Y A0) ; 1690, 1985, 2089, 2193, 2880, 2984, 3511, 3615, 4814 gauche (N58, X 0A, Y A0) | 104 et 152 images entre créations (voir §6, point 4) | **prouvé** (runs A, B, F) |
| 1 | `01` | 1 | majoritairement droite (N40) : 1934, 2038, 2190, 2294, 2453, 2502, 2654, 2709, 2758, 3340, 3771, 4027, 4339, 4491, 4699, 5330 ; gauche (N58) : 2813, 5434 | 104 / 152 / 208 (208 = deux fois 104 : une création absente, probablement refusée par la garde `9E16`, déduit) | **prouvé** (runs A, B, D, E) |
| 2 | `00` | 5 (type 5, §3.4) | 3192 (X 73, Y 70, N40), 3677 (X 77, Y 70), 5661 (X 77, Y 70) : **une seule création par entrée** dans la salle, environ 20 images après l'arrivée | - | **prouvé** |
| 3 | `00` | 5 | 2226 (X 4D, Y 30, N88) : une création, environ 20 images après l'arrivée | - | **prouvé** (run F) |
| 4 | `08` | 4 (type 4, §3.3) | 2726 droite (N48, X E5, Y AA) ; 2894 gauche (N60, X 0B, Y AA) ; 3062 droite ; 3230 gauche | **168 images** entre deux créations (= 8 × 21, voir §3.3) | **prouvé** (run G) |
| 5 | `01` | 1 | gauche (N58) : 2121, 2225, 2377, 2993, 3097, 3249 ; droite (N40) : 2481 | 104 / 152 | **prouvé** (runs D, G) |
| 6 | `01` | 1 | droite (N40) : 3388, 3492, 3644, 3748 | 104 / 152 | **prouvé** (run D) |
| 7 | `00` | 5 | 3996 (X B7, Y 50, N40), **une création**, environ 21 images après l'arrivée ; statique pendant 390 images | - | **prouvé** (run D) |

Notes :
- Les salles 0, 1, 5 et 6 suivent le drapeau `01` : création de type 1 en continu (minuterie `9CC3`).
- **Les salles 2, 3 et 7 ont un drapeau `00` mais reçoivent un objet de type 5 à l'arrivée.** Ce n'est pas le spawn du drapeau. Le créateur n'est pas identifié (§7, point 1).
- La salle 4 (drapeau `08`) donne le type 4, comme la table le prévoit.
- Les côtés droite et gauche alternent dans les données de la salle 0, mais restent majoritairement d'un seul côté dans les salles 1, 5 et 6, selon la route suivie. La règle exacte (`C42C` selon la note `enemy-spawn.md` §4) n'a pas été relevée dans cette trace (`C42C` n'est pas dans `frames.csv`) : **déduit**, non vérifié.

### 2.3 Position d'apparition et image provisoire (prouvé)

Chaque création de type 1 (et de type 4) écrit d'abord une entrée SAT provisoire : `Y = E1`, `X = 00`, pendant **2 images** (écriture `5F66`). La position réelle apparaît à l'image suivante : `X = E5` (droite, N40) ou `X = 0A` (gauche, N58), `Y = A0` (type 1). Pour le type 4 : `Y = AA` (sprite), `X = E5` ou `0B`.

Le Y de spawn du type 1 vaut `A0`, c'est-à-dire `E = C0` (comme dans `enemy-spawn.md` §4).

**Contradiction avec `enemy-spawn.md` (§4, cas `E = 60`)** : `Y = 40` est observé pour deux créations de type 1 en salle 1 (images 4339 et 4491, run A). Simon est alors à `C425 = 128` et `115`. La règle de la note (`H = 1` et `C425 < 88`) ne couvre pas ces deux cas. **Le cas `E = 60` est donc observé, mais sa condition est à revoir** (inconnu).

---

## 3. Mécanique par type

### 3.1 Type 1, sprite « droite » (N40/44/48/4C)

| Propriété | Valeur | Preuve |
|---|---|---|
| Apparition | X E5, Y A0, à 2 images de la création | prouvé |
| Direction | gauche, **-1 px par image** (X -2 toutes les 2 images) | prouvé (runs A, B, D, E, sur 218 images) |
| Animation | pose basse N48/4C ↔ N50/54, **une pose toutes les 8 images** ; la pose haute reste N40/44 | prouvé (changement toutes les 8 images sur toute la traversée, runs A, B, D, E) |
| Sortie | disparaît quand X atteint 00 (dernière image X = 00, puis Y = E0) | prouvé (runs A, B, D, E) |
| Durée de vie | environ 218 images (E5 vers 00) | prouvé |
| Contact avec Simon | observé à X ≈ 120-123 (Simon en 120) : -2 sur `C415` | prouvé (C415 32 → 30) |

### 3.2 Type 1, sprite « gauche » (N58/5C/60/64/68/6C)

| Propriété | Valeur | Preuve |
|---|---|---|
| Apparition | X 0A, Y A0 | prouvé |
| Direction | droite, **+1 px par image** | prouvé (même mesure) |
| Animation | pose basse N60/64 ↔ N68/6C, toutes les 8 images (pose haute N58/5C) | prouvé (run A, images 1692 à 1818, huit changements) |
| Sortie | disparaît à X = D8 à E7 (la borne exacte varie : D8, E7 et une sortie à l'image de changement de salle) | prouvé, borne non fixée |
| Durée de vie | environ 210 images (0A vers E7) | prouvé |

Les deux sprites ne sont pas deux types : ce sont deux sens (droite et gauche) du même type 1 (même `C800` écrit à `1`). Le spawn `5F82` écrit toujours `1`.

### 3.3 Type 4 (salle 4, drapeau `08`)

| Propriété | Valeur | Preuve |
|---|---|---|
| Création | routine `9D97` : `HL = CF06`, `DE = 9DBB` (table `14 14 14 28 14 14 14 28`, périodes 20 et 40), appel de `9CC3` ; puis `9DAB` : `X = F0` si le bit 0 de `CF07` vaut 1, sinon `10` ; `E = C425 - 8` (`9DB2`) ; `JP 5F1D` | lu dans le code ; trace (4 créations) |
| Y de sprite | `AA` observé pour Simon à `C425 = C0` (donc `E = B8` d'après le code) ; le décalage E vers sprite n'est pas vérifié pour ce type | prouvé (trace) pour `AA` ; déduit pour le décalage |
| Cadence | 168 images entre les 4 créations (2726, 2894, 3062, 3230) = 8 × 21 | prouvé (trace) ; rechargement 21 déduit (période 20 de `9DBB`, plus 1, et 8 images par tick de `9CC3`) |
| Côté | alterne droite / gauche à chaque création (bit 0 de `CF07`) | prouvé (4 cas) ; lien avec le bit 0 déduit du code |
| Mouvement | la SAT suit l'objet une image sur deux : 3 px en X toutes les 2 images (X E5 vers 01 en 154 images) | prouvé (SAT image par image, 8 épisodes, `scratchpad/type4/gfx/out3/sat4.csv`) |
| Animation | 4 poses de 16 images : vers la gauche 48, 50, 48, 58 ; vers la droite 60, 68, 60, 70 (seconde couche N+4). L'ancienne mesure « N fixe » échantillonnait mal | prouvé (même trace) |
| Sortie | disparaît à X 01 ou E6 | prouvé |
| Contact | -2 sur `C415` à 2797 et 3133, Simon en X 120 et Y 192, l'ennemi à X ≈ 122 | prouvé |

### 3.4 Type 5 (salles 2, 3, 7 ; drapeau `00`)

**Remplacé par la section « Type 5 : la panthère » en fin de document** (créateur, déclencheur, trajectoire, dessin, contact et fouet). Le tableau ci-dessous est l'ancienne mesure.

| Propriété | Valeur | Preuve |
|---|---|---|
| Création | environ 20 images après l'arrivée dans la salle, une seule fois (salles 2, 3, 7) | prouvé (3 salles, 4 créations) |
| Position initiale | salle 2 : X 73-77, Y 70, N40 ; salle 3 : X 4D, Y 30, N88 ; salle 7 : X B7, Y 50, N40 | prouvé |
| Mouvement | salle 2 (run F, image 3192) : -2,13 px par image, disparaît à X 04 après 56 images ; salle 3 (run F, image 2226) : -0,52 px par image, disparaît à X 04 après 144 images | prouvé |
| Statique | salle 7 : aucun changement de SAT sur 390 images (Simon en X 120, objet en X B7) ; salle 2 (run A) : statique jusqu'à 5722 (Simon en X 10) | prouvé |
| Activation | salle 2 (run B) : l'objet, statique en X 77 (mesure 0x73 à 4034), commence à bouger à l'image 4034 quand Simon est en X 66 à 68 (distance ≈ 48 px) | prouvé (une fois) ; **déclencheur inconnu** (hypothèse : distance inférieure à environ 60 px, déduite de 4 cas, non vérifiée) |
| Contact | aucun dégât observé en salle 2 et 7 (Y 50-70, Simon en Y 192) | prouvé (négatif) |

### 3.5 Coup de fouet

- **Aucun ennemi détruit** : 4 coups de fouet (run E, débuts aux images 1987, 2038, 2088, 2138 ; `C422` monte de 0 à 4 puis revient à 0, `C42F` de 0 à 8, soit environ 11 images par coup). Position de l'ennemi le plus proche au début de chaque coup, en X, par rapport à Simon : 33 px (coup 1, Simon en 82), 3 px (coup 2, Simon en 82), -26 px (coup 3, l'ennemi a déjà dépassé Simon en 70), 23 px (coup 4, Simon en 38). Dans aucun cas la SAT de l'ennemi ne change ou ne disparaît pendant le coup (il continue son mouvement de -1 px par image).
- **Inconnu** : la hitbox du fouet n'est pas mesurée ici (`research/whip-hitbox.md` la décrit), et l'absence de destruction ne prouve pas l'absence de dégât (un ennemi peut avoir des points de vie). Il faut un test avec un ennemi juste à la portée du fouet et plusieurs coups, puis une observation de `C800`/`D700` et de la SAT.

---

## 4. Dégâts, invulnérabilité et ce qui est subi

### 4.1 Dégâts infligés à Simon

| Source | Dégât | Preuve |
|---|---|---|
| Type 1 (contact) | -2 sur `C415` par contact | prouvé (runs A, B, D, E : 32 → 30, 30 → 28) |
| Type 4 (contact) | -2 sur `C415` | prouvé (runs G : 2797, 3133) |
| Type 5 | aucun en salle 2, 3, 7 (positions hors de la zone de Simon) | prouvé (négatif, positions non testées au sol) |

### 4.2 Recul et invulnérabilité (confirmé)

- Après un contact, `C42D` passe à `59` (89), et décroît d'une unité toutes les 2 images (runs E : 89 à 2075, 88 à 2077, 87 à 2079). Pendant ce temps Simon est repoussé vers la gauche (X 82 vers 38 en quelques dizaines d'images dans run E) et les contacts ne se répètent pas. Cohérent avec `research/player-health.md` (fenêtre de 3 s, 90 images).
- Le recul pousse Simon hors de la salle : en run A, un recul fait passer Simon de la salle 1 à la salle 0 pendant une attente (x = 246), puis une sortie à droite ramène en salle 1. **Un ennemi peut donc changer la salle** par le recul seul. Ce comportement n'était pas documenté.

### 4.3 Ce qui subit des dégâts

- Simon ne perd de l'énergie que par contact (type 1, type 4). Le fouet ne blesse personne (§3.5).
- Les ennemis ne sont pas détruits (aucune destruction observée sur 4 coups).

---

## 5. Table de mécaniques

| Type | Salle(s) | Déplacement | Vie (PV) | Dégâts à Simon | Dégâts subis (fouet) | Points | Drop | Confiance |
|---|---|---|---|---|---|---|---|---|
| 1, sprite droite (N40) | 0, 1, 5, 6 | -1 px/img, pose toutes les 8 img, sort à X 00 (218 img) | inconnu | -2 par contact | non détruit observé (4 coups) | inconnu | inconnu (aucune mort) | spawn et mouvement prouvés ; PV, points, drop inconnus |
| 1, sprite gauche (N58) | 0, 1, 5 | +1 px/img, pose toutes les 8 img, sort à X D8 à E7 | inconnu | contact côté gauche non isolé (le dégât -2 est prouvé pour le contact côté droit) | idem | inconnu | inconnu | mouvement prouvé ; dégât à confirmer |
| 4 | 4 | -1,5 px/img (sens droite) ou +1,5 (sens gauche), sans pose, sort à X 01 ou E6 (152 img), créé toutes les 168 img | inconnu | -2 par contact | non testé | inconnu | inconnu | spawn, mouvement et contact prouvés |
| 5 | 2, 3, 7 | statique ou -0,5 à -2,1 px/img après activation (déclencheur inconnu), sort à X 04 | inconnu | aucun observé | non testé | inconnu | inconnu | spawn et mouvement prouvés, dégâts négatifs à hauteur Y 50 à 70 |

**Drop et points** : aucun ennemi détruit n'a été observé, donc aucune table de points ni de drop n'est mesurée. Le fouet ne tue pas les ennemis de type 1 dans les 4 coups testés. Le lien avec `research/candles-and-drops.md` reste inconnu (le drop cité là-bas concerne une torche de la cour, pas un ennemi).

---

## 6. Corrections et contradictions relevées

1. **`enemy-spawn.md` §4 (cas `E = 60`)** : la condition `C425 < 88` ne correspond pas aux deux cas observés (Y 40 à Simon Y 128 et 115). Le cas existe, sa condition est à revoir.
2. **`enemy-spawn.md` §2 (drapeaux)** : la salle 7 (drapeau `00`) a un objet de type 5 à l'arrivée. La salle 2 et la salle 3 en ont aussi un. Le drapeau ne décrit donc pas tous les objets qui apparaissent dans une salle.
3. **`enemy-spawn.md` §4 (type par bloc)** : le type 1 apparaît dans `C800` ou `C880` indifféremment. Le côté (droite ou gauche) détermine le motif de sprite (N40 ou N58), pas le bloc. La phrase « tir 1 dans C800, tir 2 dans C880 » est à retirer.
4. **`enemy-spawn.md` §3 (cadence)** : les intervalles 104 et 152 sont confirmés (runs D : 1934, 2038, 2190, 2294, 2502, 2654, 2758 donnent 104, 152, 104, 208, 152, 104).
5. **`mechanics-remaining.md` (M08b, M09)** : le comportement de l'ennemi (1 px/img, pose toutes les 8 images, sortie) est maintenant prouvé pour le type 1 (les deux sens) et le type 4.
6. **Routes** : les salles 3 et 4 n'avaient pas été atteintes dans les runs précédents ; les plans D et G les atteignent (run F : salle 3 ; run G : salle 4). Le run A ne visite pas la salle 3 (il revient en salle 0 par recul).

---

## 7. Inconnues

Les points 1, 2 et 7 sont résolus dans la section « Type 5 : la panthère ».

1. **Créateur du type 5** (salles 2, 3, 7) : le bloc `C800` est écrit par `5F82` (appelé par `5F1D`). La pile au moment de l'écriture a été lue (`scratchpad/enemy-behavior/runD-c/callers.txt`). Un mot de pile (`4382`) est compatible avec un retour après `CALL 62F5` (`437F`, routine de chargement de salle), mais ce n'est pas confirmé. Aucun des sites d'appel de `5F1D` examinés ne charge `C = 5` directement : le type vient donc probablement d'une table ou d'un calcul (déduit).
2. **Déclencheur d'activation du type 5** (et sa portée).
3. **Points de vie** de chaque type, **points**, et **drop** : aucun ennemi détruit.
4. **Hitbox du fouet** et hitbox de l'ennemi (16 × 32 supposé, non vérifié).
5. **Règle de côté** (`C42C`) pour le type 1 : non relevée.
6. **Condition du cas `Y = 40`** (type 1, salle 1) : non identifiée.
7. **Dégâts du type 5 au sol** : non testés (les positions observées sont hors de la zone de Simon).
8. **Table de spawn de type 4** : la période et les côtés proviennent de la lecture de `9D97` et de quatre créations. La séquence complète (bit 0 de `CF07`, valeurs 20 et 40 de `9DBB`) n'est pas vérifiée au-delà de quatre créations.
9. **Sortie à droite** : la borne de disparition (D8 ou E7) n'est pas fixée.

---

## 8. Fichiers

- Trace : `tools/enemy_behavior_trace.tcl` (lecture seule, plan d'actions), `tools/run_enemy_behavior_trace.py ROM OUT PLAN [TIMEOUT]`, option `EB_EXTRA=tools/enemy_spawn_writers.tcl` (écritures SAT et `C800/C880`, avec PC).
- Sonde : `tools/enemy_spawn_callers.tcl` (pile et registres à l'écriture de `C800`).
- Analyse : `tools/analyze_enemy_behavior.py RUNDIR`.
- Données de travail (non versionnées) : `scratchpad/enemy-behavior/` (ROM copiée, plans `plan-A` à `plan-G`, runs A, B, D, E, F, G et leurs variantes `-w` ; `runD-c/callers.txt`).

## Type 4 : fouet et table des objets (état actuel)

- **Table des objets** (RAM MSX2 `C800` et `C880`, deux blocs de 128 octets) : premier octet = type (`00` libre, `01` type 1, `04` type 4, `05` type 5, `1E` et `24` effets de fin). Y à +2/+3 (SAT Y = +3 - 15, prouvé sur le type 4), X à +4/+5 (SAT X = +5 - 8, prouvé). Les enregistrements de sprite (`[k][Y][X][N][C]`) commencent à +0x21.
- **Fouet** : sur 17 instances de type 4 (trace `scratchpad/type4/out2`), 12 meurent. Chaque mort a lieu sur la première image utile de la phase 3 du fouet (`C422` = 3), où la boîte du fouet touche l'ennemi, ou sur la suivante. Un objet `1E` apparaît au même X/Y. Les 4 autres finissent par sortie d'écran, sans contact. Calibrage : 3 morts de type 1 suivent le même schéma (`scratchpad/enemy-behavior/runC`).
- **Géométrie** : ajustée, non contrôlée. Dans le repère de la table, boîte de l'ennemi de X-8 à X+7 et de Y-8 à Y+7. Aucun écart sur 17 échantillons. Les bornes exactes restent à ±1-2 px.
- **Code** : `Enemies_whipHit` tue les types 1 et 4. La boîte du type 4 est provisoire (16 x 16 en coordonnées d'écran).
- **Ouvert** : la direction du fouet n'a pas été lue dans la trace (les morts à x=200 se produisent côté gauche, alors que le plan demandait la droite). Une trace contrôlée avec l'ennemi posé en bordure de boîte fixerait les bords.
- **Preuves** (hors dépôt) : `scratchpad/type4/whip-kill-type4.md`, `scratchpad/type4/object-table-format.md`, `scratchpad/type4/analysis/`.

## Trajectoire du type 4

Mesurée image par image en salle 4 (2026-10-27, `scratchpad/type4/gfx/out3/sat4.csv`, 8 épisodes) : la courbe Y est identique à chaque tir et dans les deux sens. Par pas de 2 images depuis l'apparition, Y SAT = 170, 171, 172, 174, 175, 175, 176, 177, 178, 178, 179 (×8), 178, 178, 177, 177, 176 … jusqu'au haut 158 (pas 42 à 45), puis redescend à 178 (pas 68 à 76). Le pas 18 manque (clignotement du multiplexage), interpolé à 178. Table complète : `kType4Y` dans `sgdk/src/enemies.c`.

Dessin : flux RLE de la ROM en `0x154DE`, octets 271 à 462 (paires 48/4C, 50/54, 58/5C) ; vers la droite, les mêmes paires en miroir (60/64, 68/6C, 70/74). Vérifié pixel par pixel contre la VRAM de la salle 4. Couleurs : 02 et 44 (CC), comme les types 1. Il ressemble à une chauve-souris (corps noir, contour cyan, face beige).

## Type 5 : la panthère (salles 2, 3 et 7)

**Porté** dans `sgdk/src/enemies.c` (`pantherStep`, `pantherSpawn`, `Enemies_whipHitType5`), tuiles par `tools/build_enemy_sprites_md.py` (`vk_enemy5_tiles`, VRAM 760+), tests `test_type5` et scénario `g_room2_panther`. Non reproduits : retard d'affichage d'une mise à jour, objet `1E`, score, contact accroupi.

Relevé du 2026-10-27/28 (openMSX, SAT et objets lus à chaque image, plus lecture du code). Il **remplace le tableau du §3.4** et ferme les points 1, 2 et 7 du §7. Le type 5 est une panthère : elle est assise, bondit vers Simon quand il approche, saute au bord des plateformes et retombe, puis repart vers Simon.

Repère : celui de la table des objets (`C800`), qui est celui de Simon (`C427` = X, `C425` = Y des pieds). Objet : `+1` état, `+2/+3` Y en 8.8, `+4/+5` X en 8.8, `+6` déplacement actif, `+7/+8` vitesse Y (8.8, signée), `+9/+A` vitesse X, `+B` pose, `+C` compteur d'animation, `+D` points de vie, `+E` drapeaux (bit 0 : touchable au fouet, bit 1 : contact), `+F` numéro de l'entrée de liste.

Bancs : routines du type 5 en `A000`, banque fichier `0x6000` (fichier = `0x6000 + (adresse - A000)`) ; `8000` = banque fichier `0x4000` ; `6000` = banque fichier `0x2000`. Mapping lu dans un vidage mémoire (`scratchpad/type5/out/B/mem_f2700.bin`, `slots_f2700.txt`) et confirmé par les PC des écritures (`out/B/objpcs.csv`).

### Apparition (prouvé)

| Point | Valeur | Preuve |
|---|---|---|
| Source | Liste d'objets du hall en ROM fichier `0x1C674` (CPU `8674`, banque `0x0E`), 14 octets `00 00 05 88 00 85 46 00 00 00 00 85 6C FF` (SHA-256 `9d9e008b07cd5dc9885be4a296d7e6e6effd715a589c8e1056af10b85f5cd904`). Format : paires `type, position` par salle, `00` passe à la salle suivante, `FF` termine | lu dans le code (`6181`) et dans la RAM `DB00` (vidage) |
| Copie | Au chargement de chaque salle, `62F5` appelle `6154` (recopie de la liste vers `DB00 + 16 × salle`, 4 entrées de 4 octets) puis `JP 61BB` (création). La liste est relue à chaque entrée : **la panthère réapparaît à chaque visite**, même tuée | code ; trace : 2 créations en salle 3 et 2 en salle 7 dans `out/A` |
| Création | `61BB` : pour chaque entrée non nulle, `C = type & 7F`, `E = position & F0` (Y), `D = (position & 0F) × 16` (X), `A = numéro d'entrée` (vers `+F`), puis `CALL 5F1F`. Le bit 7 du type (`85` en salles 3 et 7, `05` en salle 2) est masqué ; son rôle ailleurs est inconnu | code ; PC `5F82` et pile `61F9` dans `out/A/objwrites.csv` |
| Positions | salle 2 : X `80`, Y `80` ; salle 3 : X `60`, Y `40` ; salle 7 : X `C0`, Y `60` | prouvé (`out/A/sat5.csv`) |
| Instant | 21 ou 22 images après l'écriture de `D001` (fin du chargement de salle) : 2667→2688, 2929→2950, 3601→3622, 4361→4383, 5223→5245 | prouvé |
| Nombre | une seule panthère par salle (une entrée de liste) | prouvé |
| Initialisation (`A85C`) | pose `43` (assise, tournée à droite) si `C427 >= X`, sinon `3F` (assise, à gauche) ; `+6 = 1`, `+C = 0`, vitesses X et Y nulles ; état 0. Points de vie `+D = 1` (table `60E1 + type`), `+E = 7`, 4 entrées SAT (table `6057 + type`) | code ; valeurs lues dans `out/B/objpcs.csv` (`5FC0` écrit `+D = 1`) |

### Mise à jour (prouvé)

Les objets sont mis à jour **une image sur deux**. À chaque mise à jour, dans cet ordre (boucle `98EC`) : routine du type (`A87D`), déplacement (`99B9` : `Y += vy`, `X += vx` en 8.8 si `+6 ≠ 0`), bornes (`99E5`).

1. **Sortie à gauche** : si `X < 18` au début de la routine, l'objet est supprimé (`JP 99F6`). Le déplacement s'applique encore, d'où le X `10` lu après la suppression. Dernier X affiché : `14`.
2. **État 0, assise** (`A891`) : `d = |C427 - X|`. Si `d < 40` (64 px), `vx = +0x0400` si `C427 >= X`, sinon `-0x0400`, puis état 1. Aucune condition sur Y. Trace : activation à `d = 62` dans les 5 cas (salle 3 : Simon 158 et 34, panthère 96 ; salle 2 : 190 contre 128 ; salle 7 : 130 contre 192). Le pas de Simon (2 px par mise à jour) ne permet pas de distinguer 62 et 63 ; le code donne `< 64`.
3. **État 1, course** (`A8AC`) : animation (`A909`), puis test de sol au point `(X, Y)` et `(X - 10, Y)` (`7B98` = `7D2F` + `7C5E`, les mêmes fonctions que `vk_sample_tile` et `vk_tile_solid` du port). S'il y a du sol en l'un des deux points, la course continue. Sinon : `vy = -0x0108`, pose `46` (vers la droite) ou `42` (vers la gauche, bit 7 de `+A`), état 2. **Pas de demi-tour pendant la course**, même si Simon est dépassé (salle 2, run A ; tests `WS4`/`WS8`).
4. **État 2, saut ou chute** (`A8D2`) : `vy += 0x00A0`, plafonnée à `0x07FF` dès que l'octet fort atteint 8 (`A549`). Test de sol au point `(X, Y)` et `(X - 10, Y)`. S'il y a du sol : atterrissage, `vx` vers Simon (`+0x0400` si `C427 >= X`, sinon `-0x0400`), `+C = 0`, état 1, `Y = Y & F8` (fraction à 0), `vy = 0`. La pose reste celle du saut jusqu'à la mise à jour suivante.
5. **Bornes** (`99E5`, communes) : suppression si `Y >= E4`, `X >= F1` ou `X < 07`. Sortie à droite observée : X `F0` affiché, suppression à `F4` (salle 2).

Vitesse X : 4 px par mise à jour, soit **2 px par image**, en course comme en saut. La vitesse X ne change qu'à l'activation et à l'atterrissage.

### Courbe du saut (prouvé, table complète)

Pas = une mise à jour (2 images), depuis la mise à jour qui déclenche le saut (le déplacement s'applique dans la même mise à jour). Y de départ entier (fraction 0). `ΔY` = Y 8.8 cumulé, `Y` = partie entière pour un départ en `80`.

| Pas | vy (8.8) | ΔY (1/256 px) | Y (départ 80) |
|---|---|---|---|
| 0 | `-0x0108` | -264 | `7E` |
| 1 | `-0x0068` | -368 | `7E` |
| 2 | `+0x0038` | -312 | `7E` |
| 3 | `+0x00D8` | -96 | `7F` |
| 4 | `+0x0178` | 280 | `81` |
| 5 | `+0x0218` | 816 | `83` |
| 6 | `+0x02B8` | 1512 | `85` |
| 7 | `+0x0358` | 2368 | `89` |
| 8 | `+0x03F8` | 3384 | `8D` |
| 9 | `+0x0498` | 4560 | `91` |
| 10 | `+0x0538` | 5896 | `97` |
| 11 | `+0x05D8` | 7392 | `9C` |
| 12 | `+0x0678` | 9048 | `A3` |
| 13 | `+0x0718` | 10864 | `AA` |
| 14 | `+0x07B8` | 12840 | `B2` |
| 15 et suivants | `+0x07FF` | +2047 par pas | `BA`, `C2`, `CA`, `D2`… |

Les 6 sauts de `out/A` (départs Y `40`, `80`, `80`, `60`, `80`, `A0`) suivent cette table à l'octet près, fraction comprise (`scratchpad/type5/analysis/jump.py`). Exemples d'atterrissage : salle 3, départ `40` à X `84`, posé en Y `80` à X `C4` (17 pas) ; salle 7, `60` vers `80` (13 pas), puis `80` vers `A0`, puis `A0` vers `C0`.

### Animation et dessin des poses (prouvé)

Compteur `+C` : `+1` par mise à jour en course, de 1 à 12 puis retour à 1 (`A909`). Pose de base `41` (vers la gauche) ou `45` (vers la droite) pour 1-3, base + 1 pour 4-9, base - 1 pour 10-12. Cycle de 12 mises à jour = 24 images : 6 images, 12 images, 6 images. Le compteur est figé pendant le saut (pose `42` ou `46`). Après l'activation, la pose assise reste une mise à jour, puis le cycle commence à 1.

Motifs : `N = 40 + 16 × (pose - 3F)`, deux cellules 16 × 16, chacune à deux couches (N et N+4). Positions par rapport à l'objet (X, Y), mesurées sur 981 images sans exception (`analysis/offsets.py`) :

| Pose | Usage | Cellule gauche (couches) | Cellule droite (couches) |
|---|---|---|---|
| `3F` | assise, tournée à gauche | `40/44` en X-9 | `48/4C` en X+7 |
| `40` | course gauche, 10-12 | `50/54` en X-16 | `58/5C` en X |
| `41` | course gauche, 1-3 | `60/64` en X-16 | `68/6C` en X |
| `42` | course gauche, 4-9, et saut | `70/74` en X-16 | `78/7C` en X |
| `43` | assise, tournée à droite | `88/8C` en X-23 | `80/84` en X-7 |
| `44` | course droite, 10-12 | `98/9C` en X-16 | `90/94` en X |
| `45` | course droite, 1-3 | `A8/AC` en X-16 | `A0/A4` en X |
| `46` | course droite, 4-9, et saut | `B8/BC` en X-16 | `B0/B4` en X |

- SAT Y = Y - 16 pour toutes les poses (lignes Y-15 à Y à l'écran).
- **Décalage d'affichage** : la SAT lue à l'image f montre l'objet de l'image f-2 (une mise à jour de retard), sur les deux parités d'image (146 sur 150 contre 0 sur 150 pour les autres décalages ; les 4 écarts sont des transitions).
- **Miroir** : la couche `N + 40` est le miroir horizontal exact de la couche N (16 paires, `mirror_layer` de `tools/extract_enemy_sprites.py`). Le corps est symétrique autour de X - 0,5 : la pose à droite est la pose à gauche retournée.

### Graphismes (prouvé)

- Flux RLE en **ROM fichier `0x15AB9`** (banque 10, décalage `0x1AB9`), 417 octets avec le `00` final (`0x15AB9` à `0x15C59`), sans saut `80` ; décodage par `rle_decode` de `tools/extract_enemy_sprites.py`. SHA-256 des octets bruts : `97bbe84c677bd4a9338cb2e0fdda5065e1b2d7cb6e0b7b4d6347ab4f4415c697`.
- Sortie : **512 octets = les 16 couches `40` à `7C`** dans l'ordre (32 octets par couche, quadrants UL, LL, UR, LR). SHA-256 de la sortie : `c0845ef045b49981e35d7b640acc6ea6438668f075197a49573b34cbed39c4df`. Les couches `80` à `BC` sont les miroirs (calculés par le jeu).
- Vérification : les 32 couches `40` à `BC` lues en VRAM (`F800 + 8 × N`) sont identiques octet pour octet dans 38 instantanés des salles 2, 3 et 7 (`out/A/snaps/`), et les 16 premières sont égales à la sortie du flux. Le flux a été trouvé en décodant chaque décalage de la ROM (`analysis/findrle.py`) : c'est le seul. Aperçu : `analysis/poses.png`.
- Couleur : table de couleur (512 octets avant la SAT) à `02` pour la couche N et `44` (couleur 4, CC) pour la couche N+4, sur les 16 lignes, dans les 76 entrées vérifiées. Recouvrement = couleur 6. Même règle que les types 1 et 4.

### Palette (prouvé)

Palette VDP des salles 2, 3 et 7 (37 instantanés) : `0000 08AE 0222 064C 0CC8 0800 004C 0040 020E 0006 0008 0268 0AAA 0444 0EEE 0E00` en format MD, **égale à `vk_palettes[2]`, `[3]` et `[7]`** de `sgdk/src/generated/rooms.c` (hall = `vk_stages[0]`, premier index 0). Couleurs utiles : 2 = `0222` (corps noir), 4 = `0CC8` (contour), 6 = `004C` (taches). Les salles 0 et 1 ont d'autres valeurs en 4 et 6 (`0C2E`, `0ACE`).

### Contact (prouvé)

- Test `8290` (même boucle `7E7D` que les autres types), classe 2 de la table `82CB` (type 5 → `03`), paramètres `HL = 0A0C` (`83D2`) : touche si `|C427 - X| < 17` et `0 <= C425 - 2 - Y + 10 < 36` (debout ; `28` si `C420` vaut 1 ou 2, et `C425 - 8` si `C420 = 1`). Debout, cela donne Y de `C425 - 27` à `C425 + 8`.
- **Dégât : 6** (`81CE`, entrée du type 5 = `01 03` : base 3, doublée si le bit 4 de `C701` est nul, `816C`). Trois contacts mesurés : 24 → 18 (salle 7, `out/A` image 5421, panthère en saut), 32 → 26 (`WC10`, `WS4`, `WS8`, salle 2). Recul et invulnérabilité `C42D = 89` comme pour les autres types. La panthère continue sa course à travers Simon.

### Fouet (prouvé)

- Test `7DAD` : uniquement en phase `C422 = 3`, pour les objets dont le bit 0 de `+E` est mis ; boîte `84A7` avec les mêmes paramètres (`L = 0C`, `H = 0A`) : X du fouet = `C427 + 20` (droite, `C42C = 0`) ou `C427 - 20` (gauche). Touche si `|X fouet - X| < 24` (12 + 12). Si `C416 = 1` (niveau de fouet, lu dans le code), la portée passe à 24 et la demi-largeur à 28 et `C425 - B - 3 <= Y <= C425 - B + 10`, avec `B = 18` debout et `B = 10` accroupi (`C420 = 2`).
- Conséquence : **debout, le fouet passe au-dessus d'une panthère posée au même niveau que Simon** (Y de `C425 - 21` à `C425 - 8`) ; **accroupi, il la touche** (Y de `C425 - 13` à `C425`).
- Points de vie 1 : `+D` passe à 0 au premier coup (2 si `C416 ≠ 0`), puis `81AB` (score, octet `01` de `81CE`, comme le type 1) et `9A3E` (mort : objet `1E` à la même position, drop aléatoire commun à tous les ennemis).
- Mesure (salle 2, Simon posé sur la plateforme de la panthère en `x 194, y 128` par poke, tourné à gauche, puis un pas à gauche pour l'activer) : accroupi (`WC4`, `WC6`, `WC8`), **mort au premier coup**, à la première image de phase 3, panthère en X 160, 164 et 168 ; `C800` passe de `05` à `1E` puis `00`. Debout (`WS4`, `WS8`) : phase 3 avec la panthère en X 160 et 168, **pas de mort**, puis contact et -6. `WC10` : contact avant la phase 3.

### Reste ouvert

- Rôle du bit 7 du type dans la liste (`85` contre `05`) : ignoré par `61BB`, aucun autre lecteur trouvé.
- Points de score : octet `01` de `81CE`, même valeur que le type 1 (déduit, non mesuré pour le type 5).
- Le niveau de fouet (`C416`) des tests n'a pas été lu ; les tests valent pour le fouet de départ.
- Bornes exactes du contact et du fouet : données par le code, non balayées au pixel en trace (les cas mesurés sont compatibles).

### Fichiers (hors dépôt)

- Outils : `scratchpad/type5/tool/` (copies de `harness.tcl`, `run.py`, `render_snap.py` ; `t5_extra.tcl` : SAT et couleur par image en salles 2, 3, 7, objets `C800/C880`, écritures avec PC et pile, instantanés VRAM à chaque nouvel ensemble de motifs ; `t5_code.tcl` : vidage mémoire et PC des écritures dans l'objet).
- Plans : `scratchpad/type5/plans/` (`K1` : chemin naturel salles 3, 2, 3, 7 ; `B` : jusqu'à la salle 2 ; `WC4`-`WC10`, `WS4`, `WS8` : tests du fouet).
- Traces : `scratchpad/type5/out/A/sat5.csv` (2876 images en salles 2, 3, 7), `out/A/objwrites.csv`, `out/A/snaps/` ; `out/B/` (vidage mémoire) ; `out/WC*/`, `out/WS*/` (fouet).
- Analyses : `scratchpad/type5/analysis/` (`norm.py`, `A_norm.txt`, `offsets.py`, `offsets2.py`, `layers.py`, `vram_layers.bin`, `findrle.py`, `poses.png`).

## Stage 2 : ennemis (relevé du 2026-10-28)

Lecture du code (`tools/disasm_rom_window.py`) et trace openMSX `scratchpad/stage2e/out/A` (plan `S2` = K3 puis stage 2 ; SAT, objets `C800`-`CB7F`, instantanés VRAM).

| Point | Valeur | Preuve |
|---|---|---|
| Drapeaux de salle | `85D7`-`85DC` (pointeur `85AA`) : `08 08 01 08 02 02` (salles 0 à 5). Bit 3 (`9D97`, type 4) en salles 0, 1, 3 ; bit 0 (`9CE6`, type 1) en salle 2 ; bit 1 (`9D4B`, type 2) en salles 4, 5 | ROM |
| Objets fixes | aucun : la liste du stage 2 (appel 2 de `6181` depuis `8674`) est vide (`FF`) | ROM |
| Répartition des routines (`993B`, table `9942`) | type 1 `A964`, types 2 et 3 `A310`, type 4 `B0F8`, type 5 `A87D` | ROM |
| Type 4 : dessin | les 12 couches `48`-`74` en VRAM sont identiques à celles du hall (5 instantanés des salles 0, 4, 2) | trace |
| Type 4 : Y du spawn | `9D97` : `E = C425 - 8` ; Y SAT au spawn = **C425 - 22** (Simon 96 → 74, Simon 128 → 106 ; hall : 192 → `AA`) | code + trace (20 tirs) |
| Type 4 : cadence | table `9DBB` = `14 14 14 28 14 14 14 28` : 168, 168, 168 puis 328 images (tirs 6068, 6236, 6404, 6572, 6900, …) ; le premier tir vient 168 images après l'entrée | code + trace |
| Type 1 (salle 2) | `9CFC` branche stage 2 : `E = A0`, côté `D = F0`, ou `10` si le bit 1 de `CF01` est mis | code seul |
| Type 2 (salles 4, 5) | `9D4B` : minuterie `CF02`, périodes `9D8F` (`01 18 18 …`), Y `C8`, X = table `9D87` (`60 D0 30 90 A0 40 60 B0`) selon `CF03 & 7`, décalé de 32 px vers le centre si Simon est à moins de 24 px. Routine `A310` : chute sous gravité `0x80`, posé au sol puis marche vers Simon à ±`0x180` ; sous Y `C0` sans sol, disparaît en créant deux objets `20` (éclaboussures). Homme-poisson probable | code seul |

**Porté** : type 4 au stage 2 (salles 0, 1, 3), Y du spawn relative à Simon et cadence 168/328 pour tous les stages. **Non porté** : type 1 de la salle 2 (dessin au stage 2 non relevé), type 2 (dessin, sauts et tir de l'état 2 `A45A` non relevés). La trace n'a pas atteint les salles 2 et 4 : le lien forcé (`C41E`) n'a pas servi, Simon était sur un escalier.
