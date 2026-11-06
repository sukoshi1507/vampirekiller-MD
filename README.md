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
  **Le fouet tue les ennemis de types 1 et 4** (type 1 confirmé à l'émulateur ; type 4 :
  boîte `84A7` de la ROM, du côté où Simon regarde, debout et accroupi, vérifiée par trace
  openMSX au stage 2 et dans la salle 4 du hall). Le fouet à chaîne (`C416` = 1) porte à
  ± 24 et élargit la boîte de 4 px pour tous les ennemis, comme `84A7`/`84D9`.
- **Clés blanches et portes** : la clé blanche (bloc caché de la salle 2 du hall, de la
  salle 5 du stage 2) se ramasse au contact et s'affiche dans le HUD ; la porte du stage
  (salle 7 du hall, salle 1 du stage 2) bloque Simon sans elle, s'ouvre avec (barreau
  qui se lève, Simon qui sort seul) et mène au stage suivant (trace openMSX pour les
  deux stages, `research/door-room7.md` 16 et 18). Au stage 2, le bloc se touche par un
  saut depuis le bloc immergé : comme sur MSX2, le saut n'a pas de plafond en l'air.
  Sauts sous les plafonds et rebords du hall et du stage 2 comparés à openMSX image par
  image (`research/jump-compare.md` §8) : départ bloqué par la sonde `7B78`, et pose sur un
  rebord atteint au sommet dès le pas 9 (`6D13`, `Y &= F8`), comme sur MSX2.
- **Stages 1 à 10 (parcours)** : 73 salles avec les cartes complètes (routine `4F83`), les
  liens de la ROM (`B99A`), la clé et la porte de chaque stage, les listes d'ennemis fixes.
  La porte mène au stage suivant ; la musique ne redémarre qu'après un stage de boss
  (`794F`, table `7777`). Après la porte du stage 10, retour au titre (pas de fin portée).
  Les stages 3 à 10 ne sont vérifiés que contre la ROM et par tests natifs, pas encore en
  émulation ni comparés à openMSX (`research/stages-1-10/`).
- **Bougies, clé dorée et coffres** (cour, hall, stage 2) : les objets lâchés et les
  objets fixes se ramassent au contact, aussi pendant leur chute ; une seule clé dorée
  à la fois (icône du HUD, gardée jusqu'au retour en jeu après une mort), qui ouvre un coffre dont le contenu attend 32 images puis
  tombe. Effets portés (`research/castle-candles.md` 9.5 à 9.8) : cœurs (01, 02) ;
  énergie (07 : +8, 16 : pleine) ; 09, contact destructeur 300 images (l'ennemi touché
  meurt et rapporte ses points, Simon clignote en rouge) ; 08, aucun contact 300 images
  (Simon clignote en blanc) ; 06 et les objets d'arme (1A : fouet à chaîne ; 1B, 1C, 1D : dague, hache, croix
  lancées, `research/secondary-weapons.md`) arrêtent les
  apparitions 300 images ; 05 détruit les ennemis présents, le fond s'éclaire en blanc
  6 fois ; 03, armure : dégâts de face divisés par deux pendant 16 coups ; 0B, 0C, 0D
  (bits de `C431`) : minuteries portées à 480 images, recul rapide de 2,5 px, saut
  haut. Minuteries, armure et bits de `C431` sont remis à 0 à la fin d'une mort. Au
  changement de stage, le compteur d'armure (`C441`) passe à 0 mais l'armure reste : le
  coup de face suivant le fait boucler à FFh, comme sur MSX2 (trace openMSX) ; l'énergie
  est gardée.
- **Stage 2 (jouable)** : six salles avec les collisions et les liens de la ROM
  (`tools/build_stage_maps_md.py`) ; arrivée en salle 0 à x 16, y 96, comme sur le MSX2.
  Tous les ennemis du MSX2 y sont portés : chauves-souris (type 4, salles 0, 1, 3),
  zombie (type 1, salle 2), hommes-poissons et leurs gouttes (type 2, salles 4, 5)
  (inventaire : `research/enemy-behavior.md`, « Inventaire du stage 2 »). Après une
  mort, Simon reprend en salle 0 à x 16, y 96, regard à droite (table `641F`, trace).
