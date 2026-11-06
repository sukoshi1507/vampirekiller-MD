# Castlevania : comparaison avec le portage Mega Drive vampirekiller-MD

Source analysée : `/home/sukoshi/Documents/githubsuko/castlevania-md` (lecture seule). Ce dépôt est un portage Mega Drive (SGDK, C) réalisé à partir de la ROM NES *Castlevania (USA) Rev 1*. La ROM NES utilisée pour les vérifications est `/home/sukoshi/Documents/githubsuko/Castlevania (USA) (Rev 1).nes` (copie de travail dans le scratchpad, non modifiée).

Aucune modification n'a été faite dans `castlevania-md`, ni dans `sgdk/src` ou `validation/` de ce dépôt. Les intermédiaires sont dans `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/castlevania-nes/`.

---

## 1. Inventaire

| Élément | Constat | Statut |
| --- | --- | --- |
| Langage et build | C, SGDK (`Makefile` : `include $(GDK)/makefile.gen`), scripts Python 3 pour générer les données | prouvé |
| Modules | `src/main.c` (logique du jeu), `src/level.c`, `src/block2..7.c`, `src/maps.c`, `src/sound.c`, `src/fm_music.c`, `src/music.c`, `src/fm_music_data.inc`, `src/ym2612_music_data.inc`, `src/miku_*.inc` | prouvé |
| Données de niveaux | `level_map[480*30]`, `level_solid[]`, `level_wall[]` (bits par colonne de tuiles) dans `src/level.c` et `inc/level.h` | prouvé |
| Outils | `tools/*.py` (extraction ROM NES, capture, génération C) ; `tests/*.py` | prouvé |
| Pistes musicales | 15 pistes en FM/PSG (`vgm/*.vgm`, `partitions/*.csv|mid`) ; trois modes de lecture dans le code (ORIGINAL, FM, YM2612) | prouvé |
| Script `build_fm_soundtrack.py` | cité dans l'en-tête de `src/fm_music_data.inc` mais absent de `tools/` : la génération de ces données n'est pas reproductible à partir du dépôt | prouvé |
| Licence | aucun fichier de licence trouvé dans le dépôt | prouvé |
| Points d'entrée | `src/main.c` (boucle de jeu), `src/sound.c` (`sound_init`, `music_play`, `sfx_play`) | prouvé |

Le dépôt comporte aussi un moteur de cartes du château (`src/maps.c`) et un écran d'options. Ils ne concernent pas Vampire Killer.

---

## 2. Musique

### 2.1 Ce qui existe déjà au format Mega Drive

Trois ensembles de données coexistent :

