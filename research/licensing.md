# Inventaire des sources tierces et des licences (vampirekiller-MD)

Objet : lister ce que le dépôt tire de sources qui ne sont pas l'auteur du portage, avec la licence connue et ce qu'elle impose, pour choisir la licence du dépôt en connaissance de cause.

Ce document est un inventaire technique, pas un avis juridique. Les points marqués « à vérifier » demandent une confirmation de l'auteur de la source ou un juriste.

Référence : `git ls-files` au commit `56ec273` (branche `main`). Le fichier `Vampire Killer (Japan, Europe) (En).zip` est à la racine, **non versionné** (ignoré par `.gitignore`, `/Vampire Killer*.zip`). SHA-256 attendu : `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`.

## Constats clés

1. **Aucun fichier LICENSE** dans vampirekiller-MD. Le README ne donne aucune licence. Seul `third_party/SGDK-LICENSE.txt` (MIT) existe. Sans licence, le droit d'auteur s'applique par défaut : personne n'a le droit de copier, modifier ou redistribuer le dépôt.
2. **Données de la ROM Konami versionnées** : graphismes, palettes, cartes de salles, sprites, séquences audio (traces PSG) et extraits de désassemblage Z80 sont dans le dépôt. Ce sont des éléments extraits d'un jeu sous copyright Konami. Le code de conversion est écrit par l'auteur, mais les données qu'il produit restent celles du jeu.
3. **Une donnée vient de castlevania-md** : le son du fouet (`SFX_0A`, 50 octets) dans `sgdk/src/generated/sfx_md.c`, généré par `tools/build_sfx_md.py` qui lit `../castlevania-md/src/music.c`. castlevania-md n'a **aucune licence** (voir plus bas), donc aucune licence ne peut être héritée de là.
4. **Le copyleft annoncé pour castlevania-md n'est pas trouvé dans le dépôt** : ni fichier LICENSE, ni mention GPL/copyleft, dans l'arbre courant ni dans l'historique (`git log --all`). Le commentaire « Copyleft » dans `sfx_md.c` et `build_sfx_md.py` vient de l'auteur et n'est pas étayé par la source.
5. **Code d'un partenaire** (`research/partner-baseline/`, `MERGE_PARTENAIRE.md`) : pas de licence connue.

## Tableau source / chemin / licence / implication

