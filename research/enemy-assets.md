# Ennemis de la salle 1 du hall : graphismes et palette (état)

Statut : **motifs des types A, B et C extraits de la ROM et vérifiés octet pour octet contre la VRAM d'openMSX**. Les PNG sont produits par `tools/extract_enemy_sprites.py` dans `scratchpad/enemy-assets/sprites/` (non versionné). **La couleur de jeu n'est pas résolue** : les PNG utilisent des indices de couche.

ROM de référence : SHA-256 interne `99eabaae...` (copie `scratchpad/rom/`). Adresses CPU sauf mention contraire.

## 1. Motifs VRAM : emplacement (corrigé)

- Table des motifs de sprites : `R#6 = 31`, base `F800`. Un motif N occupe `F800 + 8*N` (8 octets par motif 8x8). Un sprite 16x16 utilise `N & FC` ; la couche est dans l'ordre UL (octets 0-7), LL (8-15), UR (16-23), LR (24-31).
- L'ancienne formule `F800 + (N & FC) * 32` (ici et dans `whip/render.py`) est **fausse** pour les 16x16 : elle faisait sortir N >= 40 de la VRAM. Avec `*8` (déjà utilisé par `research/whip-variants.md`), N = 40..6C tombe dans `FA00-FB7F`, où la VRAM contient des motifs de sprites cohérents.
- SAT : `F600` en VRAM (copie RAM `D600`), 4 octets par entrée (Y, X, N, couleur). `Y = E0` / `D0` : entrée masquée.

## 2. Correspondance type -> motifs (prouvé, VRAM openMSX)

Capture : `scratchpad/enemy-assets/work/run-a/vram-f02222.bin` (image trace 2222, deux ennemis en salle 1, D000=1, D001=0). Chaque ennemi = deux cellules 16x16 empilées (Y A0 puis B0), même X ; chaque cellule = deux couches N et N+4.

