# Porte de la salle 7 du hall (stage 1) : ce qui joue le rôle de porte

Statut : **sortie de stage prouvée par trace (test forcé) ; accès naturel à la zone de sortie non trouvé.**

ROM de référence : `Vampire Killer (Japan, Europe) (En).zip` (racine), SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`, vérifié sur l'extraction `scratchpad/door7/game.rom`. Adresses CPU Z80. Fenêtre `4000-5FFF` = banque 0 (fichier `CPU - 4000`). Les sorties de salle sont décrites dans `research/room-links.md` ; l'escalier de la salle 7 dans `research/down-stairs.md`.

Mesures : openMSX 21.0, `C-BIOS_MSX2`, ROM en cartouche, pilotage clavier (ligne 8). Scripts : `tools/door_room7_trace.tcl` (ajouté après `tools/vertical_links_trace.tcl`), `tools/run_door_room7_trace.py`, `tools/door_room7_scan.py` (recherche statique). Journaux : `scratchpad/door7/runs/` (non versionné).

## 1. Réponse courte

- **Ce qui fait office de porte** : une zone de déclenchement de sortie de stage, dans l'ouverture noire du coin haut-droit de la salle 7. L'ouverture est faite de tuiles `0x00` : colonnes 28 à 31 (x 224 à 255), rangées 4 à 9 (y 48 à 95). Elle est au-dessus du rebord de briques (rangée 10, y 96).
- **Donnée de la porte** : `C5AE = 0xEC` (236), écrit au chargement de la salle 7 (`BB52`). La zone est `X` dans `[C5AE+8, C5AE+16)` = **x 244 à 251**.
- **Condition** : `C5AC = 5` (mémoire d'état de la porte) et `X` dans la zone, puis `C408 := 1` en `7802`, ce qui envoie en état 10 (sortie de stage).
- **Destination mesurée** (test forcé, voir §4) : stage 2 (`D000` 1 → 2), salle 0 (`D001 = 0`), Simon posé à `x = 16`.
- **Accès naturel** : **non trouvé**. Dans nos essais, `C5AC` reste à 1 dans la salle 7 et Simon ne dépasse pas x ≈ 228 en marchant.

## 2. Preuves de la structure (salle 7, stage 1)

| Fait | Preuve | Confiance |
|---|---|---|
| Liens de la salle 7 : HAUT = aucun, BAS = 3, GAUCHE = 6, DROITE = aucun | écritures `B9B3` à l'entrée (`C41C=FF`, `C41D=03`, `C41E=06`, `C41F=FF`), `writes.csv` run `t0` | prouvé |
| Entrée en salle 7 : `D001 := 7` en `B987` | trace, run `t0` frame 2429 | prouvé |
| À l'entrée : `C5AC := 0` (`638E`), `C5AE := 0`, puis `C5AE := 236` (`BB52`), `C5AC := 255` (`BB5D`) | `writes.csv` frame 2429 | prouvé |
| `C5AC` passe de 255 à 1 à la remise en état 5 (écrivain `53FC`) | `writes.csv` frame 2450 | prouvé |
| Les salles 0, 1 et 3 du hall ont `C5AE = 0` (aucune porte) ; la cour salle 2 a `C5AE = 224` | `writes.csv` runs `t0`, `p4` | prouvé (salles 0, 1, 3 et cour 2) |
| Carte vivante `D100..D2FF` = table de collision `vk_stage1_collision[7]` : identique aux rangées 2-17 et 22-23 et dans l'ouverture (colonnes 28-31). Écarts : rangées 0-1 (HUD, table à zéro) et rangées 18-21, colonnes 24-27 (hors porte) | dump `room_entry_2452.bin` vs `sgdk/src/generated/stage1_collision.c` | prouvé |
| Ouverture noire : tuiles `0x00` colonnes 28-31, rangées 4-9 ; rangée 2 solide (`01 02`) au-dessus, rangée 10 solide (`01 02`) en dessous | dump `room_entry_2452.bin` (identique à `room_f3000.bin`) | prouvé |
| Image de la salle 7 : ouverture noire en haut à droite, sous un mur de briques | `scratchpad/stairs/renders/hall-s1-r7.png` (rendu existant) | prouvé (visuel) |

Le code de sortie (`ROM`) :

- État 5 (`4257`) appelle la routine `7718` (bloc `7718-7805`, banque 1 fenêtre `6000`) teste le bord et la porte. Appel observé à `6B0C` (`CD 18 77`, octet fichier `0x2B0C`).
- `77B3` : `LD A,(C5AC) ; CP 5 ; JR Z,77C4`. Si `C5AC = 5`, la porte se teste par la zone `[C5AE+8, C5AE+16)` (`77C4-77CF`). Sinon, c'est le test de bord `X-8 >= F0` (x ≥ 248 ou ≤ 7).
- `77D1` : si `X` est à droite (bit 7) et `C41F = FF`, ou à gauche et `C41E = FF`, il y a sortie : `7800 : C408 := 1`, `RET`.
- `7802` : `LD (C408),A` (A = 1). Écrivain observé : `PC 7802` (run `p6` frame 2950).
- `4387` (état 10) puis `4347` : `C410`/`C411` BCD +1, `D000 += 1` (`435A`), `D001 := 0` (`435D`), `C408 := 0`, `C000 := 4` (`41B6`). Trace confirmée (`p6` frames 2951-2952).

## 3. Preuve de la zone de déclenchement (cour, référence)

Le même mécanisme explique la sortie cour 2 → hall, déjà mesurée :

- Entrée en cour salle 2 : `C5AE := 224` (`BB52`), `C5AC := 4`.
- `C5AC := 5` à `7755` (run `t0` frame 1451, x = 216).
- `C408 := 1` à `7802` (frame 1515), x = 232.

Zone `[224+8, 224+16)` = `[232, 240)` : le déclencheur à x = 232 correspond exactement. Le passage dans le hall est donc une porte **sans tuile**, seulement une zone de coordonnées.

## 4. Test forcé dans la salle 7 (destination mesurée)

Run `p6` (scripts `scratchpad/door7/plans/p6.txt`) : titre, puis arrivée en salle 7 (`1/7`), puis `poke C5AC = 5`, `poke X = 250`, `poke Y = 40`.

| Image | PC | Événement |
|---|---|---|
| 2948 | `4028` (poke) | `C5AC = 5` |
| 2949-2950 | (poke) | Simon en `(250, 40)` |
| 2950 | `7802` | `C408 := 1` |
| 2951 | `41B6` | `C000 := 10` |
| 2952 | `435A`, `435D`, `4361`, `41B6` | `D000 : 1 → 2`, `D001 := 0`, `C408 := 0`, `C000 := 4` |
| 2971 | `B9B3` | liens de la salle 0 du stage 2 chargés : `C41C = 02`, `C41D = 04`, `C41E = FF`, `C41F = 01`. `C5AC := 0`, `C5AE := 0` (`638E`) |
| 2970 | (état 4) | Simon posé en `x = 16`, `y = 96` |
| 2993 | `4251` | `C000 := 5` (retour en jeu) |

Destination mesurée : **stage 2, salle 0, `x = 16`**. Le `y = 96` est la hauteur du sol à `x = 16` de cette salle (Simon posé, pas un choix de la porte). La sortie courtyard → hall donnait de même `x = 16` au retour en jeu (frame 1555, `d = 1/0`).

Le test `p5` (même position, `C5AC = 1`, `X = 250`, `Y = 40`, puis `hold R`) ne sort pas : `C408` reste à 0 pendant 120 images. Ce test ne prouve pas que le bord x ≥ 248 est inactif : la routine `7718` a une porte de validation (`CALL 8580`, `RET NC`) qui n'a pas été étudiée.

## 5. Ce qui n'est pas réussi : accès naturel

| Essai | Résultat | Run |
|---|---|---|
| Marche DROITE depuis x = 10, salle 7, y = 192 | arrêt à x = 185 (`STUCK`) | `t0` |
| `poke X = 240` à Y = 176, 144, 120, 96 puis DROITE | arrêt à x = 240 (y = 96, 128) ou à 240 dans la zone | `p1` |
| `poke X = 232` à Y = 40 : chute jusqu'à Y = 96 (la colonne 29 est vide en vertical) | Simon posé à `(232, 96)` | `p2` |
| `poke X = 200` à Y = 96 puis DROITE, puis saut DROITE+HAUT | arrêt à x = 228 (`STUCK`), pas de progression | `p3`, `p3b` |
| `poke X = 250` à Y = 40 : chute à `(250, 96)`, DROITE | pas de sortie (`C5AC = 1`) | `p5` |

Observations :

- En marchant, `C5AC` reste à 1 (de x = 56 à x = 228). Le `INC C5AC` (`7752`) ne s'exécute pas. En cour, `C5AC` passait de 4 à 5 à x = 216 par ce `INC`. En salle 7 du hall, la routine a une porte de contrôle supplémentaire : `7737` lit `C701` bit 0 quand `D000 != 0` et retourne (`RET NC`) si le bit est à 0. Le bit n'a pas été mesuré.
- La marche DROITE est bloquée à x ≈ 228 à hauteur de sol (y = 96). La colonne de tuiles à droite (colonne 29) est `01 02` (rangée 10) ou `00` (rangées 4-9). Le blocage latéral par la tuile `0x00` n'est **pas prouvé** ; il est déduit de l'arrêt constant à x = 228.

## 6. Inconnues

1. **Ce qui fait passer `C5AC` de 1 à 5 dans la salle 7** (`INC` de `7752`, gardé par `C701` bit 0 et par `8580`). Non trouvé.
2. **Comment atteindre x 244-251** sans poke. La zone est dans l'ouverture noire ; l'entrée latérale est bloquée à x ≈ 228.
3. Rôle exact de `8580` (validation avant le test de porte) et de `C701` bit 0 (qui l'écrit ?).
4. Le test de bord `X >= 248` (sans `C5AC = 5`) est-il actif dans la salle 7 ? Le test `p5` ne le prouve pas (`8580` peut bloquer).
5. Le `y` d'arrivée au stage 2 (96 mesuré pour la salle 0, `x = 16`) dépend-il de la salle ? Une seule mesure.

## 7. Fichiers

- Trace et scripts : `tools/door_room7_trace.tcl`, `tools/run_door_room7_trace.py`, `tools/door_room7_scan.py`.
- Journaux (non versionnés) : `scratchpad/door7/runs/t0`, `p1`, `p2`, `p3b`, `p4`, `p5`, `p6`. Dumps de la carte : `scratchpad/door7/runs/p3b/room_*.bin`, `p4/room_s*.bin`.
- Données existantes : `research/room-links.md`, `research/down-stairs.md`, `research/mechanics-remaining.md` (M33, U5).

## 8. Setter de C701 bit 0 (analyse statique et dynamique)

- **Setter unique** (preuve par octets de la ROM) : `LD B,1 ; CALL 8E6D` en CPU 8E60 (banque 2, octet fichier 0x4E60). `8E6D` fait `LD HL,C701 ; OR B ; LD (HL),A`. Aucun autre `SET 0` ni OR sur C701.
- **Entrée** : une seule, la table de sauts en CPU 8D3E (entrée 23, état A = 0x18), atteinte par `8D30`. Appelants de `8D30` : `94E8` (A = C708), `8D29` (A = (IX+4), depuis 8BFE et 2046C), `8D2C` (depuis 9A9E).
- **Gate de sortie** : `7737` (banque 1) efface le bit 0 et laisse passer `772A` (qui fait `INC C5AC`) seulement si le bit était à 1. Passer de C5AC = 1 à 5 exige donc au moins deux événements « bit 0 » (1 vers 2, puis 4 vers 5). Entre les deux, un compteur `C5AF` en banque 2 (`9147`).
- **Dynamique** (openMSX, watchpoints) : parcours naturel 1 vers 0 vers 3, puis montée vers 7. Toutes les écritures de C701 valent 0. Le bit 0 n'a jamais été mis à 1, et `8E60` n'a jamais été atteint. Le chemin naturel vers la sortie n'est donc **pas trouvé**.
- **Adresses** : le mapping CPU = offset + 0x4000 vaut pour les fenêtres 4000 et 6000, mais pas pour 8000, qui change de banque pendant le jeu (`509F` écrit 0x0E dans (8000h)). Le code de banque 2 lu statiquement n'est donc pas forcément celui exécuté.
- **Ce qui tranchera** : trouver ce qui produit A = 0x18 en `8D30` (watch de C419 pendant le jeu), identifier la banque mappée en 8000 dans la salle 7, et tracer les écritures de C5AC non étudiées (1EC3, 2559, 2B12, 2B5D, 2C81, 303C, 37B3, 383A, 3868, 388A, 3917, 5147, 516A).
- **Preuves** (hors dépôt) : `scratchpad/door7/c701/`.

