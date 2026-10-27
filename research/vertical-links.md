# Liens verticaux HAUT/BAS du hall (stage 1) : déclenchement et arrivée

Statut : **routine de sortie identifiée et tracée ; sorties HAUT 1→5, BAS 5→1, HAUT 3→7, BAS 7→3, GAUCHE 5→4 et DROITE 5→6 mesurées en jeu ; cause exacte du blocage observé par l'utilisateur non reproduite.**

ROM de référence : `Vampire Killer (Japan, Europe) (En).zip` (racine), SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`, vérifié. Adresses CPU Z80 ; quand une adresse est donnée en fenêtre `6000`, elle suppose la banque 1 mappée en `6000-7FFF` (confirmé par les PC des écritures tracées, voir §5).

Lien de référence : `research/room-links.md` (table décodée et liens L/R), `research/stairs.md` (volées, tuiles 12/13).

## 1. Résumé

- Les sorties HAUT et BAS ne sont pas des tests de bord « sans condition ». Elles passent par la même routine que les sorties gauche/droite, la routine `767B`, qui écrit une **requête de sortie** dans `C41B` :
  - `C41B = 1` : HAUT, écrite en `76B9`, seulement si `C420 = 3` (mode escalier) et `Y < 48`.
  - `C41B = 2` : BAS, écrite en `76FF`, si `Y >= 225` (quel que soit le mode).
  - `C41B = 3` : GAUCHE (`770A`), `C41B = 4` : DROITE (`7715`), si `X < 8` ou `X >= 248`.
- La requête est consommée par `B963` (écriture de `D001` en `B987`), qui lit la salle cible dans les nibbles de la table (§4).
- **Une sortie HAUT ou BAS en escalier arrive en mode escalier (`C420` reste à 3) et non sur le sol** : HAUT arrive à `Y = 224` (sous le sol), BAS arrive à `Y = 48`. `X` est décalé de ±16.
- Salle 5, arrivée HAUT depuis la salle 1 : Simon se retrouve à `(207, 224)` en mode escalier, glisse seul vers `(200, 217)` et s'arrête. Sans touche, il reste dans ce mode (observé pendant plus de 120 images, `runs/a2`). Le saut ne fait rien. GAUCHE ou HAUT le ramènent sur le sol à `(175, 192)`. DROITE ou BAS le font redescendre à `Y = 225` et ramènent en salle 1 à `(192, 48)`.

## 2. Routine de sortie (ROM, fenêtre 6000, banque 1)

Code ci-dessous, lu par `tools/disasm_rom_window.py` (z80dis) sur le fichier ROM. Entrée `767B`.

| Adresse | Rôle | Statut |
|---|---|---|
| `767B` | `LD A,(C420)` ; `CP 6` ; `RET Z` (pas de sortie si `C420 = 6`) | lu + prouvé (trace : aucune requête pendant `C420 = 6`) |
| `7681` | `HL = C41B`, `DE = C425 (Y)`, `BC = C427 (X)` | lu |
| `768A`-`768F` | `CP 3` sur `C420`, puis `A = Y` | lu |
| `7692`-`7694` | si `C420 = 3` : `CP 30h` ; `JR C` si `Y < 48` → `76A4` (HAUT) | lu + prouvé (trace : `76B9` écrit `C41B=1` avec `C420=3`) |
| `76A4`-`76B9` | HAUT : `Y := E0h (224)`, `X += ±16` selon `C42C`, `C41B := 1`. Aucun test de `C41C` (voir §9, point 3) | lu + prouvé (trace) |
| `7696`-`7698` | `CP E1h` ; `JR NC` si `Y >= 225` → `76BC` (BAS) | lu |
| `76BC`-`76C2` | BAS : si `C41D = FF` → chute (`C420 := 6`, `Y := FAh`, `C428 := 1Eh`) | lu, non tracé |
| `76E8`-`76EA` | BAS avec lien : `Y := 30h (48)` | lu + prouvé (trace : `Y = 48` après sortie) |
| `76EB`-`76FE` | si `C420 = 3` : `X += ±16` selon `C42C` (0 → -16, sinon +16) | lu + prouvé (trace) |
| `76FF` | `C41B := 2` (BAS) | lu + prouvé (trace : `76FF` écrit `C41B=2`) |
| `769A`-`76A1` | GAUCHE si `X < 8` (`CP 8`) et `C41E ≠ FF` : `X := F6h (246)`, `C41B := 3` (`770A`) | lu + prouvé (trace : `770A` écrit `C41B=3`) |
| `769F`-`76A1` | DROITE si `X >= 248` (`CP F8h`) et `C41F ≠ FF` : `X := 0Ah (10)`, `C41B := 4` (`7715`) | lu + prouvé (trace : `7715` écrit `C41B=4`) |

Appelant direct de `767B` : `6B24` (`CALL 767B` ; fenêtre 6000, offset fichier `0x2B24`), dans un bloc de mise à jour par image. La chaîne complète depuis la boucle d'état 5 n'est pas remontée.

## 3. Mécanisme : requête, cible, changement de salle

| Étape | Adresse | Preuve |
|---|---|---|
| L'état 5 (`4257`) passe à l'état 9 si `C41B ≠ 0` (`LD A,(C41B)` ; `AND A` ; `LD A,9` ; `JP NZ,41B6`) | `4257`-`4270` | lu + prouvé (trace : `C000 = 9` après chaque `C41B` non nul) |
| L'état 9 (`4377`) appelle `5A2E`, qui appelle `B963` (`CALL B963` en `5A31`) | `4377`, `5A2E`, `5A31` | lu ; `5A2E` appelé aussi depuis `4E28` (routine `4E20`, `CALL 5A2E` si `C41B ≠ 0` ; rattachée à l'état 2 par `research/player-health.md`, non vérifié) |
| `B963` : `C41B - 1` pilote le choix du nibble ; si le nibble est `F`, `RET Z` (pas de salle) | `B963`-`B986` | lu + prouvé (trace : `B987` écrit `D001` avec la valeur de la table) |
| `B987` : `LD (D001),A` | `B987` | prouvé (trace, PC `B987` pour chaque changement) |
| Chargement de la salle, positionnement | `62F5` (non désassemblé en détail) | arrivée mesurée, voir §4 |
| Cas `C41B = FF` (chemin spécial) : `C41B := 0`, `D001 := (C5B4)` | `B98C`-`B993` ; `FF` écrit en `7109` | lu, non tracé |

## 4. Liens verticaux prouvés (stage 1)

Table : `D001` lue dans `writes.csv` au PC `B987`. Position mesurée à l'image où `C000` redevient 5 (fin de transition), sauf mention.

| Sortie | Condition de déclenchement (mesurée) | Écriture (PC, valeur) | Salle cible (`D001`) | Arrivée (X, Y), mode | Confiance |
|---|---|---|---|---|---|
| 1 HAUT → 5 | mode 3 (volée C), `Y` passe de 48 à moins de 48, touche HAUT tenue | `76B9`, `C41B=1` ; `C42C=1` | 5 | (207, 224), `C420=3` | **prouvé** (`runs/a2`, frame 2046-2048) |
| 5 BAS → 1 | mode 3 en descente, `Y` atteint 225 (la volée descend jusqu'à `Y=224`) | `76FF`, `C41B=2` ; `C42C=0` | 1 | (192, 48), `C420=3` | **prouvé** (`runs/cR`, `runs/f1` frame 2233-2235) |
| 3 HAUT → 7 | mode 3 (volée à `(48, 64)`), `Y < 48`, HAUT tenu | `76B9`, `C41B=1` ; `C42C=0` | 7 | (49, 224), `C420=3` | **prouvé** (`runs/d1`, frame 2427-2429) |
| 7 BAS → 3 | mode 3 en descente, `Y >= 225` | `76FF`, `C41B=2` ; `C42C=1` | 3 | (64, 48), `C420=3` | **prouvé** (`runs/e1`, frame 2614-2617) |
| 5 GAUCHE → 4 | marche sur le sol (`C420=0`), `X < 8` | `770A`, `C41B=3` | 4 | (246, 192), `C420=0` | **prouvé** (`runs/f1` frame 2942-2944) |
| 5 DROITE → 6 | marche ou saut sur le sol, `X >= 248` | `7715`, `C41B=4` | 6 | (10, 192), `C420=0` | **prouvé** (`runs/g2` marche-saut ; `runs/h1` marche sans saut, frame 2437-2439) |

Remarques :
- Dans toutes les sorties ci-dessus, `D001` reçoit la valeur de la **case de la table** qui correspond à la direction (`C41C` HAUT, `C41D` BAS, `C41E` GAUCHE, `C41F` DROITE). Voir §5.
- X après sortie verticale : `X = X_avant ± 16` (signe selon `C42C`), à ±1 px près à cause du pas d'escalier de la même image (`192 → 207`, `64 → 49`, `207 → 192`, `49 → 64`). Y n'est pas décalé : `224` (HAUT) et `48` (BAS).
- Les sorties BAS et HAUT **hors** escalier (`C420 ≠ 3`) ne sont pas tracées. Le code BAS (`76E8`-`76FF`) n'applique pas le décalage de X quand `C420 ≠ 3` (déduit).

## 5. Ordre des nibbles (HAUT, BAS, GAUCHE, DROITE)

**Prouvé** :
- `B99A` (appelée depuis `5A3A`) remplit `C41C`, `C41D`, `C41E`, `C41F` avec les nibbles dans l'ordre HAUT, BAS, GAUCHE, DROITE (`B9A2`-`B9BA`, `FF` si nibble `F`).
- Valeurs lues dans `C41C..C41F` pendant les runs, comparées à la table :

| Salle | `C41C` HAUT | `C41D` BAS | `C41E` GAUCHE | `C41F` DROITE | Table (`decode_room_links.py`) |
|---|---|---|---|---|---|
| 1 (`runs/a2`, frame 2047) | 5 | FF | 0 | 2 | HAUT 5, BAS -, GAUCHE 0, DROITE 2 : concordant |
| 3 (`runs/d1`, frame 2427) | 7 | FF | 2 | 0 | HAUT 7, BAS -, GAUCHE 2, DROITE 0 : concordant |
| 5 (`runs/cR`, frame 2049) | FF | 1 | 4 | 6 | HAUT -, BAS 1, GAUCHE 4, DROITE 6 : concordant |
| 7 (`runs/e1`, frame 2615) | FF | 3 | 6 | FF | HAUT -, BAS 3, GAUCHE 6, DROITE - : concordant |

- Le code de requête vaut `1` HAUT, `2` BAS, `3` GAUCHE, `4` DROITE (PC `76B9`, `76FF`, `770A`, `7715`).

## 6. Ce qui se passe après l'arrivée en salle 5

### 6.1 Arrivée mesurée

Arrivée HAUT depuis la salle 1 (`runs/a2`) : `(207, 224)`, `C420 = 3`, `C421 = 0`, `C42C = 1`.

| Image | Événement | Position |
|---|---|---|
| 2047 | écriture de `C41B = 1` (`76B9`) | (207, 224) |
| 2048 | `B987` écrit `D001 = 5` ; `C000 = 9` | (207, 224) |
| 2049-2066 | transition (`C000 = 9`) ; `C420` reste à 3 | (207, 224) |
| 2067-2080 | Simon glisse seul vers le bas-gauche, 1 px en X et 1 px en Y toutes les 2 images | (207, 224) → (200, 217) |
| 2080-2335 | arrêt sans touche, `C420 = 3` | (200, 217) |

### 6.2 Réaction aux touches depuis (200, 217) en mode 3

| Touche (maintenue) | Résultat mesuré | Run |
|---|---|---|
| aucune | reste à (200, 217), mode 3 indéfiniment | `runs/a2`, `runs/b1` |
| GAUCHE | descend l'escalier vers la gauche, atteint le sol à (175, 192) (mode 0), continue vers x=107 selon le temps de maintien | `runs/b1` (frame 2224-2339) |
| HAUT | monte vers la gauche, atteint le sol à (175, 192) (mode 0) puis reste | `runs/cU` (frame 2274) |
| DROITE | l'escalier descend ; `Y` atteint 225 : **sortie BAS vers salle 1 à (192, 48)**, mode 3 | `runs/cR` (frame 2233-2236) |
| BAS | idem DROITE : sortie BAS vers salle 1 à (192, 48) | `runs/cD` (frame 2236) |
| SAUT (S) | aucun effet, mode 3 reste à (200, 217) pendant 40 images | `runs/cS` |

Depuis (192, 48) en salle 1, mode 3 : HAUT remonte la volée et revient en salle 5 à `(207, 224)` (`runs/f1`, frame 2477-2480). Donc le couple 1↔5 peut être parcouru en boucle.

### 6.3 Sorties de la salle 5 au sol

Au sol, en `(175, 192)` (mode 0) :
- GAUCHE : atteint la salle 4 en `(246, 192)` (`runs/f1`).
- DROITE sans saut : traverse la zone `x = 184..191` à `y = 192` sans chute, atteint `x = 229`, puis la salle 6 en `(10, 192)` (`runs/h1`).
- DROITE avec saut : même résultat (`runs/g2`).

### 6.4 Géométrie de la salle 5

Source : `sgdk/src/generated/stage1_collision.c`, salle 5 (index 5 du tableau `vk_stage1_collision`). Ligne `y = 16 + 8 * row`. `#` = tuile 1 à 4 (solide pour le stage 1, voir `research/stairs.md` §2.3), `+` = tuiles 12 ou 13 (escaliers), `%` = autre tuile, non solide.

