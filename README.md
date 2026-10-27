# Vampire Killer MSX2 → Mega Drive / SGDK

Jalon 4, 26 octobre 2026 : **le demarrage enchaine le logo Konami, le titre,
la scene animee devant la grille, les trois ecrans jouables de la cour et
l'entree dans le hall du chateau.** Simon marche et saute avec les graphismes
extraits de la ROM. L'explorateur de 152 fonds reste disponible.

Les attaques, les ennemis, le son et les escaliers ne sont pas encore portes.
Les flammes de la cour sont animees : deux etats extraits de la ROM, tenus 16 ticks chacun.
La flamme A s'eteint des qu'un coup du fouet la touche (le fond cuit de la flamme est
efface dans le meme cas). Les flammes ne blessent pas Simon.
Le controleur et l'activation de la porte sont encore simplifies.

Correction du demarrage : le cadre bleu du titre et le HUD de l'intro ont
ete reconstruits en executant les routines Z80 de dessin et de palette
jusqu'aux ports VDP. Les anciennes substitutions oubliaient leurs effets
sur les registres CPU. Les references et la ROM ont ete regenerees ; des
controles specifiques protegent maintenant le bleu du titre et les palettes.

## Essayer

Ouvrir `build/vampire-killer-md-viewer.bin` dans un emulateur Mega Drive.

| Situation | Commande |
| --- | --- |
| Logo Konami | Attendre le titre ou START pour le passer |
| Titre | START lance l'entree devant le chateau |
| Scene animee de la grille | Attendre la fin ou START pour la passer |
| Cour exterieure | Gauche / droite pour marcher, C pour sauter |
| Bord d'un ecran de la cour | Continuer a marcher pour changer d'ecran |
| Porte du chateau, troisieme ecran | Se placer devant et appuyer sur HAUT |
| Partie | START revient au titre |

La cour constitue le stage 0 du jeu original, avec un depart a x=16,y=176.
Le prototype precedent commencait plus loin, dans le hall (stage 1).
Le nouveau parcours corrige cet ordre. Le HUD d'origine est conserve dans
les nouvelles scenes, avec un guide de commandes dans la marge inferieure.

Depuis le titre, A ouvre l'explorateur technique : gauche/droite change de
salle, haut/bas change de stage, A montre les tuiles et B ouvre l'ancien test
du personnage dans le hall. Dans ce test, C saute, B revient a l'explorateur
et START reinitialise Simon.

## Resultats verifies

- 153 entrees de salles decodees dans les tables des stages 1 a 18.
- Pour chacune, execution de la routine Z80 originale `4F83/4FAF` dans un harness
  CPU + mapper : les **768 octets de carte correspondent exactement** a l'extraction.
- 152 fonds convertis en ressources SGDK; 7 jeux de tuiles distincts.
- Pour les 152 fonds, comparaison pixel par pixel dans Genesis Plus GX avec la
  reconstruction attendue : **45 056 pixels identiques par fond**, soit 6 848 512 au total.
- Navigation entre salles et stages, bascule vers l'atlas et remise a zero testees.
- Compilation SGDK 2.11 / GCC m68k-elf 13.1.0 sans avertissement; ROM de 524288 octets.
- Signature SEGA, alignement et checksum au format SGDK controles.
- 53 872 comparaisons C/Z80 : classification, adressage de carte, sondes
  au sol et laterales, calcul du saut.
- Huit poses de Simon extraites des masques RLE et des tables de placement.
- Test MD : 60 pixels en 30 images a droite, 20 pixels en 10 images a gauche,
  saut et retour au sol. Cinq captures correspondent pixel par pixel aux
  masques originaux composes sur le decor.

- Sept nouvelles scenes reconstruites en executant les routines originales
  avec des adaptateurs BIOS/VDP explicites.
- 320 images de trajectoires originales pour le nuage, les chauves-souris
  et Simon dans l'intro, avec seulement 14 motifs de sprites distincts.