| # | Source | Chemin dans le dépôt | Licence connue | Implication |
|---|---|---|---|---|
| 1 | ROM MSX2 Konami (Vampire Killer) | `Vampire Killer (Japan, Europe) (En).zip` (racine, non versionné) | Copyright Konami ; aucune licence libre | Non versionné : le dépôt ne redistribue pas la ROM. L'utilisateur doit la fournir pour reconstruire les données. |
| 2 | Graphismes extraits de la ROM (tuiles, palettes, décors, sprites Simon, flammes, logo) | `assets/**` (PNG, `manifest.json`, `intro/konami.png`, `intro/title.png`, `player/`, `flame_fx/`, `stage-overview.png`) ; `research/simon-assets/**` ; `research/simon-frame-audit/**` | Copyright Konami (artwork) | Données du jeu, pas de l'auteur. Une licence copyleft sur le code ne peut pas les relicencier. Rendre le dépôt public avec ces PNG = redistribution de contenu Konami. |
| 3 | Données générées depuis la ROM (C versionné) | `sgdk/src/generated/` : `rooms.c/.h`, `stage1_collision.c/.h`, `player_assets.c/.h`, `intro_assets.c/.h`, `simon_attack_md.c/.h`, `simon_crouch_md.c/.h`, `whip_level1_md.c/.h`, `whip_level2_md.c/.h`, `attack-*.png`, `crouch-candidate-*.png`, `simon_crouch_preview.png`, `flame_fx_md.h`, `flame_fx_manifest.json`, `*_manifest.json` | Copyright Konami (contenu tuiles/cartes/animations) | Idem #2 : c'est la mise en forme C de données Konami. Le fichier `rooms.c` est dit « Generated from the reference ROM ». |
| 4 | Binaire ROM Mega Drive embarquant les données extraites | `build/vampire-killer-md-viewer.bin` (versionné) | Mélange : code SGDK (MIT) + données Konami | Ce binaire contient des graphismes Konami. Le distribuer = distribuer des données Konami. |
| 5 | Captures de rendu de la ROM | `validation/*.png` (dont `md-konami.png`, `md-title.png`, `md-stage-*.png`, `md-castle-*.png`, `last-room.png`, `md-atlas.png`) | Copyright Konami (contenu visible) | Captures de jeu : même statut que #2. |
| 6 | Analyses et tables de la ROM | `analysis/rom.json`, `analysis/banks.csv`, `analysis/mapper_candidates.csv`, `research/room-format.md`, `research/room-links.md`, `research/stairs.md`, etc. | Données factuelles sur un jeu Konami | Les descriptions de format sont des faits, mais les tables de données extraites ne sont pas libres. |
| 7 | Désassemblage Z80 de la ROM | `research/known.asm`, `research/loading-and-states.asm`, `research/z80-vdp-io-analysis.md`, `research/castle-navigation/*.json`, `research/simon-assets/animation-selector-audit.json` | Code Z80 Konami (copyright) | Extraits de code du jeu (octets et instructions). Même réserve que #2. |
| 8 | Tuiles de flamme copiées de la ROM dans une source C | `research/flame_v15_source.txt` | Copyright Konami (tuiles) | Données du jeu sous forme C. |
| 9 | Traces audio du pilote Konami (PSG AY/MSX) | `research/audio/psg_ay_msx_long1_150s.vgm`, `research/audio/long1-*.csv`, `research/audio/scan-row8-game-state.csv`, `research/audio/triggers.json`, `research/audio/manifest.json` | Musique et sons du jeu (copyright Konami) | Traces brutes de la musique du jeu. |
| 10 | Conversions audio dérivées de #9 | `research/audio/psg_sn76489_candidate.{vgm,xgm}`, `research/audio/conversion-report.json`, `sgdk/src/generated/music_md.c` (21 004 octets, `vk_music_test`) | Dérivé de musique Konami | Données dérivées : même statut que #9. Le script `tools/convert_psg_vgm.py` est un code de l'auteur. |
| 11 | Son du fouet, copié de castlevania-md | `sgdk/src/generated/sfx_md.c`, `sgdk/src/generated/sfx_md.h` (`vk_sfx_whip[50]`) | castlevania-md : aucune licence. Données du jeu NES Castlevania (Konami) | Contenu emprunté à un dépôt sans licence, lui-même issu d'un jeu Konami. Aucune licence à hériter. **À retirer, remplacer ou obtenir une autorisation écrite.** |
| 12 | Script de génération du son du fouet | `tools/build_sfx_md.py` (lit `../castlevania-md/src/music.c`) | Code de l'auteur ; dépendance à un dépôt externe sans licence | Le code est libre de choix, mais il dépend de castlevania-md à l'exécution. Le commentaire « copyleft » est non étayé. |
| 13 | Documentation de provenance du son du fouet | `research/audio/whip-sfx-castlevania.md`, `research/hit-sound.md` (§3.3), `research/mechanics-remaining.md` (ligne M30), `research/audio/whip-sfx-diff.md` | Texte de l'auteur | Le document signale lui-même : « La licence d'un portage ne couvre pas forcément les données du jeu d'origine ». |
| 14 | Analyse comparative avec castlevania-md | `research/castlevania-nes-comparison.md` | Texte de l'auteur, faits sur castlevania-md | Pas de code copié. Citer un dépôt sans licence reste une référence. Pas de problème particulier. |
| 15 | SGDK (bibliothèque et build Mega Drive) | `third_party/SGDK-LICENSE.txt` (seul fichier) ; `sgdk/Makefile` (suppose `GDK` externe) | **MIT**, Copyright (c) 2025 Stephane Dallongeville | Compatible avec un copyleft. Il faut conserver l'avis MIT dans les copies et parties substantielles. SGDK n'est pas versionné : l'utilisateur l'installe à part. |
| 16 | Code de démarrage dérivé de SGDK | `sgdk/legacy/boot/sega.s` (notes « Modified by Stephane Dallongeville », Charles Coty, Paul W. Lee) ; `sgdk/legacy/boot/rom_head.c` ; `sgdk/src/rom_header.c` | Code issu de SGDK (MIT) ; l'en-tête Sozobon/Paul W. Lee est une source ancienne (licence à vérifier) | Garder les en-têtes d'origine. Vérifier la licence de la source Sozobon / Paul W. Lee avant publication. |
| 17 | Code du partenaire (baseline) | `research/partner-baseline/` (`Makefile.original`, `build_original.py`, `verify_baseline_original.py`, `BUILDING.md`, `PROJECT_STATUS.md`, `requirements-validation.txt`), `MERGE_PARTENAIRE.md` | **Inconnue** (projet `vampirekiller2.zip` du partenaire) | Impossible de publier sans accord. Demander la licence au partenaire ou retirer ces fichiers. |
| 18 | Dépendances Python | `requirements.txt` : `Pillow>=10`, `z80dis==1.0.6`, `z80==1.2.0` (idem dans `research/partner-baseline/requirements-validation.txt`) | Pillow : HPND (à vérifier) ; `z80`, `z80dis` : licences à vérifier | Non redistribuées par le dépôt (installées par pip). Vérifier leur licence avant de citer la chaîne d'outils. |
| 19 | Outils externes non versionnés | Scripts `tools/*.tcl`, `tools/run_*.py`, `research/audio/run_psg_openmsx.sh` (openMSX) ; émulateurs BlastEm et Genesis Plus GX (cités dans les docs) ; `xgm2tool.jar` (SGDK, cité dans `research/audio/README.md`) ; chaîne GCC m68k-elf / Marsdev | openMSX : GPL (à vérifier) ; BlastEm, Genesis Plus GX : licences propres (à vérifier) ; xgm2tool : sous SGDK (MIT, à vérifier) | Ces outils ne sont pas copiés dans le dépôt : seuls les scripts qui les appellent sont versionnés. |
| 20 | Code écrit par l'auteur | `sgdk/src/*.c/.h` (`main.c`, `player.c`, `whip.c`, `enemies.c`, `targets.c`, `room_manager.c`, `input.c`, `psg_player.c`, `sound_test.c`, `gameplay.c`, etc.), `tools/*.py` (hors `convert_wilhelm`), `tests/*`, `research/*.md` (rédaction), `.claude/agents/*.md` | Libre de choix (aucune source tierce copiée, sauf comportements observés sur la ROM) | Le comportement de la ROM n'est pas protégé en tant que tel, mais le code et les données extraites sont ce qu'ils sont (voir #2 à #10). |