1. **Mode ORIGINAL** : `src/music.c` contient, pour chaque piste, le flux des écritures APU de la ROM NES (un tableau d'octets : paires `registre, valeur`, séparateur `$FF` par trame, `$FE n` pour n trames sans écriture). `src/sound.c` rejoue ce flux sur le SN76489 (PSG) en modélisant les balayages, compteurs de longueur et enveloppes. Généré par `tools/sound_to_md.py` (qui appelle un émulateur 6502 `py65` pour exécuter le moteur son de la ROM). Les tableaux sont listés dans `inc/music.h` (`MUS_PROLOGUE` à `MUS_GAME_OVER`, index 0 à 14). **prouvé** (lecture du code).
2. **Mode FM** : `src/fm_music_data.inc` (`fm_tracks[15]`), joué par `src/fm_music.c`. Format : flux de commandes de type VGM (`0x50` PSG, `0x52/0x53` YM2612, `0x62` une trame), avec un bloc d'initialisation `fm_init_*` et une position de boucle. **prouvé** (lecture de `commands()` dans `src/fm_music.c`).
3. **Mode YM2612** : `src/ym2612_music_data.inc` (`ym_tracks[15]`), même format. **prouvé**.

Les exports `vgm/01..15-*.vgm` correspondent au mode FM (écritures `0x52` YM2612 et `0x50` SN76489 présentes, en-tête VGM 1.50 avec horloge YM2612 à l'offset `0x2C` = 7 670 454 Hz et horloge SN76489 à `0x0C` = 3 579 545 Hz).

Résultats de lecture des VGM (`scratchpad/.../pistes-md/stats.txt`, script `tools/vgm_stat.py`) :

| Piste | Durée (s) | Écritures YM | Écritures PSG | Boucle |
| --- | --- | --- | --- | --- |
| 01 Prologue | 6,4 | 470 | 217 | non |
| 02 Vampire Killer | 29,9 | 2 793 | 944 | oui |
| 03 Stalker | 51,2 | 5 030 | 2 215 | oui |
| 04 Wicked Child | 68,2 | 8 216 | 3 296 | oui |
| 05 Out of Time | 41,8 | 4 380 | 1 718 | oui |
| 06 Heart of Fire | 89,6 | 11 748 | 3 379 | oui |
| 07 Walking on the Edge | 38,4 | 4 858 | 1 838 | oui |
| 08 Nothing to Lose | 44,8 | 4 502 | 926 | oui |
| 09 Poison Mind | 7,5 | 843 | 79 | oui |
| 10 Black Night | 17,6 | 2 691 | 635 | oui |
| 11 Voyager | 52,1 | 2 210 | 1 115 | non |
| 12 Stage Clear | 4,1 | 622 | 221 | non |
| 13 All Clear | 4,1 | 692 | 192 | non |
| 14 Lose Life | 1,6 | 284 | 11 | non |
| 15 Game Over | 4,6 | 614 | 206 | non |

La détection de boucle est approximative (position de l'offset d'en-tête, pas de parcours exact de la commande de saut). Elle n'est indiquée qu'à titre indicatif.

### 2.2 Recapture et conversion : non faites

La correction reçue indique que la musique MD existe déjà : aucune reconversion depuis l'APU n'a été faite. La recapture des écritures APU (`scratchpad/.../music_apu.json`, 15 pistes, 386 à 5 377 trames) a seulement servi à vérifier que le moteur ORIGINAL est reproductible. Elle n'a pas été comparée octet par octet à `src/music.c`. **inconnu** pour ce point précis.

### 2.3 Pour Vampire Killer

- Les fichiers `vgm/*.vgm` sont directement utilisables comme **modèle** de format (YM2612 + SN76489, VGM 1.50), ou pour une écoute de contrôle dans un lecteur VGM. Copie de travail : `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/castlevania-nes/pistes-md/`.
- Le format `fm_tracks` (init + données + boucle, commandes 0x50/0x52/0x53/0x62) est reprenable tel quel par un lecteur Z80 ou C, sans convertir l'APU.
- Le mode ORIGINAL (APU rejoué sur le PSG) ne convient que si la musique de Vampire Killer est écrite comme des registres de générateur de bruit et de tons à partir d'un moteur NES : ce n'est pas le cas du MSX2.
- **Écoute** : je ne peux pas écouter les pistes. Les jugements de rendu (équilibre, justesse des timbres) restent à faire à l'oreille.

---

## 3. Palettes

### 3.1 Format

- Les palettes MD de `src/block3.c`, `block4.c`, `block5.c`, `block7.c` et `maps.c` sont des `u16` au format CRAM `0000 BBB0 GGG0 RRR0`. Sur les 704 mots `u16` de palettes MD examinés, aucun n'a de bit hors de ce format (masque `0xF111`). **prouvé**.
- Chaque bloc a deux jeux : `*_palette` (couleurs « 16 bits », dérivées du portage SNES) et `*_palette_nes` avec `*_palette_nesidx` (index dans la palette NES de 64 entrées). **prouvé** (lecture des tableaux).

### 3.2 Comparaison NES vers CRAM

Script : `scratchpad/.../tools/palette_check.py`, résultat : `scratchpad/.../palettes/palette_check.txt`.

- Sur 116 entrées de `*_palette_nes` comparées à la table RGB NES standard (`NES_RGB` de `tools/nes_dump.py`, quantifiée sur 3 bits par deux méthodes), **43 correspondent** et **73 diffèrent**.
- Les écarts ne prouvent pas une erreur du portage : la table NES utilisée par le projet peut être une autre palette (le projet propose 14 palettes au choix), et la quantification exacte n'est pas connue.
- **inconnu** : la règle exacte utilisée pour produire `*_palette_nes`.

### 3.3 Couleurs non représentables

La CRAM Mega Drive donne 3 bits par composante, soit 8 niveaux par canal (valeurs 0, 2, 4 ... 14 dans le mot). Une couleur RGB 8 bits est donc arrondie au niveau le plus proche : les nuances NES intermédiaires sont perdues. Le projet répartit les tuiles sur quatre palettes (deux fond, deux sprites, `docs/graphics-art.md`). Pour Vampire Killer, il faut garder la même quantification sur les couleurs MSX, pas la table NES.

---

## 4. Logique de jeu : comparaison avec vampirekiller-MD

Références : `research/player-format.md`, `research/room-format.md`, `research/enemies-platforms.md` (vampirekiller-MD). Le portage vampirekiller-MD suit le Vampire Killer MSX2 (routines Z80 aux adresses `6C2F`, `6D95`, `7B88`, etc.). Castlevania-md suit la ROM NES (CPU 6502).

| Sujet | vampirekiller-MD (MSX2) | castlevania-md (NES) | Transposable ? |
| --- | --- | --- | --- |
| Marche | 0200 en 8.8, soit 2 px/tick (`6C2F`) | 1 px/frame (`jump_vx = ±1`, `src/main.c` ~4484) | **non directement** : vitesse à vérifier pour Vampire Killer |
| Saut | 19 valeurs (`6D95`) : `-6,-6,-5,-5,0,-4,-3,-2,-1,0,0,1,2,3,4,5,5,6,6` | table `simon_jump_dy[42]` en px entiers (`src/level.c` l. 2389) : pic -36 px | **non** : tables différentes, unités à confirmer |
| Chute | 2,4,6,6 (`6F81`) | `FALL_VY = 8` px/frame (`src/main.c` l. 35) | **non** |
| Gravité ennemis | non chiffrée dans ce qui est lu | `GRAVITY 0x21` (8.8), `DEATH_GRAVITY 6` | **à vérifier** pour les ennemis MSX |
| Collisions | carte 32 x 24 octets à `D100`, index `(((y-16)&255)&248)*4+(x>>3)`, tuile > limite = solide | `level_solid[col]` / `level_wall[col]` : un bit par tuile de la colonne, 30 lignes | **concept transposable** (surface vs mur), format différent |
| Fouet | hitbox et déclenchement décrits dans `research/whip-hitbox.md` | `whip_*`, `whip_level`, `ST_WHIP` dans `src/main.c` (niveaux non détaillés ici) | **à comparer** avec `whip-hitbox.md` |
| Escaliers | non traités dans `player-format.md` (« ne traite pas les escaliers ») | `stair_id`, `stair_pos` en demi-pixels, `stair_dir`, `CLIMB_STEPS 95` | **oui, comme référence de logique** : la mécanique est documentée dans le code NES |
| Ennemis | zombies, poissons, etc. (`enemies-platforms.md`, routines Z80 tracées) | nombreux ennemis codés en vitesses 8.8 (`ZOMBIE_SCORE`, `FRANK_V`, `SKEL_JUMP_VX`...) | **valeurs de référence**, à recouper avec le MSX (les ennemis sont différents) |
| Sons | `sound-triggers.md` (fouet, musique) | `SFX_*` de `inc/music.h` | **non** : pas de correspondance d'identifiants |

Ce qui est transposable en priorité :

1. **Format de collision par colonne** : le principe « une surface posée sur un bit par tuile » est plus simple à relire que l'index à 8 bits de vampirekiller-MD. Il ne faut pas copier la structure, mais tester la règle de sol et de mur.
2. **Constantes de chute et de gravité** : valeurs NES en 8.8 (`0x21`, `0x20`), à comparer aux mesures MSX avant tout emploi.
3. **Logique d'escalier** : le modèle `stair_pos` en demi-pixels est une référence complète, alors que vampirekiller-MD ne la couvre pas.

Ce qui n'est pas transposable : les tables de saut (unités et tailles différentes), les adresses et routines Z80 (MSX), la carte (format 32 x 24 contre 480 x 30).

---

## 5. Prouvé / déduit / inconnu

**Prouvé**
- Structure, langage, build SGDK, modules et points d'entrée de castlevania-md (lecture des fichiers).
- Trois modes musicaux (ORIGINAL, FM, YM2612) et leurs structures de données (code et en-têtes VGM).
- 15 exports VGM 1.50 présents dans `vgm/`, avec écritures YM2612 et PSG (script de lecture).
- Format CRAM `0000 BBB0 GGG0 RRR0` de toutes les palettes MD examinées (704 mots, 0 hors format).
- Tables de saut et de chute de castlevania-md (`simon_jump_dy`, `FALL_VY`) et de vampirekiller-MD (`6D95`, `6F81`).

**Déduit**
- Le mode ORIGINAL de castlevania-md rejoue fidèlement le moteur son NES (inféré des commentaires de `sound_to_md.py` et de `sound.c` ; non vérifié octet par octet).
- Les 73 écarts de palette viennent d'une palette NES différente ou d'une quantification différente (hypothèse, non testée).

**Inconnu**
- Écoute des pistes : impossible ici. Le rendu sonore n'est pas jugé.
- Script `tools/build_fm_soundtrack.py` absent : impossible de régénérer `fm_tracks` / `ym_tracks` depuis le dépôt.
- Unité exacte de la table de saut `6D95` de vampirekiller-MD (px/frame ou 8.8).
- Correspondance exacte entre `vgm/*.vgm` et `fm_tracks` (non recoupée).
- Règle de génération de `*_palette_nes` (table NES utilisée).

---

## 6. Marche à suivre pour la musique

1. Ne pas reconvertir l'APU : le modèle à suivre est le format `fm_tracks` (init + données + boucle, commandes 0x50/0x52/0x53/0x62).
2. Pour Vampire Killer, produire les données dans le même format (VGM ou équivalent), puis les intégrer dans le lecteur C ou Z80 de vampirekiller-MD. Vérifier le nombre de canaux et les horloges (YM2612 7 670 454 Hz, SN76489 3 579 545 Hz) dans l'en-tête.
3. Contrôler chaque piste à l'oreille (non fait ici) et comparer les durées et boucles aux valeurs ci-dessus.
4. Pour un contrôle hors dépôt, `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/castlevania-nes/pistes-md/*.vgm` peut être lu dans un lecteur VGM.

---

## 7. Fichiers produits (scratchpad)

Dossier : `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/castlevania-nes/`

- `pistes-md/*.vgm` : copies des 15 VGM de castlevania-md (non reconverties).
- `pistes-md/stats.txt` : statistiques de lecture des VGM.
- `pistes-md/music_orig_apu.c.copy` : copie de `src/music.c` (flux APU du mode ORIGINAL).
- `music_apu.json` : écritures APU par trame des 15 pistes, recapturées pour contrôle.
- `palettes/palette_check.txt` : résultat de la comparaison NES / CRAM.
- `tools/` : `record_apu.py`, `vgm_stat.py`, `palette_check.py` (copies de travail, aucune écriture dans les dépôts).