- Parcours complet titre → intro → cour 0 → cour 1 → cour 2 → hall verifie
  dans Genesis Plus GX; logo, titre, trois instants animes et scenes jouables
  compares pixel par pixel aux references reconstruites.

La derniere entree, stage 18 / salle 9, utilise des identifiants de tuiles
192 a 245 qui ne sont pas couverts par le chargement standard identifie.
Son image de diagnostic comporte du magenta; elle est exclue de l'explorateur.
Ce cas demande une analyse specifique. Les autres images restent des fonds
statiques sans les modifications du jeu en cours d'execution.

## Parcourir les fichiers

| Chemin | Contenu |
| --- | --- |
| `build/vampire-killer-md-viewer.bin` | ROM Mega Drive prete a essayer |
| `validation/md-stage-01-room-00.png` | Capture reelle de la ROM dans Genesis Plus GX |
| `validation/md-castle-entry.png` | Scene animee de la grille dans la ROM MD |
| `validation/md-castle-door.png` | Simon devant la porte accessible dans la cour |
| `research/intro-format.md` | Routines de demarrage, reconstruction et adaptations |
| `validation/md-player-jump.png` | Simon en plein saut dans la premiere salle |
| `research/player-format.md` | Sprite, collisions, saut et limites du prototype |
| `assets/stage-overview.png` | Apercu reconstruit des 18 premiers fonds de stages |
| `assets/rooms/` | 153 images PNG et cartes completes de 32 x 24 tuiles |
| `assets/manifest.json` | Provenance ROM, palettes, statut et limites de chaque fond |
| `research/room-format.md` | Formats retrouves et correspondances d'adresses |
| `research/loading-and-states.asm` | 801 instructions de regions selectionnees |
| `research/known.asm` | 111 annotations initiales, avec octets attendus |
| `sgdk/src/` | Explorateur et ressources C generees |
| `tools/` | Analyse, extraction, desassemblage et verification |
| `validation/` | Rapports et captures de test |

## Reproduire l'extraction

Depuis ce dossier, avec Python 3.10 ou plus recent :

```sh
python3 -m pip install -r requirements.txt
python3 tools/probe.py "/chemin/Vampire Killer (Japan, Europe).zip"
python3 tools/extract_rooms.py "/chemin/Vampire Killer (Japan, Europe).zip"
python3 tools/extract_player.py "/chemin/Vampire Killer (Japan, Europe).zip"
python3 tools/extract_intro.py "/chemin/Vampire Killer (Japan, Europe).zip"
python3 tools/disassemble_regions.py "/chemin/Vampire Killer (Japan, Europe).zip"
python3 tools/verify_z80_maps.py "/chemin/Vampire Killer (Japan, Europe).zip"
```

Un fichier `.rom` brut est egalement accepte. Les outils refusent une empreinte
SHA256 differente du dump etudie. La ROM MSX originale n'est pas incluse dans
ce dossier. Les ressources graphiques du programme proviennent du fichier fourni.
Les PNG et cartes dans `assets/rooms/` sont regenerables et exclus de Git.
Les ressources C generees sont versionnees : compiler la ROM MD ne necessite
pas le fichier MSX dans le checkout.

## Tests Python

Depuis la racine du depot, avec un interpreteur dans lequel `requirements.txt` est installe
(par exemple un venv : `python3 -m venv .venv && .venv/bin/pip install -r requirements.txt`) :

```sh
python3 -m unittest discover -s tests -p 'test_*.py'
```

`tests/test_flame_fx.py` lance `tools/build_flame_fx.py`, qui depend du module `z80`
et de la ROM de reference : il echoue avec un message explicite si `z80` manque.

## Compiler

Les routines de l'archive `start.zip` sont prises en compte : lecture de
manette dans `sgdk/src/input.c`, commandes transmises a la physique existante.
Les adaptations de physique, de virgule fixe et de ressources sont detaillees
dans [research/start-environment.md](research/start-environment.md).