## Points à noter

- **Nom de la ROM** : « (En) » dans le nom de l'archive (convention No-Intro) désigne en général une traduction anglaise. À vérifier : s'il s'agit d'une traduction non officielle, elle est une œuvre dérivée d'une seconde couche de droits.
- **Wilhelm (castlevania-md, CC0)** : le cri `res/audio/wilhelm.pcm` et son JSON indiquent CC0 (Wikimedia Commons). **Il n'est pas présent dans vampirekiller-MD** : aucun impact ici.
- **Graphismes SNES Rumbleminze / Fiuke Bnuy** : `castlevania-md/tools/import_snes.py` précise que ces tuiles et palettes « n'ont aucune licence qui permettrait de les redistribuer ». **Aucun fichier de ce type n'est dans vampirekiller-MD** (pas de `import_snes`, pas de `level_tiles`). Aucun impact ici, sauf si on réutilise du code de castlevania-md.
- **ReapNES (t3dy)** : cité dans les sources de castlevania-md (`src/menu_nes_music_data.inc`). Non présent dans vampirekiller-MD.
- **Le dépôt contient déjà une réserve** : le README indique que la ROM n'est pas incluse, mais ce n'est pas suffisant tant que les données extraites sont versionnées.

## Décisions à prendre (utilisateur)

