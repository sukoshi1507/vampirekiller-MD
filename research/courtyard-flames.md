# Flammes de la cour — graphismes extraits de la ROM MSX2

ROM de référence : SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`, lue via `tools/probe.py`. Aucune ROM n'est incorporée au dépôt.
Mode d'écran : **SCREEN 5** (`CHGMOD` avec A=5 dans le harnais, R#0=6, R#2=0x1F, 256×212, 4 bpp, 128 octets par ligne). Adresse VRAM d'un pixel : `y*128 + x/2`, pixel de gauche en poids fort.

## 1. Adresses VRAM

| Élément | Coordonnées (x, y) | Adresse VRAM | Page | Statut |
|---|---|---|---|---|
| Flamme A affichée | (64, 144), 16×16 | `0x4820` (ligne 144) | visible (page 0) | **prouvé** |
| Flamme B affichée | (192, 144), 16×16 | `0x4860` | visible | **prouvé** |
| Source état A (F48) | (48, 368), 16×16 | `0xB818` | hors écran (lignes 256–511) | **prouvé** |
| Source état B (F32) | (32, 368), 16×16 | `0xB810` | hors écran | **prouvé** |

Preuves :
- Dump openMSX, 80 trames (ticks 2300–2379, `anim-inventory/vram1/vram.bin`) : seules les lignes 144–159 de la page affichée changent, aux colonnes d'octets 32–39 (x 64–79) et 96–103 (x 192–207).
- Registres VDP capturés (ticks 2300–2330) : chaque mise à jour est une commande **LMMM** (R#46 = 0x98, TIMP : la couleur 0 n'est pas copiée), avec `DX` = 64 ou 192, `DY + NY = 160`, et `SY - DY = 224`. Les sources lisent donc les lignes 368–383 en `SX` = 48 ou 32.
- Sur les trames stables, chaque pixel non nul de F48 (170 pixels) ou F32 (169 pixels) est identique au pixel VRAM correspondant : 0 désaccord sur toutes les trames stables des deux flammes. Les sources restent identiques sur les 80 trames.

## 2. Séquence et tuiles

| Flamme | Ordre (état en trames) | Décalage |
|---|---|---|
| A (x 64) | F48 trames 0–12, transition, F32 trames 14–28, … | référence |
| B (x 192) | F48 trames 0–8, transition, F32 trames 10–24, … | **B est en avance de 4 trames sur A** |

- Chaque état tient 16 ticks (15 trames pleines plus une trame de transition), période 32 ticks ≈ 0,53 s.
- Le déclencheur de changement est `SX` : F48 si `SX` = 48, F32 si `SX` = 32. Le changement de flamme A se produit au tick 2313 (passage à F32) et au tick 2329 (retour à F48) ; celui de B aux ticks 2309 et 2325.
- **Correction de l'inventaire** : `animation-inventory.md` indique « en phase ». La séquence de la VRAM montre un décalage de 4 ticks. Statut : **prouvé** sur 80 trames.
- Les trames de transition (b, d, e, f, g dans l'analyse) ne sont pas des motifs ROM distincts. Ce sont des états composites : une copie partielle en bas de la boîte écrase des lignes, le reste garde les pixels précédents (transparence). Elles ne sont pas exportées.
- Les 16 lignes de la zone 488–492 (hors écran, toute la largeur) changent aussi entre les trames. Ce n'est pas une flamme (pas de 16 px de large) ; son rôle est **inconnu**.

## 3. Tuiles et format

- Motif 16×16 : 128 octets dans la ROM, 16 lignes de 8 octets, pixel de gauche en poids fort (format VDP MSX2 SCREEN 5). Décodé par `decode_packed` dans `tools/build_flame_fx.py`.
- Adresses dans la ROM : F48 à l'offset fichier `0xDF48` (banque `06`, offset dans la banque `0x1F48`) ; F32 à `0xDEC8` (banque `06`, `0x1EC8`). Le cadrage exact des banques dans les fenêtres CPU n'est pas établi : `cpu_address` est `null` dans le manifeste. Chaque motif occupe 128 octets contigus (`0xDEC8`–`0xDF47` et `0xDF48`–`0xDFC7`).
- Les octets ROM sont identiques aux pixels VRAM produits par le code ROM exécuté dans `IntroHarness` (contrôle dans `build_flame_fx.py`, qui échoue si ce n'est pas le cas).
- Sprite Mega Drive 2×2 (SPRITE_SIZE(2,2)) : 4 tuiles 8×8 par trame, en 4 bpp. Ordre en colonnes (haut puis bas, colonne suivante) : haut-gauche, bas-gauche, haut-droite, bas-droite. Ordre à **vérifier** dans un émulateur MD (voir section 6).
- Index 0 = transparence du sprite, cohérent avec le LMMM TIMP de la ROM.
- Index utilisés par les deux motifs : **0, 1, 2, 8, 14**.

## 4. Palette

Source : exécution des routines ROM de la cour (`IntroHarness`, écritures sur le port 9A), salle 0 — même chemin que `tools/extract_intro.py`. Les valeurs ci-dessous sont les 3 bits par composante (R, G, B), conversion MD `0000 BBB0 GGG0 RRR0` (`(b<<9)|(g<<5)|(r<<1)`).

| Index | RGB333 | CRAM MD | Utilisé par les flammes |
|---|---|---|---|
| 0 | (0,0,0) | 0x0000 | transparent |
| 1 | (7,5,4) | 0x08AE | oui |
| 2 | (1,1,1) | 0x0222 | oui |
| 8 | (7,0,1) | 0x020E | oui |
| 14 | (7,7,7) | 0x0EEE | oui |

- Vérifications : la salle 0 reproduit le dump openMSX à tick 2300 pour les 54 272 pixels de la page 0 (0 différence). La salle 1 donne le même écran ; la salle 2 diffère.
- Statut de la palette : **candidate**. Elle n'est pas encore comparée à la palette VDP vivante d'openMSX (le debuggable `VDP palette` existe dans cette version, non lu ici). Les indices sont prouvés ; les RGB ne le sont que par exécution du harnais.
- Les couleurs ne sont pas copiées du prototype V15. L'ancien `flame_v15_source.txt` n'est plus utilisé.

## 5. Fichiers produits et modifiés

| Fichier | Rôle | Statut |
|---|---|---|
| `tools/build_flame_fx.py` | Remplace la génération V15 par la lecture ROM et la vérification harnais | modifié |
| `sgdk/src/generated/flame_fx_md.h` | Régénéré ; mêmes noms ; 224 mots | modifié |
| `sgdk/src/generated/flame_fx_manifest.json` | Manifeste : offsets fichier, banque, VRAM, palette, statuts | nouveau |
| `assets/flame_fx/flame_state_A.png`, `flame_state_B.png` | PNG indexés 4 bits, palette de la cour (régénérables) | nouveau |
| `tests/test_flame_fx.py` | Vérifie les deux états ROM et la séquence A,A,B,B / A,B,A | modifié |
| `research/courtyard-flames.md` | Cette note | nouveau |

Non modifiés : `sgdk/src/main.c`, `player.c`, `whip.c`, `gameplay.c`, `validation/`. Aucun commit.

Nombre de trames : `VK_FLAME_FRAMES` reste **4** (`A, A, B, B`) ; `VK_SPARK_FRAMES` reste **3** (`A, B, A`). Le duplicat des états est nécessaire car `main.c` avance par `(flame_clock >> 3) & 3` : 8 ticks par trame, donc 16 ticks par état et période 32, comme dans la ROM.

## 6. Prouvé / déduit / inconnu

**Prouvé**
- Adresses VRAM des sources et des flammes affichées, SCREEN 5, LMMM transparent (R#46=0x98).
- Les deux motifs 16×16 (F48, F32) sont identiques à la VRAM sur les 80 trames, et lisibles dans la ROM aux offsets donnés.
- Cadence 16 + 16 ticks, période 32, et décalage de 4 ticks entre A et B.
- Palette des indices 0, 1, 2, 8, 14 par exécution du harnais, qui reproduit l'écran du dump à 0 pixel près.

**Déduit**
- La transparence de l'index 0 dans le jeu vient du LMMM TIMP (opcode lu, pas d'effet visuel vérifié).
- Le motif F48 est l'état « allumé » ou « plein » ; ce nom n'est pas établi. Les noms A et B suffisent.
- Le décalage de 4 ticks vient de la routine qui traite les deux flammes à des instants différents. Non tracé dans le code.
- Les trames burst (`A, B, A`) n'existent pas dans la cour : c'est un choix d'intégration pour garder l'interface de `main.c`.

**Inconnu**
- Adresses CPU de F48 et F32 dans la ROM (mappage des banques 8 Ko non tracé à l'exécution).
- Rôle de la zone de lignes 488–492 (changements full-width).
- Ordre des tuiles 2×2 dans le VDP Mega Drive (colonnes supposées).
- Palette réellement affichée en MD : comparaison avec la VDP palette d'openMSX non faite.
- Les états transitoires composites (`b, d, e, f, g`) ne sont pas reproduits. Le rendu MSX affiche les pixels résiduels ; le sprite MD n'en contient pas.

## 7. Limites d'intégration (main.c, non modifié)

- `main.c` place la flamme sur la cible de test (`test_target.x/y`) et non à (64,144) / (192,144). Les deux flammes ROM ne sont pas représentées. Le décalage de 4 ticks ne peut pas être exprimé par l'interface actuelle.
- Le burst (`flame_burst`) réutilise des trames de flamme ROM, pas une étincelle ROM.

## 8. Tests et vérifications

- `python3 -m unittest discover -s tests -p 'test_*.py'` : 22 tests, **OK** (le module `z80==1.2.0` de `requirements.txt` est nécessaire : l'environnement de l'agent ne l'avait pas, il a été installé dans le venv du scratchpad, `PYTHONPATH` pointant sur un lien vers le paquet ; le test `test_flame_fx` échoue sans ce chemin).
- `sh tests/run_native.sh` : **PASS**.
- `cd sgdk && make GDK=...` : code de retour 0, **0 avertissement**.
- Scripts d'analyse (hors dépôt) : `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/flames/` (`diffmap.py`, `regions.py`, `states.py`, `source_search.py`, `verify.py`, `harness_check.py`).