Le code du portage est en C99, compile avec **GCC pour Motorola 68000**
(`m68k-elf-gcc`). Le Makefile fixe explicitement `-std=c99`.
GCC natif (x86/ARM) permet de tester les primitives du personnage ; la ROM
Mega Drive utilise le compilateur croise et les bibliotheques SGDK.

Avec une installation SGDK 2.11 fonctionnelle et sa chaine m68k-elf :

```sh
cd sgdk
make GDK=/chemin/vers/sgdk
```

Pour reproduire le controle GCC avec les avertissements traites en erreurs :

```sh
make GDK=/chemin/vers/sgdk clean
make GDK=/chemin/vers/sgdk EXTRA_FLAGS='-std=c99 -Werror'
```

Validation : compilation complete GCC m68k-elf 13.1.0 / SGDK 2.11 en C99
avec `-Wall -Wextra -Werror`, et primitives compilees avec GCC natif 13.3.0
en `-std=c99 -pedantic-errors -Wall -Wextra -Werror`. Les 53 872 comparaisons
C/Z80 passent. Apres integration du module de commandes de `start.zip`,
les tests de parcours, de deplacement et des 152 fonds passent egalement.

La sortie est `sgdk/out/rom.bin`. Sous Windows, on peut employer le makefile
de l'installation SGDK avec les outils fournis dans `bin/`.

L'environnement de validation utilise Marsdev v1.0.0-rc1 / GCC 13.1.0. Le format
LTO de la bibliotheque precompilee SGDK 2.11 etait incompatible avec ce GCC.
Les objets natifs deja presents dans cette archive ont ete utilises, en retirant
uniquement les sections LTO d'une copie de la bibliotheque :

```sh
m68k-elf-objcopy --remove-section='.gnu.lto_*' \
  /chemin/sgdk/lib/libmd.a /chemin/sgdk/lib/libmd-native.a
make GDK=/chemin/sgdk LIBMD=/chemin/sgdk/lib/libmd-native.a
```

La version Java disponible ici necessitait aussi que son dossier `lib/` soit
present dans `LD_LIBRARY_PATH`. Cela depend de l'installation locale de Java.
Les versions et empreintes des archives utilisees figurent dans `validation/toolchain.json`.

## Reproduire le test Mega Drive

Avec un core libretro Genesis Plus GX compile localement en sortie RGB565 :

```sh
python3 tools/verify_viewer.py /chemin/genesis_plus_gx_libretro.so \
  "/chemin/Vampire Killer (Japan, Europe).zip" build/vampire-killer-md-viewer.bin
```

La verification couvre le rendu MD par rapport a l'extraction. L'execution de
la routine Z80 valide separement les cartes. Aucune execution complete MSX2
avec BIOS, VDP et interruptions n'a encore ete comparee a ce portage.

Pour verifier les primitives du personnage avec un compilateur C natif :

```sh
cc -shared -fPIC -std=c99 -Wall -Wextra -Werror -O2 \
  sgdk/src/player.c sgdk/src/generated/player_assets.c -o /tmp/player.so
python3 tools/verify_player_primitives.py \
  "/chemin/Vampire Killer (Japan, Europe).zip" /tmp/player.so
python3 tools/verify_play_mode.py /chemin/genesis_plus_gx_libretro.so \
  "/chemin/Vampire Killer (Japan, Europe).zip" sgdk/out/rom.bin sgdk/out/symbol.txt
python3 tools/verify_intro.py /chemin/genesis_plus_gx_libretro.so \
  "/chemin/Vampire Killer (Japan, Europe).zip" sgdk/out/rom.bin sgdk/out/symbol.txt
```

Les fichiers de demarrage SGDK conservent leur licence MIT dans
`third_party/SGDK-LICENSE.txt`. Les emulateurs et la chaine de compilation ne sont
pas inclus. Prochain jalon : fouet, ennemis et son, puis extension des interactions du hall.
Depot : https://github.com/spamkenny02/vampirekiller.
