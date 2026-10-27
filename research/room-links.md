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
6. Déclencheur exact de la porte de cour (x≥232 au passage, `C000=10`) : mesuré, mais pas le seuil exact ni la raison de l'état 10.
7. Stages 2 à 18 : table décodée, réciprocité non garantie (22 cas), aucun lien vérifié en jeu.
8. Lecture des banques (`romblocks`) : la sortie des banques dans `writes.csv` n'était pas fiable pendant ce travail. Les PC et les valeurs de `D000`/`D001` sont fiables.

## 8. Reproduction

- Décodage table : `python3 tools/decode_room_links.py "Vampire Killer (Japan, Europe) (En).zip"` (lit la ROM, aucune écriture).
- Trace : openMSX `C-BIOS_MSX2` avec la ROM en cartouche ; watchpoints `write_mem` sur `D000`/`D001` (PC via `CPU regs` 20/21) ; contrôleur par point d'arrêt `0x4028` (interruption VBLANK), titre passé par SPACE à partir de la frame 120. Les scripts de trace sont restés dans `scratchpad/room-links/`.