```
row y=128 : ###########%%%...   solide x = 0..87
row y=136..184 : diagonale de tuiles + de (88,136) à (176,184)
row y=192 : #######################%########   solide x = 0..183, (x=184..191 non solide dans la table), solide x = 192..255
```

- Le sol au sol `y = 192` est solide de x = 0 à 183 et de x = 192 à 255. Le trou de la table en `x = 184..191` n'est pas confirmé : Simon le traverse à pied (`runs/h1`).
- **Le point d'arrivée HAUT `(207, 224)` est à 32 px sous le sol solide le plus proche (`y = 192`, sous x=207).** Ce n'est pas un point d'appui : Simon est en mode escalier.
- Le point `(175, 192)` de notre implémentation (`vk_stage1_links`, `vk_floor_arrival`) est exactement le point où la montée depuis `(207, 224)` atteint le sol (`runs/cU`, frame 2274). Il est donc accessible, mais le prototype arrive directement au sol et ne reproduit pas la montée en mode escalier.

### 6.5 Ce qui peut bloquer Simon en salle 5 (déduit des mesures)

- En mode 3 le saut est inactif. Si l'utilisateur appuie sur saut ou ne touche à rien après l'arrivée, Simon reste à `(200, 217)`.
- DROITE ou BAS, appuyés à l'arrivée, renvoient en salle 1 à `(192, 48)` (sortie BAS). Un joueur qui ne voit pas que GAUCHE ou HAUT sont nécessaires peut croire que la salle est bloquée.
- Aucune des mesures ne montre de blocage dur (aucun état sans sortie). La cause réelle du blocage signalé par l'utilisateur reste **inconnue** : sa séquence de touches n'est pas connue, et le prototype (`sgdk/src/gameplay.c`) n'arrive pas dans le même état (`(175, 192)`, mode 0, et sortie HAUT déclenchée par `y < 32` sans escalier).

