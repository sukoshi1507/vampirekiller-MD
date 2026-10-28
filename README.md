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
- **Ennemis du hall** : type 1 (salles 0, 1, 5, 6), type 4 (salle 4) et type 5, la
  panthère (salles 2, 3, 7 : assise, bondit vers Simon sous 64 px, saute au bord des
  plateformes, -6 d'énergie au contact, tuée par le fouet accroupi), avec
  apparition, mouvement, animation, contact (-2 d'énergie, recul, invulnérabilité),
  chute sur le sol et couleurs d'après la table de couleur du MSX2.
  **Le fouet tue les ennemis de types 1 et 4** (type 1 confirmé à l'émulateur).
- **Clé et porte** : la clé blanche de la salle 2 se ramasse au contact et s'affiche
  dans le HUD ; la porte de la salle 7 bloque Simon sans elle, s'ouvre avec (barreau
  qui se lève, Simon qui sort seul) et mène vers le stage 2.
- **Stage 2 (jouable, chauves-souris seules)** : six salles avec les collisions et les liens
  de la ROM (`tools/build_stage_maps_md.py`) ; arrivée en salle 0 à x 16, y 96, comme
  sur le MSX2.
- **Énergie, vies, mort, game over** : la barre PLAYER du HUD suit l'énergie (32 points).
  À 0, Simon meurt ; 252 images plus tard l'énergie revient au maximum et, 27 images
  après, il perd une vie et reprend. Sans vie, « GAME OVER » puis le titre (378 images).
  Délais et compteur de vies (`C410`, 2 en jeu, 3 morts au total) mesurés sur le MSX2
  (`research/player-health.md`). L'écran de fin de partie reprend celui du MSX2
  (écran effacé, « GAME  OVER » à la police MSX).
- **Pause** : START met le jeu en pause avec le son du MSX2 (silence, jingle, puis
  reprise de la musique où elle était) ; A+START retourne au titre (raccourci de debug).
- **Raccourcis de debug** : C au titre lance une partie au stage 2 ; en jeu, A+GAUCHE ou
  A+DROITE change de salle dans le stage courant.
- **Son** : musiques et effets MSX2 émulés depuis le pilote de la ROM sur le PSG
  (`research/audio-driver.md`). Ajout du port : le cri de Wilhelm (CC0) quand Simon
  tombe dans un trou ou dans l'eau.

Ce qui n'est pas porté ou reste provisoire :

- **Type 4** (salle 4, volant) : dessin, battement d'ailes et vol en vague d'après le
  MSX2 ; tué par le fouet d'après la table des objets (boîte provisoire, direction
  du fouet non vérifiée, `research/enemy-behavior.md`).
- **Type 5** : le retard d'affichage d'une mise à jour du MSX, l'objet de mort (`1E`), le
  score et la boîte de contact accroupi ne sont pas reproduits ; la phase 3 du fouet
  est assimilée au fouet déployé (`vk_whip_phase` 2) et le recul face à une panthère
  assise est provisoire.
- **Stage 2** : seules les chauves-souris (salles 0, 1, 3) sont portées ; le type 1 de la
  salle 2 et les hommes-poissons des salles 4 et 5 ne le sont pas. Pas d'objets. Après une mort, Simon reprend en salle 0
  (x 16, y 96), lieu provisoire. Les liens haut/bas viennent de la table de la ROM,
  sans trace en jeu.
- **Clé dorée et coffre** : portés pour la cour, le hall et le stage 2 (objets fixes,
  clés et coffres des blocs cachés, une seule clé à la fois, attente de 32 images du
  contenu, icône du HUD gardée jusqu'au retour en jeu après une mort) d'après la lecture du code, sans
  trace openMSX dédiée. Les contenus autres que les cœurs n'ont pas d'effet
  (`research/castle-candles.md` 9.7).
- **Stages 3 et suivants, châteaux, attaques autres que le fouet** : non portés.
- **Énergie (provisoire)** : échelle de la barre (2 px par point), lieu de reprise
  (entrée du hall), commandes bloquées pendant la mort : non relevés sur le MSX2. Les vies (`P-02`), les cœurs et le score du HUD restent figés.
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
Après le remplissage de SGDK (`sizebnd`, multiple de 128 Ko), `tools/md_rom_header.py`
écrit dans l'en-tête l'adresse de fin de ROM et la somme de contrôle standard (somme
des mots à partir de 0x200), à la place du XOR de SGDK. L'en-tête (`sgdk/src/rom_header.c`)
commence par « SEGA MEGA DRIVE », et le démarrage de SGDK écrit « SEGA » en `A14000`
sur les consoles à TMSS.
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
| `tools/build_msx_sound_md.py` | `generated/msx_sound_md.c/.h` (tous les effets et musiques, pilote son MSX émulé) et `research/audio/msx_sounds.json` | ROM via `VK_ROM_PATH` ou en argument |
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