- **Énergie, vies, mort, game over** : la barre PLAYER du HUD suit l'énergie (32 points).
  À 0, Simon meurt ; 252 images plus tard l'énergie revient au maximum et, 27 images
  après, il perd une vie et reprend. Sans vie, « GAME OVER » puis le titre (378 images).
  Délais et compteur de vies (`C410`, 2 en jeu, 3 morts au total) mesurés sur le MSX2
  (`research/player-health.md`). L'écran de fin de partie reprend celui du MSX2
  (écran effacé, « GAME  OVER » à la police MSX).
- **HUD** : la ligne du haut est celle du MSX2, à la police de la ROM : score (100 points
  par zombie, chauve-souris ou panthère, 200 par homme-poisson, table `81CE`), stage
  (00 dans la cour), cœurs et vies ; « PLAYER » et « ENEMY » à leurs positions MSX.
- **Pause** : START met le jeu en pause avec le son du MSX2 (silence, jingle, puis
  reprise de la musique où elle était) ; A+START retourne au titre (raccourci de debug).
- **Raccourcis de debug** : C au titre lance une partie au stage 2 ; en jeu, A+GAUCHE ou
  A+DROITE change de salle dans le stage courant ; le code Konami ouvre le test du son au titre,
  et, en pause, le choix du stage et de la salle ainsi que le god mode (A le coupe ou le remet).
- **Son** : musiques et effets MSX2 émulés depuis le pilote de la ROM sur le PSG
  (`research/audio-driver.md`). Ajout du port : le cri de Wilhelm (CC0) quand Simon
  tombe dans un trou ou dans l'eau.