## 7. Écarts avec le prototype (`sgdk/src/gameplay.c`, `room_manager.c`)

| Élément | Prototype | Authentique (mesuré) |
|---|---|---|
| Déclencheur HAUT | `p->y < 32` sans condition d'escalier | `C420 = 3` et `Y < 48` (`76B9`) |
| Arrivée HAUT | sol sous `X` (`vk_floor_arrival`), `Y` d'après le sol | `Y = 224`, `X ± 16`, mode 3 conservé |
| Déclencheur BAS | absent | `Y >= 225` (`76FF`), quel que soit le mode |
| Arrivée BAS | sol sous `X` | `Y = 48`, `X ± 16` seulement en mode 3 |
| Liens 1↔5 et 3↔7 | présents (`vk_stage1_links`) | présents et confirmés (tables §4) |

Ces écarts ne sont pas corrigés dans ce travail (aucune modification de `sgdk/`, `tests/`, `validation/`, `.claude/`).

## 8. Méthode

1. Lecture de la table et de `B963` (`tools/decode_room_links.py`, désassemblage de `B900`-`BA30`, banque 13 en `A000`).
2. Recherche des appelants : `CALL B963` en `5A31` (fenêtre 4000, banque 0) ; `CALL 767B` en `6B24` ; `LD (C41B),A` en `3109`, `37ED`, `B98D` ; `LD HL,C41B` en `3681`.
3. Désassemblage de la routine `767B`-`7717` (banque 1, fenêtre 6000) via `tools/disasm_rom_window.py`.
4. Traces openMSX 21.0, C-BIOS_MSX2, ROM en cartouche, script `tools/vertical_links_trace.tcl` (lancement par `tools/run_vertical_links_trace.py ROM.rom OUTDIR PLAN`). Watchpoints `write_mem` sur `D000`, `D001`, `C41B` (PC via `CPU regs` 20/21). Journal par image (`C000`, `C420`, `C41B`, `X`, `Y`, `C41C..C41F`, `C42C`). Synchronisation sur `0x4028` (VBLANK).
5. Parcours : titre passé, départs par la route connue (cour 0 → 1 → 2 → hall 0 → hall 1 à `x=10, y=192`), puis :
   - **Positionnement forcé** pour lancer la volée C : `debug write memory` sur `C427 = 240` et `C425 = 96` (le pied de la volée est à `X` dans `[232, 247]` et `Y = 96`, `research/stairs.md` §3.2), puis HAUT maintenu. Ce positionnement n'est pas une marche naturelle ; le franchissement de la volée a ensuite été naturel (HAUT tenu, sortie automatique).
   - Idem pour la volée de la salle 3 : `C427 = 52`, `C425 = 64` après GAUCHE depuis la salle 1 (passage naturel par la salle 0), puis HAUT.
