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
| Cadence | 168 images entre les 4 créations (2726, 2894, 3062, 3230) = 8 × 21 | prouvé (trace) ; rechargement `CF06 := 9DBB[CF07 & 7] + 1` = 21 vu dans `CF06` (trace `scratchpad/fidelity/out1`, 2026-10-29) |
| Premier tir | `9CA9` met `CF06 = 08` au chargement de salle. `9CC3` décrémente `CF10` (compteur partagé par tous les canaux, jamais remis à zéro hors du début de partie `44C6`) à chaque appel, une passe de l'état 5 sur deux images, et `CF06` seulement quand `CF10 & 3` tombe à 0. Le tir vient au 8e de ces appels : appel `j + 28`, `j` (1 à 4) = rang du premier appel qui amène `CF10` à un multiple de 4, soit **56, 58, 60 ou 62 images après la première passe**. Trace : `CF10` = 206 → tir à +58 (2218 → 2276), 250 → +58 (après une vie perdue, 2749 → 2807), 25 → +56 (3239 → 3295) ; les 56-57 de `research/room-links.md` 9.4 sont le cas `j = 1` | **prouvé** (trace et code) |
| Côté | `D = F0` si le bit 0 de `CF07` (après l'incrément) vaut 1, sinon `10` : droite, gauche, droite… ; puis `9E16` : Simon à `X < 40` → `D = F0`, Simon à `X ≥ C0` → `D = 10` (pas de refus : `9D97` ignore la retenue) | prouvé (run G : alternance ; trace `out1`, Simon en X 16 : 9 tirs sur 9 à droite) |
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
8. ~~Table de spawn de type 4~~ : résolu (§3.3 : premier tir, `CF06`/`CF07`, garde `9E16`, trace `scratchpad/fidelity/out1` ; période longue 328 vue au stage 2).
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
| Type 4 : cadence | table `9DBB` = `14 14 14 28 14 14 14 28` : 168, 168, 168 puis 328 images (tirs 6068, 6236, 6404, 6572, 6900, …) ; le premier tir vient 56 à 62 images après la première passe de l'état 5 (§3.3) | code + trace |
| Type 1 (salle 2) | `9CFC` branche stage 2 : `E = A0`, côté `D = F0`, ou `10` si le bit 1 de `CF01` est mis. C’est le zombie du hall (section « Type 1 au stage 2 ») | code + trace (2026-10-28) |
| Type 2 (salles 4, 5) | `9D4B` : minuterie `CF02`, périodes `9D8F` (`01 18 18 …`), Y `C8`, X = table `9D87` (`60 D0 30 90 A0 40 60 B0`) selon `CF03 & 7`, décalé de 32 px vers le centre si Simon est à moins de 24 px. Routine `A310` : chute sous gravité `0x80`, posé au sol puis marche vers Simon à ±`0x180` ; sous Y `C0` sans sol, disparaît en créant deux objets `20` (éclaboussures). C'est l'homme-poisson (section « Type 2 au stage 2 ») | code + trace (2026-10-28) |

**Porté** : type 4 au stage 2 (salles 0, 1, 3), Y du spawn relative à Simon et cadence 168/328 pour tous les stages. **Porté depuis** : type 1 de la salle 2 (section suivante). **Porté depuis** : type 2 des salles 4 et 5 (section « Type 2 au stage 2 » ; le tir de l'état 2 `A45A` est réservé au type 3). La trace `stage2e` n’a pas atteint les salles 2 et 4 : le lien forcé (`C41E`) n'a pas servi, Simon était sur un escalier.

## Type 1 au stage 2 : le zombie de la salle 2 (relevé du 2026-10-28)

**Identification** : l'ennemi de type 1 de la salle 2 du stage 2 (`D000 = 2`, `D001 = 2`) est **le zombie du hall** (types A et B du port). Même routine (`A964`), mêmes couches de motifs, mêmes couleurs ; seuls le Y du spawn et la règle de côté changent.

**Porté** dans `sgdk/src/enemies.c` (`stage2Type1Tick`, champ `romBox` de `Enemy`, `Enemies_whipHitType1Rom`) et `sgdk/src/actors.c` (fouet en phase 2). Aucune tuile nouvelle : les couches du hall (`vk_enemy_tiles`, VRAM `ENEMY_TILE_INDEX`) sont déjà chargées dans toutes les salles du château. Tests : `test_stage2_type1_spawn`, `test_stage2_type1_trajectory`, `test_stage2_type1_contact_whip` (`tests/test_enemies.c`) et le scénario MD `k_stage2_room2_zombie`.

### Méthode

- Trace openMSX (outils copiés du type 5 dans `scratchpad/s2t1/tool/`, extra `s2t1_extra.tcl` : SAT et couleur par image, objets `C800`-`CB00`, instantanés VRAM à chaque nouvel ensemble de motifs, filtre `D000 = 2` et `D001 = 2`). Plan `base.txt` = plan `S2` jusqu'à l'arrivée au stage 2 (salle 0, x 16, y 96), puis chemin naturel : `gotox 44`, HAUT sur l'escalier de la salle 0, arrivée en salle 2 à l'image 6190, Simon en haut de l'escalier (x 145, y 160).
- Runs : `out/O1` (observation, 6 × 300 images, énergie remise à 32), `out/W1` (coups de fouet debout à droite, puis à gauche, puis accroupi). Analyses : `ana.py` (créations, suivi d'un bloc), `satx.py` (X SAT image par image), `hits.py` (contacts et morts), `cmp_vram.py` (couches contre la ROM), `colors.py`, `pal.py`.
- Lecture de code : `9CE6`, `9CFC`, `9E16`, table d'initialisation `5FCC`, `A934`, `A964`, `99E5`, `7DAD`, `827C`/`8290` (classes `82CB`), `8378`/`8380`, `847A`, `84A7`, `84D9`.

### Code (déduit de la lecture, confirmé par la trace pour les points marqués)

| Point | Valeur |
|---|---|
| Spawn | `9CE6` : minuterie `9CC3` (`CF00`/`CF01`, périodes `9D43`, comme au hall), puis `9CFC` : branche `D000 = 2` (`9D1B`) : `E = A0`, `D = F0` ; `D = 10` si le bit 1 de `CF01` (déjà incrémenté par `9CD1`) est mis (`9D1F`). Garde `9E16`, puis `C = 1`, `JP 5F1D`. Le drapeau `C42C` n'est pas lu au stage 2 |
| Initialisation | table `5FCC` (type 1 `A934`, types 2-3 `A2E0`, type 4 `B0CA`, type 5 `A85C`). `A934` : `+6 = 1` ; si `X < 80`, `vx = +0x0220`, pose `3D`, `+10 = 0` ; sinon `vx = -0x0220`, pose `3B`, `+10 = 1` ; `vx` recopiée en `+11/+12` ; `+C = 8` ; `vy = 0`. Points de vie `+D = 01`, `+E = 07` (lus dans l'objet) |
| Mise à jour `A964` (une image sur deux) | `+C - 1` ; à 0 : la pose alterne `3B`/`3C` (vers la gauche) ou `3D`/`3E` (vers la droite), `+C = 4`. Sol testé en `(X, Y)` et `(X - 10, Y)` (`7B98`) : posé, `vy = 0`, `vx` reprise de `+11/+12`, `Y &= F8` ; sans sol, `vx = 0` et `vy += 0x60` (`A549`) : chute verticale |
| Bornes `99E5` | suppression si `X < 07`, `X >= F1` ou `Y >= E4` |
| Contact `8290` | classe 1 (`82CB` : types 1, 2, 3 → 1 ; 4 → 2 ; 5 → 3), `HL = 1805` (`8378`) puis `847A` : `|C427 - X| < 10` et `0 <= C425' - 2 - Y + 24 < H`, avec `H = 50` debout, `42` si `C420` vaut 1 ou 2, et `C425' = C425 - 8` si `C420 = 1`. Debout : Y de `C425 - 27` à `C425 + 22` |
| Fouet `7DAD` | phase `C422 = 3`, objets au bit 0 de `+E` ; classe 1, `HL = 1805` (`8380`) puis `84A7` : demi-largeur `L = 5 + 12 = 17` (`5 + 16` si `C416 = 1`) ; X du fouet = `C427 + 20` (`C42C = 0`) ou `C427 - 20` (`± 24` si `C416 = 1`), une retenue annule le coup (`84B6`) ; touche si `|X fouet - X| < 17` et `0 <= C425 - B - Y + 24 < 28`, `B = 18` debout, `10` accroupi (`C420 = 2`), soit Y de `C425 - B - 3` à `C425 - B + 24` |

### Trace (prouvé)

| Point | Valeur | Preuve |
|---|---|---|
| Créations | images 6238 (droite), 6342 (gauche), 6494 (gauche), 6598 (droite), 6702 (droite), 6806 (gauche), 6958 (gauche) : écarts 104, 152, 104, 104, 104, 152 ; côtés D, G, G, D, D, G, G = bit 1 de `CF01` = 1 à 7 | `O1`, `ana.py` |
| Premier tir | 48 images après l'écriture de `D001` (6190 → 6238) | `O1` |
| Objet à la création | `01 00 00A0 E0ED 01 0000 E0FD 3B 07 01 07 00 01 E0FD` (droite : Y `A0`, X `ED.E0` après une mise à jour depuis `F0`) ; à gauche X `12.20` depuis `10`, `vx = 0x0220`, pose `3D`, `+10 = 0` | `O1` |
| Y | Y de l'objet `A0` à chaque tir (Simon en `C425 = A0`, même sol) ; SAT : rang haut Y 128 (`80`), rang bas 144 (`90`) | `O1`, `W1` |
| X SAT | X de l'objet - 8 ; premier X visible `E5` (droite) ou `0A` (gauche), 2 images après l'écriture de `C800` | `satx.py` |
| Mouvement | `-2,125` px par mise à jour : 2 px toutes les 2 images et 1 px de plus toutes les 16 images ; séquence identique à celle du hall, décalée d'une image (âge = image - création - 1 dans les tests) | `satx.py`, `test_stage2_type1_trajectory` |
| Animation | rang bas `48/4C` ↔ `50/54` (vers la gauche) ou `60/64` ↔ `68/6C` (vers la droite) toutes les 8 images, premier changement 16 images après la création ; rang haut fixe `40/44` ou `58/5C` | `satx.py` |
| Sortie | vers la gauche : X SAT `00` pendant 2 images, suppression à X objet `06.40` ; vers la droite : X SAT `E7` pendant 2 images, suppression à X objet `F1.40`. Durées : 220 et 212 images après la création | `O1` |
| Contact | 10 contacts distincts (`O1`, `W1`, dont un sur l’escalier, `C420 = 3`) : `C415 - 2`, à `|C427 - X|` de 8 à 10 à l’image précédente, soit moins de 10 après le déplacement de l’image du test ; aucun au-delà. Recul et invulnérabilité comme au hall | `hits.py` |
| Fouet | 6 morts (`W1`), toutes en phase `C422 = 3`, debout, à `|X fouet - X|` de 0 à 15 (9, 12-14, 12, 0, 0-2, 13-15). L'objet devient `1E` en (X, Y - 16), puis `00`, ou `24` (objet tombé) 3 fois sur 6 | `hits.py`, `ana.py` |
| Dessin | les 12 couches `40`-`6C` en VRAM (`F800 + 8 × N`) sont identiques octet pour octet au flux du type A du hall (ROM fichier `0x153E8`, trois paires, et leurs miroirs pour `58`-`6C`) dans les 118 instantanés de la salle 2 pris après le chargement (57 de `O1`, 61 de `W1`). L'instantané de l'image du changement de salle (6190) montre encore les chauves-souris | `cmp_vram.py` |
| Couleurs | table de couleur `02` (couche N) et `44` (couche N+4, CC) sur les 16 lignes, pour les 12 motifs (684 entrées de `O1`) | `colors.py` |
| Palette | palette VDP de la salle 2 = `vk_palettes[10]` (`vk_stages[1].first + 2`) à l'identique : 2 = `0222` (corps), 4 = `0C2E` (contour magenta), 6 = `0ACE` (recouvrement), comme les salles 0 et 1 du hall | `pal.py` |

### Port

| Point | Port | Statut |
|---|---|---|
| Spawn | `stage2Type1Tick` : minuterie du hall, Y SAT `80`, côté selon `CF01 & 2` après l'incrément, garde `9E16` ; tir marqué `romBox` | prouvé |
| Mouvement, motifs | ceux des types A et B (même routine `A964`) | prouvé |
| Sol | chute du port (`fall`) : le rang haut se pose en Y 81 (pieds en `A0`), comme au hall (A1) | convention du port |
| Sortie | `romBox` : dernière position tenue 2 images (âges 217-218 à gauche, 209-210 à droite), puis disparition | prouvé |
| Contact | boîte `8290` de la classe 1, Simon debout ; dégât 2 | prouvé debout ; **PROVISOIRE** accroupi et en saut (`C420` non transmis à `enemies.c`) |
| Fouet | `Enemies_whipHitType1Rom` (boîte `84A7`, `C416 = 0`), appelé en phase 2 du fouet du port comme pour le type 5 ; `Enemies_whipHit` ignore les ennemis `romBox` | prouvé pour la boîte ; correspondance phase 2 du port / `C422 = 3` **PROVISOIRE** (comme le type 5) |

### Reste ouvert

- Le délai du premier tir : 48 images mesurées après l'entrée (20 de gel + 28, soit `j = 3` pour `CF00 = 04` : tir à l'appel `j + 12`). Le port garde maintenant `CF10` d'une salle à l'autre (`Enemies_spawnTimerRoom`, remis à 0 seulement en début de partie) : le délai suit la phase de `CF10` comme dans la ROM (déduit du code de `9CC3`, prouvé pour le type 4 ; la phase exacte de la trace `O1` n'est pas reproduite par le scénario `k`).
- L'entrée SAT provisoire (Y `E1`, 2 images) et le retard d'affichage ne sont pas reproduits : le port montre l'ennemi dès l'âge 0.
- Objet `1E`, objet tombé `24` (tirage commun `9A3E`), score : non portés.
- Chute sans sol (`vy += 0x60` par mise à jour, `vx = 0`) : le port garde la chute provisoire du hall (2 px par image). Sans effet dans la salle 2, dont le sol bas est continu pour la sonde à deux points.
- **Hall** (fait le 2026-10-29) : les tirs du hall (`Enemies_spawnTimerTick`) sont marqués `romBox` : sortie `99E5`, contact `8290` et fouet `7DAD` comme au stage 2, à la place de l'ancien modèle (sortie droite à X 216, boîtes de sprites). Trace du hall, salle 0, image par image (`scratchpad/fidelity/out4`, objets `C800`/`C880` et ombre SAT `D638`) : objet en Y `C0` (rang haut SAT `A0`), X SAT `E5` aux deux images qui suivent l'écriture de `C800`, suppression à X objet `06.25` (gauche) et `F1.25` (droite), dernier X SAT `00` (âge 217) ou `E7` (âge 209) dans l'ombre SAT. Le port tient ces positions deux images (âges 217-218 et 209-210), comme au stage 2 : l'ombre `D638` précède la VRAM d'une image (**déduit**, la VRAM du hall n'a pas été relevée image par image). Tests `test_e1_trajectory`, `test_e2_trajectory`, `test_side_left` ; scénario `b_hall_whip_kill` (fouet `84A7`) et `c_enemy_contact` (contact `8290`) inchangés et verts.
- Fouet de niveau `C416 = 1` (portée 24, demi-largeur 21) : non porté.

### Fichiers (hors dépôt, `scratchpad/s2t1/` du worktree)

- Outils : `tool/` (`harness.tcl`, `run.py` à plans concaténés par `+`, `s2t1_extra.tcl`, `render_snap.py`), analyses `ana.py`, `satx.py`, `hits.py`, `cmp_vram.py`, `colors.py`, `pal.py`.
- Plans : `plans/base.txt`, `plans/obs.txt`, `plans/whip.txt`.
- Traces : `out/O1/`, `out/W1/` (`sat_r2.csv`, `objwrites.csv`, `snaps/`, `events.txt`).

## Type 2 au stage 2 : l'homme-poisson des salles 4 et 5 (relevé du 2026-10-28)

**Identification** : l'ennemi de type 2 (`D000 = 2`, `D001 = 4` ou `5`, bit 1 du drapeau de salle, canal `9D4B`) est l'homme-poisson : il saute hors de l'eau depuis Y `C8`, se pose sur un sol, marche vers Simon et replonge quand il tombe sous Y `C0`. Il ne tire jamais : le tir de l'état 2 (`A45A`, `9F6D`) est réservé au type 3 (dans `A3A0`, `+10` n'est décrémenté que pour le type 3). Chaque saut et chaque plongeon crée deux gouttes (objet `20`).

**Porté** dans `sgdk/src/enemies.c` (`type2Channel`, `fishSpawn`, `fishStep`, `splashSpawn`, `type2Frame`, `Enemies_whipHitType2`, dessin dans `Enemies_sprites`), `sgdk/src/enemies.h` (`ENEMY_TYPE_2`, `ENEMY_TYPE_SPLASH`, `ENEMY_MAX` 7, champ `timer`, compteurs `type2Shots`, `type2Frames`, `type2Next`, `splashCount`), `sgdk/src/actors.c` (tuiles `vk_enemy2_tiles` chargées dans le bloc `ENEMY5` en salles 4 et 5 du stage 2, fouet en phase 2) et `tools/build_enemy_sprites_md.py` (tuiles extraites de la ROM). Tests : `test_type2_spawn`, `test_type2_trajectory`, `test_type2_contact_whip`, `test_type2_sat_budget` (`tests/test_enemies.c`) et le scénario MD `l_stage2_room4_fishman`.

### Méthode

- Trace openMSX (outils dans `scratchpad/fish/tool/`, extra `fish_extra.tcl` : SAT, couleurs et les 7 blocs `C800`-`CB00` par image quand `D000 = 2` et `D001` vaut 4 ou 5, écritures d'objets avec PC, instantanés VRAM avec `D100` et `C400`). Plans `mkplan.py` : `P1` = route `S2` jusqu'à la salle 0 du stage 2, `gotox 72`, `C41B = 2` forcé, arrivée en salle 4 (écriture de `D001` à l'image 6115), Simon immobile en (72, 112) ; `P2` = Simon en (236, 112) tourné à gauche (`C42C = 1`), coups de fouet, puis salle 5 par `gotox 250` (écriture de `D001` à 7904), Simon en x 40.
- Analyses : `an.py` (objets par image), `an2.py`, `spawns.py` (créations), `events.py` (contacts, morts), `offsets.py` (SAT contre objet), `layers.py` (couches VRAM contre la ROM), `findrle.py`, `cmpmaps.py` (carte `D100` contre celle du port).
- Lecture de code : `5EC0`, `9CA9`, `9CC3`, `9D4B`, `5F1D`, tables `5FCC`, `6057`, `60E1`, `9942`, `A2E0`, `A310`, `A3A0`, `A4B3`, `A4CF`, `A549`, `9C66`, `9CA3`, `99B9`, `99E5`, `98E5`, `992F`, `8378`/`8380`, `847A`, `84A7`, table `81CE`.

### Code (prouvé par la lecture, confirmé par la trace pour les points marqués dans la table suivante)

| Point | Valeur |
|---|---|
| Canal `9D4B` | `HL = CF02`, périodes `9D8F` = `01 18 18 18 18 18 18 18`, minuterie `9CC3` (compteur décrémenté tous les 4 appels, soit 8 images ; recharge `max(p - 2 × D012, 0) + 1`, `p` lu à `CF03 & 7` avant l'incrément). `E = C8` ; `D` = table `9D87` = `60 D0 30 90 A0 40 60 B0` à `CF03 & 7` après l'incrément ; si `(C427 - D + 18) & FF < 30`, `D += 20` si `D < 80`, sinon `D -= 20` ; `JP 5F1D`. `9CA9`, appelé au chargement de salle (`6324`), remet `CF02 = 04`, `CF03 = 00` |
| Création `5F1D` | premier des 7 blocs `C800`-`CB00` libre, puis N entrées SAT libres parmi 14 (`D638`…), N = table `6057` + type (type 2 : 4, objet `20` : 2) ; sinon la création est perdue (compteur et index ont déjà avancé) |
| Initialisation `A2E0` | pose `A4B3` : `0B` si `C427 >= X`, sinon `08` ; `+18 = 0`, `+1B = 0`, `+6 = 1`, `+19 = 1`, `vy = F480` (-0x0B80) ; points de vie 1 (`60E1`) ; deux objets `20` en (X, Y) |
| État 0 (`A310`) | `+1D += 1` ; pose vers Simon sauf si `+18` ; `vx = 0` ; `vy += 0x80` (plafond `07FF`, `A549`) ; en montée, rien d'autre ; sinon sol (`7B98`) en `(X + 8, Y)` si `+12`, sinon `(X, Y)`, et 10 px à gauche : posé → `Y &= F8` (fraction gardée), `+10 = 30 - 4 × D012`, `+11 = 10`, `+14 = 0`, `+15 = 1`, `+13 = 8`, `vy = 0`, état 1 ; sans sol et `Y >= C0` → suppression (`99F6`) et deux gouttes |
| État 1 (`A3A0`) | `+15 -= 1` ; à 0 : `+15` = table `A4F3` = `10 20 18 30` à `+1D & 3`, `+12 = (C427 >= X)`, `vx = ±0x0180`, `+13 = 1`. `+13 -= 1` ; à 0 : `+13 = 8`, `+14 += 1`, pose `08`/`09` (`A452`) ou `0B`/`0C` (`A456`) selon `+14 & 1`. Sol absent en `(X + (vx >= 0 ? 8 : 0), Y)` et 10 px à gauche → `vy = 0`, état 0, `+18 = 1` (garde sa pose en tombant) |
| Gouttes (`9C66`, `9CA3`) | `+E = 0` (ni contact ni fouet), pose `0E`, `+7E = 0` ; `CF30 += 1` : impair → `vx = +0x0100`, `vy = -0x0500` ; pair → `vx = -0x0100`, `vy = -0x0600`. Mise à jour : gravité `0x80` seule, bornes `99E5` (Y `>= E4`, X `>= F1` ou `< 07`) |
| Contact | classe 1 (`82CB`), `HL = 1805` (`8378`, `847A`) : `|C427 - X| < 10` et `0 <= C425 - 2 - Y + 24 < 50` debout ; dégât table `81CE` = `02`, doublé : **4** |
| Fouet | classe 1 (`8380`, `84A7`) : X du fouet `C427 ± 20` ; `|X fouet - X| < 17` et `0 <= C425 - B - Y + 24 < 28` (`B = 18` debout, `10` accroupi) ; un coup tue (1 point de vie) |

### Trace (prouvé)

| Point | Valeur | Preuve |
|---|---|---|
| Cadence | premier tir 51 images après l'écriture de `D001` (salle 4 : 6115 → 6166 ; salle 5 : 7904 → 7955), puis 16 images, puis 200 images, et de nouveau 16 au 9e tir (index revenu à 0) | `P1`, `P2`, `spawns.py` |
| X | 21 tirs conformes à la table `9D87` et à la garde (salle 5, Simon en 40 : `30` → `50`) ; un tir perdu en 7598 (un homme-poisson et 4 gouttes occupent 12 des 14 entrées SAT) ; au 2e tir de `P1`, une seule goutte (14 entrées prises) | `spawns.py`, `an.py` |
| Trajectoire | Y 200 → 189 dès l'image de création (une mise à jour, `vy` -0x0B00) ; sommet Y 73 ; atterrissage à Y 112.80 (fraction gardée) en 6236 ; marche à 1,5 px par mise à jour, première pose `09`/`0C`, changement toutes les 8 mises à jour ; durée avant le choix de sens suivant = `A4F3[+1D & 3]` ; chute au bord du sol (X 191.80 → 190) en 6260 ; plongeon à Y `>= C0` en 6298 avec deux gouttes. Une mise à jour toutes les 2 images | `an.py` |
| Contact | 8 contacts : `C415` 32 → 28 (-4), dont un sur un homme-poisson en chute | `events.py` |
| Fouet | mort au premier coup debout (Simon 236 tourné à gauche, objet en (208, 106-112), phase `C422 = 3` à 6235 ; objet `1E` en (208, 96)) ; aucun effet en phase 3 sur un homme-poisson à Y 74 | `P2`, `events.py` |
| SAT | rang haut N/N+4 en (X - 8, Y - 32), rang bas M/M+4 en (X - 8, Y - 16), affiché avec une mise à jour de retard (5339 images conformes sur 5395) ; poses `08` : `40`+`48`, `09` : `40`+`50`, `0B` : `68`+`70`, `0C` : `68`+`78` ; couleurs `02` (N) et `44` (N+4) ; goutte : motif `90`, couleur `06`, en (X - 8, Y - 15), retard de 2 images (4004 images sur 4094) | `offsets.py` |
| Gouttes | première mise à jour à la mise à jour suivant la création ; à la mort par le fouet, une goutte bouge sans gravité pendant une image (registre `IX` laissé par `5F1D`) | `an.py` |
| Couches | 81 instantanés VRAM de la salle 4 (table `F800`) identiques ; `68`-`8C` = miroirs de `40`-`64` ; flux RLE de la ROM au fichier `0x1597D` (316 octets, 320 décodés, SHA-256 `0c3312755315d46c000b1173a433dcf2edc37f08a44ecec1ddfa183f65c0a67f`) ; goutte au fichier `0x1423B` (19 octets, 32 décodés, SHA-256 `dca6312ea94fcba589164ca908de05e6badd3b727ef1409f92de1aa0f6725290`) | `layers.py`, `findrle.py` |
| Palette | salles 4 et 5 = `vk_palettes[12]` et `[13]` du port (2 = `0222`, 4 = `0CC8`, 6 = `0CEA`) | instantanés |
| Champs non remis à zéro | `+1D`, `+12`, `+15` gardent la valeur de l'occupant précédent du bloc | `an.py` |

### Port

| Point | Port | Statut |
|---|---|---|
| Cadence, X, garde, perte faute de SAT | `type2Channel` (minuterie `9CC3` : `CF10` partagé, compte à rebours `type2Count` = `CF02`, index `type2Shots` = `CF03`), `newObject` (7 blocs, 14 entrées SAT) | prouvé |
| Premier tir | `9D4B` appelle `9CC3` avec `HL = CF02` (désassemblage `9D4B`-`9D5B`), comme le canal `9D97` : `CF02 = 4` à l'entrée (`ENEMY_TYPE2_COUNT`), décrémenté quand `CF10 & 3` revient à 0, donc premier tir 25, 27, 29 ou 31 images de jeu après la fin de l'état 9 selon `CF10 & 3` (3 → 29, 2 → 27, 1 → 25, 0 → 31) | lu dans le code ; 31 mesuré 2 fois (51 après `D001`, `CF10` non relevé dans les traces `P1`/`P2`) ; test natif des quatre phases (port 2026-10-29) |
| Saut, sol, marche, chute, plongeon | `fishStep`, tests de sol `7B98` sur la carte de collision du port | prouvé (test natif : atterrissage 121, chute 145, plongeon 183 images après l'entrée, comme en 6236, 6260 et 6298) |
| Gouttes | `splashSpawn`, mise à jour dans `type2Frame` ; ni contact ni fouet | prouvé ; la goutte sans gravité de la mort au fouet n'est pas reproduite |
| Contact | `touches` : boîte `8378` de la classe 1, Simon debout ; dégât 4 | prouvé debout ; **PROVISOIRE** accroupi et en saut (`C420` non transmis) |
| Fouet | `Enemies_whipHitType2` (boîte `84A7`, `C416 = 0`), appelé en phase 2 du fouet du port | prouvé pour la boîte ; correspondance de phase **PROVISOIRE** (comme les types 1 et 5) |
| Dessin | 4 sprites (bloc `ENEMY5`, 21 couches `40`-`90`, 84 tuiles) en (X - 8, Y - 31) et (X - 8, Y - 15), goutte en (X - 8, Y - 14) | couches et positions prouvées ; retard d'affichage non reproduit |
| Mémoire | `ENEMY_MAX` passe de 4 à 7 (les 7 blocs de la ROM) ; `EnemyState` passe à 144 octets (énergie à l'offset 126 : scénario `d` mis à jour) | — |

### Écarts et reste ouvert

- **Carte du port** : `D100` est égale à la carte du port à l'arrivée, mais 16 images plus tard un bloc solide de 32 × 32 apparaît (salle 4 : x 32-63, y 80-111 ; salle 5 : x 192-223, y 112-143), absent de `vk_stage2_collision`. En ROM, l'homme-poisson s'y pose (salle 4, X 48 → Y 80 ; salle 5, X 208 → Y 112) ; dans le port, l'homme-poisson de X 48 passe au travers et se pose en Y 112. Les lignes 16 et 24 cachées sont aussi remplies de zéros dans le port. **Résolu** : ce bloc est le bloc caché (mode 3) de la bougie (32, 80) en salle 4 et (192, 112) en salle 5, écrit dans la carte par `86CA` ; il est maintenant porté (`research/castle-candles.md` §9.6) et le second homme-poisson se pose en (48, 80) dans le scénario `l`. Les « 16 images » observées correspondaient à la fin de l'état 9 (transition de salle) : la ROM écrit le bloc au chargement de salle `62F5`, juste avant la première image de jeu, comme le port (détail : `research/castle-candles.md`, « Moment de la pose »).
- `+1D`, `+12`, `+15` repris à 0 dans le port (la ROM garde ceux de l'occupant précédent) : le premier choix de sens peut différer pour un bloc réutilisé. **PROVISOIRE**.
- Objet `1E` (flamme de mort), score, sons 7, 8, 9 : non portés.
- Type 3 (même routine, deux points de vie, tir `A45A`) : absent des salles relevées, non porté.