- **Objet 0A, arrêt du temps** (arme secondaire du MSX2) : une fois ramassé, BAS pendant
  un saut coûte 5 cœurs et fige les ennemis 178 images (298 avec l'objet 0B), avec la
  boucle sonore FB du MSX2 et la musique gelée ; durée et gel vérifiés par une trace
  openMSX (`research/castle-candles.md` 9.9).
- **Case d'objets du HUD et bouclier** : 0F, 0A, 03/04 et 1E s'affichent dans la case
  du HUD comme sur MSX2 (objet ramassé montré seul 64 ou 66 images, puis case
  redessinée, cadence mesurée par une trace openMSX) ; avec l'armure 03 (ou 04), Simon
  prend les poses au bouclier de la ROM ; case comparée à une capture openMSX
  (`research/castle-candles.md` 9.8).
- **Calendrier d'affichage** : comme sur le MSX2, Simon, son fouet et les ennemis
  (sprites MSX) apparaissent deux images après le calcul de la passe, le décor, le HUD,
  la couleur de fond et les sons une image après (trace openMSX et harnais MD,
  `research/frame-timing.md`) ; les tuiles du décor (torche éteinte, bloc cassé, clé) passent
  par la file DMA et changent une image après, comme sur le MSX2. Restent : sons demandés
  en fin de passe MSX une image trop tôt. La manette est lue à chaque image (MSX : une
  sur deux, un appui d'une image y est perdu une fois sur deux) : écart gardé, la logique du
  port avançant à chaque image (`research/frame-timing.md`, section 4).
- **Ajout du port, saut depuis l'escalier** : sur une marche, C fait sauter Simon hors de
  la volée. Le MSX2 ne le permet pas (HAUT, sa touche de saut, y fait monter) ; ce saut
  est gardé comme un plus de jouabilité (`research/stairs.md`).

Ce qui n'est pas porté ou reste provisoire :

- **Type 5** : l'objet de mort (`1E`) n'est pas reproduit ; la phase 3 du fouet
  est assimilée au fouet déployé (`vk_whip_phase` 2) et le recul face à une panthère
  assise est provisoire.
- **Stage 2** : apparition du premier zombie de la salle 2 décalée de 4 images (phase de
  `CF10`) ; les liens haut/bas viennent de la table de la ROM, sans trace en jeu.
- **Objets, non portés** : l'objet 04 (destruction des
  projectiles ennemis, absents des stages 0-2) et les autres contenus (0E, 12, points
  13/14). Armes lancées : pose accroupie non dessinée à part.
  Les effets portés viennent de la lecture du code ; le clignotement de Simon (08, 09 et
  après un coup) est calé image par image sur une trace openMSX, les autres minuteries
  sans trace dédiée (`research/castle-candles.md` 9.8).
- **Carte du stage (objet 0F)** (`research/stage-map.md`) : A pressé seul, 3 usages. Dessinée d'un
  coup, sans les étapes de l'automate `CF38` ; START ne fait rien tant qu'elle est ouverte.
- **Boss du stage 3** (`research/stage3-boss.md`) : porté, de la porte de la salle 4 à la sortie vers
  le stage 4 (vol, boule de feu, barre ENEMY, orbe, soin).
- **Stage 4** (`research/stage4-enemies.md`) : bougies et blocs de toutes les salles, objets 06 et 08. Pas encore
  de boss ni de comparaison complète de chaque salle.
- **Stage 5** (`research/stage5-enemies.md`) : bougies et blocs de toutes les salles, objets 06 et 08, plateformes mobiles des salles 1 et 4. Clé, porte et sortie pas encore rejouées.
- **Stage 6** (`research/stage6-enemies.md`) : bougies, blocs, clés dorées et coffres de toutes les salles, chauve-souris (objet 07), objet 08, statue qui tire des boules (objet 0A) et pics du plafond de la salle 1. Le boss de la salle 5 est dans `research/stage6-boss.md`.
- **Boss du stage 6** (`research/stage6-boss.md`) : porté, de la clé blanche et de la porte de la salle 4 à l'orbe et au passage au stage 7 (objet 13h passe par passe contre la trace, boules, barre ENEMY, orbe). Le stage 7 qui suit est porté (voir plus bas).
- **Stage 7** (`research/stage7-enemies.md`) : hopper/squelette 0B, leaper/créature volante 0C et os (code partagé avec le stage 8), puce (0D), bougies, blocs, clés, coffres, murs, clé blanche, porte de la salle 6 et sortie vers le stage 8 ; pas de boss. Les routines de la ROM, exécutées dans un émulateur Z80, reproduisent la trace et le port les reproduit passe par passe (`tests/test_stage7.c`, scénario `zza_stage7`).
- **Stage 8** (`research/stage8-enemies.md`) : bougies, blocs, clés dorées, coffres et murs de toutes les salles, hopper 0B qui lance des os, leaper 0C, objet 08 et statue 0A, clé blanche, porte et sortie vers le stage 9 (objets comparés passe par passe à quatre traces openMSX, 0 écart). Pas de boss ; le stage 9 se charge sans ennemis. Le générateur de bougies produit aussi, sans comportement, les objets du stage 7.
- **Marchand et objets muraux** (`research/merchant.md`) : portés, B = oui, C = non. 10 et 11
  changent les prix sans détruire les ennemis (trace openMSX). Le hasard du registre R est
  remplacé par un compteur d'images (provisoire).
- **Stages 3 à 10** : ni boss, ni le fondu `7763` avant leur porte, ni les ennemis de type
  6, 10, 11, 12, 13, 31 et 33 et des canaux 04, 10 et 20 ; objets limités aux clés.
- **Stages 11 et suivants, châteaux** : non portés (dont le
  passage secret du stage 12, accroupi puis HAUT, `C420` = 7, `research/enemy-behavior.md`).
- **Énergie (provisoire)** : commandes bloquées pendant la mort : non relevées sur le
  MSX2. Le lieu de reprise suit la table `641F` (salle 0 ; cour (16,176), hall (16,192),
  stage 2 (16,96)), tracé au stage 2 (`research/player-health.md`). La chute dans le vide tue
  (`vk_health_fall`), avec le cri de Wilhelm ajouté au port.
- **Provisoires** : vitesse de chute des ennemis. La boîte de contact de Simon suit la
  ROM pour les types 1, 2, 4 et 5, debout, accroupi, en saut et sur l'escalier
  (`research/enemy-behavior.md`, « Contact selon la posture de Simon »).

Le détail par sujet est dans [research/INDEX.md](research/INDEX.md).

## Contrôles (manette 3 boutons)

| Touche | En jeu | Touche MSX2 |
|---|---|---|
| Croix gauche/droite | marche | curseurs |
| HAUT / BAS | escaliers ; BAS au sol : accroupi | HAUT / BAS |
| C | saut | HAUT (saut) |
| B | fouet, ou lancer de l'arme (dague 1B, hache 1C, croix 1D) | ESPACE |
| GAUCHE / DROITE pendant un saut | eau bénite (objet 1E, 5 cœurs) | GAUCHE / DROITE pendant un saut (`714D`) |
| BAS pendant un saut | arrêt du temps (objet 0A, 5 cœurs) | BAS pendant un saut (`C006` bit 1, `C420` = 1, `7136`-`716F`) |
| START | pause | F1 (pause) |

L'eau bénite garde aussi la touche du MSX2 : GAUCHE ou DROITE nouvellement pressée en
l'air, avec 5 cœurs (`research/secondary-weapons.md`).
L'arrêt du temps garde la touche du MSX2 : BAS nouvellement pressé en l'air, hors de
l'image d'un coup de fouet. En l'air, BAS ne sert à rien d'autre (l'accroupi n'existe
qu'au sol), et aucun bouton A n'entre en jeu : les raccourcis de debug restent libres.

Debug : A+START retourne au titre, A+GAUCHE / A+DROITE change de salle, A+B rallume les
bougies de la salle ; au titre, C lance une partie au stage 2 et A ouvre l'explorateur
de salles (B y lance la partie dans la salle affichée) ; le code Konami ouvre le test du son
au titre, et en pause le choix du stage et de la salle avec le god mode (A le coupe ou le remet).

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
et les ennemis, ainsi que les sorties, les transitions et les ennemis des stages 1 à 10.
Ils doivent afficher PASS.

Tests Python (certains lisent la ROM de référence) :

```sh
scratchpad/venv/bin/python -m unittest discover -s tests -p 'test_*.py'
```

Contrôle des cartes des stages 10 à 12 contre la ROM, indépendant des générateurs : la
routine Z80 originale `4F83` reconstruit chaque salle, comparée octet par octet aux tableaux C,
et les liens sont comparés à la table ROM `B9D3` (`tests/test_stages_10_12_rom.py` le lance aussi) :

```sh
scratchpad/venv/bin/python tools/verify_stages_10_12_maps.py
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

Les journaux des versions précédentes (anciens `V*.md` à la racine) ont été retirés ;
ils restent dans l'historique git.

## Licence

Le code écrit pour ce portage est sous **GPL-3.0-only** (fichier `LICENSE`).

Ce qui n'est pas couvert par cette licence :

- **Données de Konami** : graphismes, cartes, sprites, enregistrements audio
  extraits de la ROM (dans `assets/`, `sgdk/src/generated/`, `research/`, `validation/`).
  Elles restent sous copyright Konami. La ROM elle-même n'est pas incluse.
- **Sons du fouet et de coup** : dérivés de castlevania-md, qui ne publie pas de licence.
- **SGDK** : licence MIT (`third_party/SGDK-LICENSE.txt`).

Détail et points à trancher : `research/licensing.md`.