6. Chaque sortie est vérifiée par `writes.csv` (PC, valeur) et `frames.csv` (position image par image).

Journaux : `scratchpad/vlinks/runs/` (non versionné) : `a2` (1→5, rejeu après réécriture des conditions en `else` imbriqués), `b1`, `cU`, `cR`, `cD`, `cS`, `d1` (3→7), `e1` (7→3), `f1` (5→4), `g2` (5→6 saut), `h1` (5→6 marche).

Fichiers versionnés : `tools/vertical_links_trace.tcl`, `tools/run_vertical_links_trace.py`, `tools/disasm_rom_window.py`.

## 9. Inconnues

1. **Chaîne d'appel complète** de `6B24` (boucle par image de l'état 5) et de `767B` : non remontée jusqu'à la réinitialisation de `C000`.
2. **HAUT hors escalier** (`C420 ≠ 3`) : le code n'a pas de sortie HAUT. Non tracé en marchant vers le haut du hall, seulement déduit du code et du balayage de `research/room-links.md` (aucun HAUT trouvé).
3. **Cas `C41C = FF`** (HAUT sans lien) : `767B` n'a pas de test ; `B963` retourne sans `SCF` ; `4377` met alors `C000 = 6` (`LD A,6 ; JP NC,41B6`). L'effet de l'état 6 vient de `research/mechanics-remaining.md` (non tracé ici).
4. **Cas `C41D = FF`** (BAS sans lien) : chute (`C420 := 6`, `Y := FAh`). Non tracé.
5. **BAS hors escalier** (`C420 ≠ 3`) : `Y := 48` sans décalage de X (déduit du code `76E8`-`76FE`). Non tracé.
6. **Cause du blocage signalé par l'utilisateur** : inconnue. Les mesures montrent une sortie de salle 5 ou un retour en salle 1 selon la touche, pas de blocage dur.
7. **Trou du sol** `x = 184..191` en salle 5 : la table statique le déclare non solide, Simon le traverse à pied. Non expliqué.
8. **Stage 6** (`CE00 = 6`, voir `research/stairs.md` §2.4) : non étudié.
9. **Mapping de la fenêtre 6000** : déduit (bank 1), cohérent avec tous les PC tracés, mais pas vérifié par un dump de registres de banque en jeu.