1. **Licence du code** : choisir une licence copyleft et la déposer en `LICENSE` (par ex. GPL-2.0, GPL-3.0, AGPL-3.0). Vérifier qu'elle reste compatible avec SGDK (MIT, OK) et avec les autres sources retenues (castlevania-md : à trancher, voir décision 3). Le choix ne couvre ni les données Konami (#2 à #10), ni le code du partenaire (#17), ni le son du fouet (#11).
2. **Les données Konami peuvent-elles être redistribuées ?** Dans la plupart des cas, non sans accord de Konami. Deux options :
   - a) Retirer du dépôt tout ce qui est issu de la ROM (`assets/`, `sgdk/src/generated/` de données, `validation/*.png`, `build/*.bin`, `analysis/*.json|csv`, `research/audio/*.vgm|xgm|csv`, extraits `research/*.asm`) et ne garder que les scripts et le code, qui exigent que l'utilisateur fournisse la ROM.
   - b) Garder ces données et publier le dépôt en précisant, dans le README, que ces données restent sous copyright Konami et hors de la licence du code. Ce cas reste exposé.
3. **Le son du fouet (castlevania-md)** :
   - a) Obtenir une autorisation écrite de l'auteur de castlevania-md, et lui demander d'ajouter une licence à son dépôt ;
   - b) Remplacer `sfx_md.c` par un son synthétisé par l'auteur (l'historique contient une version synthétisée, commit `7fd48f9`) ;
   - c) Retirer le son.
   Dans tous les cas, le commentaire « Copyleft » de `sfx_md.c` et de `tools/build_sfx_md.py` doit être corrigé ou justifié.
4. **Le « copyleft » de castlevania-md** : la mention vient de l'utilisateur et n'est pas étayée par le dépôt. Vérifier s'il existe une licence hors dépôt (site, release, message de l'auteur). Sans licence publiée, ce dépôt n'a rien à hériter.
5. **Code du partenaire** (`research/partner-baseline/`, `MERGE_PARTENAIRE.md`) : obtenir l'accord écrit du partenaire pour le publier sous une licence, ou retirer ces fichiers.
6. **Avis SGDK** : conserver `third_party/SGDK-LICENSE.txt` et les en-têtes d'origine de `sgdk/legacy/boot/`. Vérifier la licence de la source Sozobon / Paul W. Lee citée dans `sega.s`.
7. **Dépendances Python** (Pillow, z80, z80dis) : vérifier leur licence si le dépôt doit lister des dépendances dans une documentation publique.
8. **Ouverture du dépôt** : tant que les décisions 2 et 5 ne sont pas prises, ne pas rendre le dépôt public.
9. **Nom de la ROM « (En) »** : confirmer s'il s'agit d'une traduction non officielle.

## Méthode et limites

- Listing : `git ls-files` (441 fichiers), `git grep` sur les mots-clés licence, copyright, SGDK, castlevania, Wilhelm, Fiuke, ReapNES, SNES, Konami, MIT, CC0, Wikimedia.
- castlevania-md (lecture seule, aucune commande d'écriture) : `ls`, `git ls-files`, `git log --all --name-only` filtré sur licence/copying/notice (aucun résultat), `git grep` sur `HEAD` (aucune licence trouvée ; mentions CC0 du cri Wilhelm, et commentaire `import_snes.py`).
- Non vérifié : le contenu binaire de `build/vampire-killer-md-viewer.bin` et des PNG (vérifié par chemin et par manifeste seulement) ; les licences exactes d'openMSX, BlastEm, Genesis Plus GX, Pillow, z80, z80dis ; la licence de la source Sozobon / Paul W. Lee.
