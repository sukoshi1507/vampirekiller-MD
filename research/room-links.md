# Liens entre salles : cour (stage 0) et hall (stage 1)

Statut : **liens L/R du stage 0 et du hall prouvés par trace openMSX ; mécanisme de table décodé statiquement ; liens haut/bas et salles 4 à 7 du hall : table seulement, non vérifiés en jeu.**

ROM de référence : `Vampire Killer (Japan, Europe) (En).zip` (racine), SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`, vérifié. Adresses CPU Z80 sauf mention « offset ROM » (offset fichier, banque n = offset // 0x2000, fenêtre A000 pour la banque 13).

Traces : openMSX 21.0, `C-BIOS_MSX2`, ROM en cartouche. Pilotage par clavier (ligne 8 : RIGHT 128, LEFT 16, UP 32, DOWN 64, SPACE 1), titre passé par SPACE, gameplay atteint à la frame 739 (`C000=5`, `D000=0`, `D001=0`, `x=16`, `y=176`). Les scripts de trace sont dans `scratchpad/room-links/` (non versionné) ; le script committé est `tools/decode_room_links.py`.

## 1. Variables et points d'écriture

| Variable | Rôle | Preuve |
|---|---|---|
| `D000` | stage (0 = cour, 1 = hall, ...) | écritures observées, voir ci-dessous |
| `D001` | salle dans le stage | idem |
| `C427` | X de Simon | `research/loading-and-states.asm`, `enemies-platforms.md` |
| `C425` | Y de Simon | idem |
| `C000` | état de jeu (5 = jeu ; 9 et 10 observés pendant les transitions) | trace |

Écritures de `D000`/`D001` observées dans la trace (fichier `writes.csv` de la première marche) :

| Frame | PC | Écriture | Contexte |
|---|---|---|---|
| 323 | `44D2` | D000=0, D001=0 | initialisation de partie |
| 971 | `B987` (ROM off. `1B987`, banque 13, fenêtre A000) | D001=1 | cour 0 → cour 1, sortie DROITE |
| 1231 | `B987` | D001=2 | cour 1 → cour 2, sortie DROITE |
| 1525 | `435A` (ROM off. `4357`, banque 0 fixe) | D000 += 1 (0 → 1) | cour 2 → hall, sortie DROITE à x=232 |
| 1525 | `435D` | D001=0 | idem, salle 0 du hall |
| 1574 | `B987` | D001=3 | hall 0 → hall 3, sortie GAUCHE |
| 1836 | `B987` | D001=2 | hall 3 → hall 2, sortie GAUCHE |

`B987` est donc le point d'écriture de la salle pour toutes les sorties table. `4357` est le passage de stage (château).

## 2. Mécanisme de la table de liens (ROM)

Décodé statiquement par `tools/decode_room_links.py` (lecture seule de la ROM, aucune émulation) :

1. `P[s]` : pointeur 16 bits à `B9D3 + 2*s` (banque 13, fenêtre A000), `s` = `D000`. Pointeurs : `B9F9` (cour), `B9FF` (hall), `BA0F`, ..., `BB1D` (stage 18). Les 19 pointeurs couvrent les stages 0 à 18.
2. Code `B9BD` : `HL = P[D000]`, puis `HL += 2*D001`. Le lien de la salle est donc à `P[stage] + 2*room`.
3. Chaque salle occupe **2 octets** : `b0 = (HAUT << 4) | BAS`, `b1 = (GAUCHE << 4) | DROITE`. Nibble `F` = aucun lien.
4. La routine `B963` (qui contient l'écriture `B987`) lit un nibble selon un index `C41B` (et `B99A` selon `C41C`), puis écrit la salle cible dans `D001` (`LD (D001),A` en `B987`). Un nibble `F` fait `RET Z` : pas de sortie.

Vérification de cohérence : les 2 octets × salles × stage couvrent exactement les effectifs de `research/castle-navigation/NAVIGATION_AUDIT.md` (3 + 8 + 6 + ... + 10 = 156 = 153 salles + 3 de la cour). Les pointeurs sont croissants et contigus.

Point non établi : l'ordre exact des nibbles HAUT/BAS/GAUCHE/DROITE n'est pas démontré par le code seul. Il est déduit de la trace (GAUCHE/DROITE concordent avec toutes les sorties testées ci-dessous) et de la réciprocité des liens HAUT/BAS du stage 1 (`1 HAUT → 5` et `5 BAS → 1`, `3 HAUT → 7` et `7 BAS → 3`).

## 3. Liens prouvés et décodés

Colonnes : **Trace** = sortie observée en jeu (run, frame) ; **Table** = nibble décodé ; **Entrée** = position de Simon à l'arrivée (x, y).

### 3.1 Cour (stage 0)

| Salle | Sortie | Table | Trace | Entrée | Preuve |
|---|---|---|---|---|---|
| 0 | GAUCHE | aucun | **bloqué** à x=14 | - | explorateur, run 1 |
| 0 | DROITE | 1 | **→ cour 1** | x=10, y=176 | run 2 (frame 972, `B987` D001=1) |
| 1 | GAUCHE | 0 | **→ cour 0** | x=246, y=176 | run 3 |
| 1 | DROITE | 2 | **→ cour 2** | x=10, y=176 | run 4 (frame 1232, `B987` D001=2) |
| 2 | GAUCHE | 1 | **→ cour 1** | x=246, y=176 | run 5 |
| 2 | DROITE | aucun dans la table | **→ hall 0** (stage 1, salle 0) à x=232 | x=232, y=176 | run 6 (frame 1525, `435A`/`435D`) |
| 2 | HAUT à x=192, 208 | aucun | rien (saut seul) | - | sweep `runs/sweep-02` |

Le passage cour 2 → hall est déclenché par la **droite à x=232**. HAUT seul à x=192..208 ne change pas de salle.

Précision (`research/castle-entrance.md`) : x = 232 est la sortie de la **porte du château**. Simon y arrive par une marche automatique qui commence à x = 216 (zone de `8580`, `C5AC` 4 → 5) : 1 px toutes les 4 images, touches ignorées, derrière le montant de la porte ; puis 36 images d'écran noir avant le hall.

### 3.2 Hall (stage 1)

| Salle | GAUCHE (table / trace) | DROITE (table / trace) | HAUT (table) | BAS (table) |
|---|---|---|---|---|
| 0 | 3 / **→ 3** (x=246, y=192) | 1 / **→ 1** (x=10, y=192) | - | - |
| 1 | 0 / **→ 0** (x=246, y=192) | 2 / **→ 2** (x=10, y=192) | 5 / non vérifié | - |
| 2 | 1 / **→ 1** (x=246, y=192) | 3 / **→ 3** (x=10, y=192) par saut HAUT+DROITE depuis x=166 ; marche DROITE bloquée à x=88 depuis la gauche (run 12, probe `jump12a`) | - | - |
| 3 | 2 / **→ 2** (x=246, y=192) | 0 / **→ 0** (x=10, y=192) | 7 / non vérifié | - |
| 4 | aucun | 5 / non atteint | - | - |
| 5 | 4 / non atteint | 6 / non atteint | - | 1 / non vérifié |
| 6 | 5 / non atteint | 7 / non atteint | - | - |
| 7 | 6 / non atteint | aucun | - | 3 / non vérifié |

Remarques :
- Toutes les sorties L/R tracées du hall (0 à 3) concordent avec la table.
- La salle 2 contient des murs : marche DROITE bloquée à x=88 (depuis la gauche, run 12) et marche GAUCHE bloquée à x=166 (depuis la droite, probe `jump12a`). Le mur x=166 se franchit par saut HAUT+DROITE, qui mène à la salle 3. Le passage du mur x=88 par saut n'a pas été testé.
- Les salles 4 à 7 n'ont pas été atteintes en jeu : aucun passage vertical n'a été trouvé.

### 3.3 Entrée de salle

- Arrivée par la **gauche** (sortie DROITE de la salle précédente) : x=10.
- Arrivée par la **droite** (sortie GAUCHE) : x=246.
- Le **y** est conservé d'une salle à l'autre (176 ou 192 selon le sol), sur les 13 transitions L/R observées.
- Déclenchement mesuré : marche DROITE, Simon atteint x=246 puis le pas suivant (vers 248) déclenche la transition, qui recharge la salle avec x=10. Marche GAUCHE : le pas suivant après x=8 déclenche la transition, qui recharge avec x=246. Le seuil gauche est vérifié à ±2 px près.

## 4. Escalier et sorties HAUT/BAS

- Le hall 0 et le hall 1 ont un comportement de **pente** autour de x≈28–32 : BAS (hall 0) ou HAUT puis BAS (hall 1) déclenche une marche automatique diagonale vers la gauche. Simon traverse ensuite la limite de salle (hall 1 → hall 0 à x=246, hall 0 → hall 3 à x=246, y=176). Ce comportement ne correspond à aucun lien vertical de la table.
  - **Correction** (rapport des escaliers, trace r1) : cette « pente » n'est pas un escalier. C'est un recul d'ennemi (C420 00→05, C423 00→01, C42D 00→59, C415 20→1E), suivi d'un mouvement de saut. Les vrais escaliers sont des tuiles de la carte (tuiles 12 et 13) : voir `research/stairs.md`.
- Aucun passage HAUT/BAS n'a été trouvé en balayage à 16 px (salles 0, 1, 3 du hall, x de 16 à 240 ; salle 2 seulement de 12 à 84 et de 176 à 240) et à 4 px (salle 3, x de 12 à 236 ; salle 0, même plage). Les liens 1↔5 et 3↔7 de la table ne sont donc **pas** vérifiés.
- Les escaliers et leur position exacte restent **inconnus**.

## 5. Comparaison avec le prototype

Référence prototype : `sgdk/src/gameplay.c` (`vk_courtyard_neighbors`, `vk_game_tick_controls`) et `sgdk/src/room_manager.c` (`vk_stage1_links`, `vk_room_try_edge`).

| Élément | Prototype | Authentique | Statut |
|---|---|---|---|
| Cour, liens L/R (`vk_courtyard_neighbors`) | 0:{-,1}, 1:{0,2}, 2:{1,-} | identique (table + trace) | **authentique** |
| Cour, sortie vers le hall | HAUT à x 192..216 sur la salle 2 | DROITE à x=232, sans HAUT | **non authentique** |
| Entrée du hall | salle 0, x=232 | salle 0, x=232 | authentique (trace) |
| Hall, salle 0 : gauche | aucun | salle 3 | **non authentique** |
| Hall, salle 0 : droite | 1 | 1 | authentique |
| Hall, salle 1 : gauche / droite | 0 / 2 | 0 / 2 | authentique |
| Hall, salle 2 : gauche / droite | 1 / 3 | 1 / 3 (par saut) | authentique |
| Hall, salle 3 : gauche / droite | 2 / 4 | 2 / **0** | **droite non authentique** |
| Hall, salle 4 : gauche / droite | 3 / 5 | **aucun** / 5 | **gauche non authentique** |
| Hall, salles 5, 6, 7 : gauche / droite | 4/6, 5/7, 6/- | 4/6, 5/7, 6/- | authentique (table) |
| Hall : liens HAUT/BAS (1↔5, 3↔7) | absents | présents (table) | **manquants** (non vérifiés en jeu) |
| Entrée de salle : x | 10 (droite), 246 (gauche) | 10 et 246 | authentique (trace) |
| Seuil de bord | x ≥ 248 (droite), x ≤ 8 (gauche) | x ≥ 248 : oui ; gauche : pas suivant après 8 | droite authentique ; gauche à vérifier à 2 px près |

Conclusion : le graphe de test `vk_stage1_links` est une chaîne 0-1-2-...-7 et ne correspond pas à la topologie d'origine. Trois liens L/R sont faux (salle 0 gauche, salle 3 droite, salle 4 gauche) et quatre liens verticaux manquent. La séquence `vk_room_try_edge` (entrée 10/246) est authentique.

## 6. Table des stages 2 à 18 (décodage seul)

`tools/decode_room_links.py` décode les 156 salles. Sur ces stages, **22 liens ne sont pas réciproques** (par exemple `s3 r4 DROITE→5` alors que `r5 GAUCHE` est vide, `s13 r11 GAUCHE→9` alors que `r9 DROITE→10`). Ces cas peuvent être des sorties à sens unique (escaliers) ou une erreur de lecture pour certains stages. Ils ne sont pas vérifiés et ne doivent pas être portés comme liens sûrs.

## 7. Inconnues

1. Ordre exact des nibbles (HAUT/BAS/GAUCHE/DROITE) : déduit de la trace horizontale et de la réciprocité verticale, non tracé.
2. Liens verticaux du hall (1↔5, 3↔7) et leurs escaliers : non trouvés en jeu.
3. Salles 4 à 7 du hall : non atteintes en jeu.
4. Caller de `B963` et rôle de `C41B` / `C41C` (index probable : direction ou sous-sortie) : non établi.
5. Mécanisme des pentes (marche automatique) près de x≈28–32 dans le hall : non établi.
6. Déclencheur de la porte de cour : **résolu**, voir `research/castle-entrance.md` (zone x 216..223 regard à droite, marche automatique jusqu'à x 232, état 10 = passage de stage `4387`).
7. Stages 2 à 18 : table décodée, réciprocité non garantie (22 cas), aucun lien vérifié en jeu.
8. Lecture des banques (`romblocks`) : la sortie des banques dans `writes.csv` n'était pas fiable pendant ce travail. Les PC et les valeurs de `D000`/`D001` sont fiables.

## 8. Reproduction

- Décodage table : `python3 tools/decode_room_links.py "Vampire Killer (Japan, Europe) (En).zip"` (lit la ROM, aucune écriture).
- Trace : openMSX `C-BIOS_MSX2` avec la ROM en cartouche ; watchpoints `write_mem` sur `D000`/`D001` (PC via `CPU regs` 20/21) ; contrôleur par point d'arrêt `0x4028` (interruption VBLANK), titre passé par SPACE à partir de la frame 120. Les scripts de trace sont restés dans `scratchpad/room-links/`.

## 9. État 9 : le gel du changement de salle

Relevé du 2026-10-28. Traces openMSX `T1`, `T2`, `T3` (`scratchpad/transition/` du worktree, non versionné : harnais `harness.tcl` copié de `scratchpad/fish/tool/`, extra `transition_extra.tcl`, plans `mkplan.py` et `mkplan2.py`, mesure `durations.py`). Les changements de salle sont forcés par `C41B` (1 haut, 2 bas, 3 gauche, 4 droite) une fois Simon dans la salle ; `T1` suit la route `S2` jusqu'au stage 2 puis parcourt les salles 0 à 5, `T2` et `T3` la cour et les salles 0 à 7 du hall. Par image (point d'arrêt `4028`) : `C000`, `C001`, `C003`, `C005`, registres VDP 1 et 7, palette, SAT ; points d'arrêt `4257` (état 5), `426A` (test de `C41B`), `4377` (état 9), `437F`/`4382` (appel et retour de `62F5`), `47D4` (écran éteint), `47C7` (écran rallumé) ; écritures de `C000` avec le PC.

### 9.1 Code (prouvé)

| PC | Rôle |
|---|---|
| `414D` | répartiteur : `C003 += 1`, saut par la table `4161` selon `C000` (état 5 = `4257`, état 9 = `4377`) |
| `4257`-`4270` | état 5 : `CALL 5C25` (une passe de jeu, dont `7715` qui écrit `C41B = 4`), puis `C41B ≠ 0` → `41B6` : `C000 := 9`, `C004 := 20`, `C001 := 0`, **dans la même passe** |
| `4377` | état 9 : `CALL 5A2E` (`B963` : `C41B := 0`, `D001` écrit en `B987`, retenue dès le premier appel), `CALL 62F5`, `C000 := 5` |
| `62F5` | `63C5` (SAT : Y = `E0`), `6382`, `5780`, `56E1`, `4F83`, `47D4` (R1 bit 6 à 0), `6333`, `4F91`, `91BE`, `8671` (bougies, blocs cachés), `9147`, `8EE6`, `47C7` (R1 bit 6 à 1), `9CA9` (minuteries `CF00`-`CF0E`), `6154` + `JP 61BB` (objets fixes, dont la panthère) |
| `404B`-`4053` | garde `C005` : tant que `62F5` n'a pas rendu la main, les interruptions suivantes ne jouent que la musique (`8964`) |

**Pas d'image de jeu entre la requête et l'état 9** (prouvé, code et trace) : le test de `C41B` suit `CALL 5C25` dans la passe qui l'a posé (`T3` : `426A` et l'écriture `C000 = 9` dans la même interruption, par exemple 1850). Une passe de l'état 5 couvre deux interruptions (`C003` augmente une image sur deux) : c'est pourquoi `frames.csv` montre encore `C000 = 5` à l'image où `x` vaut déjà 10 (trace `fish/out/P2`, image 970).

### 9.2 Durées (prouvé, traces)

`N` = images échantillonnées avec `C000 = 9` (première = écriture de `D001`) ; `noir` = images entre ce début et l'extinction (`47D4`). Elles dépendent de la salle d'arrivée seulement (ni de la salle de départ, ni du sens).

| Arrivée | N | noir | Cas |
|---|---|---|---|
| cour 0, 1, 2 | 18 | 1 | 7 (`T2`), plus `P2` |
| hall 0 | 19 | 2 | 3 |
| hall 1 | 20 | 2 | 3 |
| hall 2 | 22 | 4 | 2 |
| hall 3 | 22 | 4 | 3 |
| hall 4 | 21 | 3 | 2 |
| hall 5 | 19 (20 une fois) | 2 | 4 |
| hall 6 | 19 | 2 | 2 |
| hall 7 | 23 | 4 | 3 |
| stage 2 salle 0 | 23 | 3 | 3 |
| stage 2 salle 1 | 24 | 3 | 2 |
| stage 2 salle 2 | 20 | 2 | 4 |
| stage 2 salle 3 | 21 | 3 | 4 |
| stage 2 salles 4 et 5 | 21 | 3 | 1 + 1 (`T1`), plus `P1`/`P2` |

Le rallumage (`47C7`) tombe dans la dernière image `C000 = 9` (parfois l'avant-dernière). Couleur de fond R7 = 0 : **écran noir** pendant l'extinction (prouvé). La palette de la nouvelle salle est chargée avant l'extinction (`T3`, image de l'extinction), donc la dernière image allumée montre l'ancienne salle dans la nouvelle palette (déduit, non reproduit).

### 9.3 Ce qui est affiché

1. Image de la requête : dernière passe de jeu.
2. `noir` images : ancienne salle **sans sprites** (SAT vidée par `63C5`, prouvé par le code ; l'échantillon de la SAT à `4028` n'est pas fiable à cause du multiplexage).
3. Écran éteint (noir) jusqu'à la fin de `62F5`.
4. Nouvelle salle, encore sans sprites ; les sprites reviennent avec la première passe de l'état 5 (Simon rebouge 2 images après la fin de l'état 9, `P2` : 989 → 990).

### 9.4 Minuteries des ennemis (prouvé par le code)

Aucune logique d'ennemi ne tourne pendant `62F5` ; `9CA9` réinitialise les minuteries à la fin de `62F5` ; `9CC3` n'est appelé que par l'état 5. Les horloges partent donc **de la première passe de l'état 5**, sauf les objets fixes créés par `61BB` à la fin de l'état 9.

| Type | Horloge ROM | Port |
|---|---|---|
| 1 (zombie, hall et stage 2 salle 2) | état 5 (`CF00 = 04`) ; hall : 31 images après l'état 5 (image 877 → 908) | inchangé : la minuterie du port démarre à la fin du gel ; stage 2 salle 2 : 52 images après `D001` contre 48 mesurées (au lieu de 32), toujours **PROVISOIRE** (phase de `CF10`) |
| 2 (homme-poisson) | état 5 (`CF02 = 04`) ; 51 images après `D001` = 21 d'état 9 + 30 | `ENEMY_TYPE2_FIRST` 51 → **31** (image de jeu 31 = `D001` + 51) ; tests natifs décalés de 20 |
| 4 (chauve-souris) | état 5 (`CF06 = 08`) ; premier tir 56-57 images après l'état 5 (hall salle 4 : 2638 → 2694 ; stage 2 salle 0 : 6009 → 6065) | canal `9D97` porté (`type4Channel`) : `CF06 = 8`, `CF10` partagé et gardé d'une salle à l'autre ; tir 56 à 62 images après la première image de jeu selon la phase de `CF10` (`research/enemy-behavior.md` §3.3) |
| 5 (panthère) | créée par `61BB` à la fin de `62F5` (dernière image `C000 = 9`) | `ENEMY_TYPE5_DELAY` 21 → **1** (première image de jeu) |

### 9.5 Port

- `sgdk/src/room_freeze.c` (sans SGDK, test natif `tests/test_room_freeze.c`) : table 9.2 et séquence par image. **PROVISOIRE** : salles hors table (stages 3 et suivants) à 20 images, noir à 2.
- `sgdk/src/main.c` : bords gauche/droite du hall et du stage 2 (`vk_room_try_edge`), cour (`VK_EVENT_ROOM`) et liens verticaux (`VK_EVENT_VERTICAL`) lancent le gel. Pendant N images comptées en VBlank (`vtimer`) : aucune entrée (START compris), ni Simon, ni fouet, ni ennemis, ni bougies, ni objets lâchés, ni flammes ; seule la musique continue. Sprites cachés dès la requête, palettes noires à l'image `noir` (comme l'entrée du château), nouvelle salle dessinée 6 images avant la fin (le dessin du port prend environ 6 images de 68000), sprites rendus à la première image de jeu. Scénario `tests/md_scenarios/n_room_transition_freeze.txt`.
- Non concernés : l'entrée du château (marche + 36 images noires), le passage de stage (porte de la salle 7), le changement de salle debug (A + gauche/droite), la réapparition.
- Écarts : la nouvelle salle apparaît, sans sprites, environ 4 images avant la fin du gel (au lieu de la dernière image) ; l'icône de clé du HUD (sprite dans le port) disparaît avec les sprites ; la dernière image de l'ancienne salle n'a pas la nouvelle palette.
