# Vampire Killer : portage MSX2 vers Mega Drive (SGDK)

Portage en C (SGDK, 68000) de *Vampire Killer* (Konami, MSX2). Le jeu original
n'est pas inclus : le portage lit une ROM que tu fournis, pour en extraire les
données de la Mega Drive et valider le comportement contre l'original.

Dépôt : https://github.com/sukoshi1507/vampirekiller-MD

## État du portage

Ce qui fonctionne dans la ROM Mega Drive :

- **Cour (stage 0)** : trois écrans, marche, saut, accroupi, fouet (niveaux 0 à 2),
  flammes animées. Une torche touchée s'éteint et joue le son de coup.
- **Escaliers** : montée (HAUT) et descente (BAS) des volées du hall, avec la
  direction de regard. Les sorties verticales arrivent en glissade, et la sortie
  basse de la salle 5 reprend la volée de la salle 1.
- **Hall du château (stage 1)** : huit salles, liens gauche/droite et haut/bas.
- **Ennemis du hall** : type 1 (salles 0, 1, 5, 6) et type 4 (salle 4), avec
  apparition, mouvement, animation, contact (-2 d'énergie, recul, invulnérabilité),
  chute sur le sol et couleurs d'après la table de couleur du MSX2.
  **Le fouet tue les ennemis de types 1 et 4** (type 1 confirmé à l'émulateur).
- **Clé et porte** : la clé blanche de la salle 2 se ramasse au contact et s'affiche
  dans le HUD ; la porte de la salle 7 bloque Simon sans elle, s'ouvre avec (barreau
  qui se lève, Simon qui sort seul) et mène vers le stage 2.
- **Énergie, vies, mort, game over** : la barre PLAYER du HUD suit l'énergie (32 points).
  À 0, Simon meurt ; 252 images plus tard l'énergie revient au maximum et, 27 images
  après, il perd une vie et reprend. Sans vie, « GAME OVER » puis le titre (378 images).
  Délais et compteur de vies (`C410`, 2 en jeu, 3 morts au total) mesurés sur le MSX2
  (`research/player-health.md`).

Ce qui n'est pas porté ou reste provisoire :

- **Type 4** (salle 4, volant) : dessin, battement d'ailes et vol en vague d'après le
  MSX2 ; tué par le fouet d'après la table des objets (boîte provisoire, direction
  du fouet non vérifiée, `research/enemy-behavior.md`).
- **Type 5** (salles 2, 3, 7) : non porté (apparition, mouvement, graphisme inconnus).
- **Stage 2** : la porte de la salle 7 y mène, mais il n'est pas jouable ; sa première
  salle s'ouvre dans la visionneuse de salles.
- **Clé dorée** et coffre : non portés.
- **Stages 2 et suivants, châteaux, attaques autres que le fouet** : non portés.
- **Énergie (provisoire)** : échelle de la barre (2 px par point), lieu de reprise
  (entrée du hall), commandes bloquées pendant la mort, écran de game over : non
  relevés sur le MSX2. Les vies (`P-02`), les cœurs et le score du HUD restent figés.
  Pas d'animation de mort ni de clignotement d'invulnérabilité. La chute dans le vide
  ne tue pas.
- **Provisoires** : vitesse de chute des ennemis, boîte de contact de Simon.

Le détail par sujet est dans [research/INDEX.md](research/INDEX.md).

## Construire la ROM

Prérequis : SGDK installé à part (non versionné) et la chaîne `m68k-elf-gcc`.

```sh
cd sgdk
make GDK=/chemin/vers/SGDK
```

La ROM est `sgdk/out/rom.bin`. La compilation doit sortir sans avertissement.
`sgdk/out/` est ignoré par Git.

## Tests

Tests natifs, compilés avec le compilateur C de la machine (sans SGDK) :

```sh
sh tests/run_native.sh
```

Ils couvrent la physique, les escaliers, les liens de salles, le fouet, les cibles
et les ennemis. Ils doivent afficher PASS.

Tests Python (certains lisent la ROM de référence) :

```sh
python3 -m unittest discover -s tests -p 'test_*.py'
```

## Données générées

Les données extraites de la ROM sont versionnées dans `sgdk/src/generated/` et
`assets/`. Elles se régénèrent avec les scripts de `tools/` :

| Script | Produit | Entrée |
| --- | --- | --- |
| `tools/build_enemy_sprites_md.py` | `generated/enemy_sprites_md.c/.h` | ROM via `VK_ROM_PATH` |
| `tools/build_sfx_md.py` | `generated/sfx_md.c` (son du fouet) | dépôt castlevania-md voisin |
| `tools/build_sfx_hit_md.py` | `generated/sfx_hit_md.c` (son de coup) | dépôt castlevania-md voisin |
| `tools/build_door_key_md.py` | `generated/door_key_md.c/.h` (clé, case du HUD, barreau) | instantanés VRAM openMSX du run K3 (`scratchpad/`, non versionnés) |

## Organisation

| Chemin | Contenu |
| --- | --- |
| `sgdk/src/` | Code du jeu : `player.c` (physique, escaliers), `whip.c`, `enemies.c`, `gameplay.c`, `room_manager.c`, `health.c` (vies, mort), `main.c` (boucle), `scene.c` (logo, titre, château), `room_view.c` (salles, flammes), `actors.c` (sprites de Simon, du fouet, des ennemis), `hud.c` (barre d'énergie) |
| `sgdk/src/generated/` | Données générées depuis la ROM (cartes, sprites, sons) |
| `tests/` | Tests natifs (C) et Python |
| `tools/` | Extraction, désassemblage, traces openMSX (`*.tcl`), générateurs |
| `research/` | Notes de recherche sur la ROM MSX2 (index dans `research/INDEX.md`) |
| `assets/` | Images et manifestes extraits |
| `validation/` | Captures et rapports de validation |

Les anciennes notes à la racine (`V*.md`, `*_V*.md`, `ROM_*.md`, etc.) sont les
journaux des versions précédentes du projet. Elles sont conservées telles quelles.

## Licence

Le code écrit pour ce portage est sous **GPL-3.0-only** (fichier `LICENSE`).

Ce qui n'est pas couvert par cette licence :

- **Données de Konami** : graphismes, cartes, sprites, enregistrements audio
  extraits de la ROM (dans `assets/`, `sgdk/src/generated/`, `research/`, `validation/`).
  Elles restent sous copyright Konami. La ROM elle-même n'est pas incluse.
- **Sons du fouet et de coup** : dérivés de castlevania-md, qui ne publie pas de licence.
- **SGDK** : licence MIT (`third_party/SGDK-LICENSE.txt`).

Détail et points à trancher : `research/licensing.md`.
