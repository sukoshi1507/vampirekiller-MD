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
| Mouvement | 1,5 px par image (X E5 vers 01 en 152 images) | prouvé |
| Animation | aucune pose changée sur 150 images (N fixe 48 ou 60) | prouvé (une piste par côté) |
| Sortie | disparaît à X 01 ou E6 | prouvé |
| Contact | -2 sur `C415` à 2797 et 3133, Simon en X 120 et Y 192, l'ennemi à X ≈ 122 | prouvé |

### 3.4 Type 5 (salles 2, 3, 7 ; drapeau `00`)

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