| Type | Cellule haute (Y A0) | Cellule basse (Y B0) | Source |
|---|---|---|---|
| A (X 68 à l'image 2222) | 40/44 | 48/4C ou 50/54 (alternance) | pairs 0 (haut), 1 et 2 (bas) du flux `0x153E8` |
| B (X 19 à l'image 2222) | 58/5C = miroir de 40/44 | 60/64 = miroir de 48/4C, 68/6C = miroir de 50/54 | même flux, **miroir horizontal** |
| C (Y 70, D001=2, image 2844) | 70/74 (X 5C) et 78/7C (X 6C), côte à côte | - | flux `0x15947`, offsets 1002 et 1066 |

- Le miroir horizontal : les quadrants gauche/droite sont échangés (UL<->UR, LL<->LR) et chaque octet est inversé bit à bit. Vérifié pour les 6 couches de B.
- Dans les images de type D001=1 et 2, les mêmes N (40..6C) réapparaissent à Y 70 (`vram` non capturée seule, SAT observée dans `sat-log.txt`).
- Animation de A et B : haut fixe, bas alterne toutes les 8 images (cf. `research/enemies-platforms.md`, 1.2). Les deux poses bas sont donc deux cellules distinctes.

## 3. Source ROM (prouvé)

- Flux RLE 1 : **fichier `0x153E8`** (banque 10 du fichier, décalage `0x13E8` dans la banque ; adresse CPU selon la fenêtre choisie par le jeu, non établie). Décodé par le même RLE que Simon (`tools/extract_player.py`) : 0 termine, 80 saute à une adresse 16 bits, 81-FF copie `n & 7F` octets, 01-7F répète l'octet suivant. Sortie : **192 octets = 3 paires de 64 octets** (couches 40/44, 48/4C, 50/54).
- Flux RLE 2 : **fichier `0x15947`** (banque 10, décalage `0x1947`). Sortie : 1130 octets ; les couches 70/74 sont à l'octet 1002, 78/7C à l'octet 1066.
- Un saut `80` est résolu dans la même banque 8 Ko (hypothèse, suffisante pour ces flux : les 6+4 couches sont retrouvées telles quelles).
- Vérification : les 16 couches du manifeste (A, B, C) sont égales aux octets VRAM de `vram-f02222.bin` (A, B) et `vram-f02844.bin` (C).

## 4. Palette (prouvé)

- Palette VDP lue par `debug read_block {VDP palette}` pendant la capture 2222 : 16 entrées, identique à la palette de la salle (stage 1, salle 0) décodée par `tools/extract_rooms.py` (`decode_room(data, 1, 0)`). Palette écrite dans le manifeste et utilisée par les PNG (aperçu seulement).
- Index 0 = transparent dans les PNG.

## 5. Couleur (résolue pour les types A et B)

- Les sprites du mode 2 du V9938 prennent leur couleur dans une table de couleur de 16 octets par sprite, placée 512 octets avant la SAT : `F400 + 16 * numéro de sprite` quand la SAT est en `F600`. Chaque octet = couleur (bits 0-3) et CC (bit 6, combinaison avec la couche dessous).
- Mesure corrigée (2026-10-27) : dans chaque vidage VRAM du hall (`scratchpad/enemy-assets/work/run-a/vram-f02222.bin` et `vram-f02844.bin`), la SAT (`F600`) et la table de couleur (`F400`) sont lues **dans le même vidage**. Toutes les paires suivent la même règle, prouvé pour 40/44, 50/54, 58/5C, 60/64, 68/6C :
  - première couche N : octet `02` (couleur 2) ;
  - seconde couche N+4 : octet `44` (couleur 4, CC) ;
  - les 16 octets (une couleur par ligne) sont identiques.
  - 48/4C ne sont pas à l'écran dans ces vidages : même règle supposée.
- L'ancienne table (48 → 1, 4C → 2, 68 → 1, 6C → 2) associait la SAT de `info-f02222.txt` à la table de couleur du vidage VRAM, prises à des instants différents : les couleurs 1/2 étaient celles des sprites de Simon. Elle est abandonnée.
- CC : là où les deux couches ont un pixel, le V9938 affiche le OU des couleurs (2 | 4 = 6). `tools/build_enemy_sprites_md.py` cuit ce OU dans la première couche, dessinée au-dessus sur MD : le rendu est exact sans combinaison matérielle.
- Indice de couleur = indice de la palette MSX2 de la salle, donc l'indice de PAL2 dans le jeu.

## 6. Fichiers

- `tools/extract_enemy_sprites.py` : `python3 -I tools/extract_enemy_sprites.py ROM.rom [SORTIE]`. Écrit 5 cadres PNG 4 bits (typeA_frame0/1, typeB_frame0/1, typeC_frame0) et `manifest.json` (décalages, SHA-256 des couches, palette). Défaut de sortie : `scratchpad/enemy-assets/sprites/`.
- Captures de travail (hors dépôt) : `scratchpad/enemy-assets/work/run-a/` (SAT, VRAM aux instants où N change, `info-*.txt`), `run-b/` (registres VDP et palette), scripts de recherche `search_*.py`, `map_layers.py`.
- Trace : `scratchpad/enemy-assets/work/trace-a.tcl` (titre SPACE, RIGHT de 25 s à 60 s, dump VRAM à chaque nouvel ensemble de N).

## 7. Types restants

- Hors du hall stage 1 (D000=1) : N = 00..0C (Simon), 10..3C (scène d'intro et pièce de départ, D000=0), non extraits.
- Type C : seul le couple 70/74 + 78/7C est vu ; sa pose (une seule image observée) et sa couleur restent à confirmer.
- Non fait : hauteur et boîte de contact des types A/B/C, couleur, et les autres salles du stage 1 (flags 85CF, bits 1 à 5).

## 8. Inconnues

- Quelle routine choisit le flux (`0x153E8` ou `0x15947`) selon le type et la phase : non identifiée (les tables de `research/enemy-spawn.md` donnent le type, pas le motif).
- Adresse CPU exacte de la banque 10 pendant la salle 1.
- Mécanisme de couleur (section 5).
