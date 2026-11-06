# Diagnostic initial

Document historique du premier jalon. L'etat courant, les tests en execution
et le format des salles sont decrits dans `room-format.md` et le README.

## Identite du dump

| Champ | Valeur |
| --- | --- |
| Fichier dans le ZIP | Vampire Killer (Japan, Europe).rom |
| Taille | 131072 octets, soit 128 Kio |
| Signature | `41 42` / `AB` |
| Adresse INIT | `4075` |
| Autres vecteurs de l'en-tete | `0000` |
| SHA256 | `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2` |

Le nom du fichier n'est pas une preuve de revision ou de region detectee.
L'empreinte identifie ici les octets effectivement analyses.

## Resultats observes

Le demarrage `4075` configure les slots, positionne SP a `F0F0`, efface
`C000-F0EF`, appelle `5336`, `5C92`, de nouveau `5336`, puis `4B59`.
Il ecrit ensuite `JP 4028` a `FD9F`, reactive les interruptions et attend
indefiniment a `40C3`. La progression du programme passe donc par le hook installe.
La frequence effective, les conditions d'appel et les effets des interruptions
imbriquees restent a mesurer en execution.

`5336` ecrit 1 a `6000`, 2 a `8000` et 3 a `A000`. Ces valeurs sont aussi
conservees a `F0F1-F0F3`. C'est une evidence forte pour un mapper Konami sans SCC
a banques de 8 Kio. Ce modele de travail devra etre confirme par la trace du mapper.

| Fenetre CPU Z80 | Banque apres 5336 | Offset physique de debut |
| --- | --- | --- |
| `4000-5FFF` | `00`, fixe dans le modele retenu | `00000` |
| `6000-7FFF` | `01` | `02000` |
| `8000-9FFF` | `02` | `04000` |
| `A000-BFFF` | `03` | `06000` |

Le hook `4028` selectionne temporairement les banques `0E` et `0F`, puis appelle
`8964`. Sous ce mapping, cette cible correspond a l'offset fichier `1C964`.
Il restaure ensuite les banques depuis `F0F2-F0F3`. Le role sonore de ce chemin
est une piste, pas une identification acquise.

Le hook teste le bit 0 de `C005`, le positionne par increment avant d'appeler
`4B9D` et `414D`, puis le remet a zero. Ce mecanisme semble proteger ces traitements
contre la reentrance. `4B9D` mene a des lectures du clavier et du PSG.

Dans `4B59`, les octets de `4B5F-4B63` chargent A=5 puis appellent le BIOS
`CHGMOD` (`005F`). La selection initiale du mode 5 est donc directement observee.
Les changements ulterieurs de mode, les palettes, les formats de ressources et
les commandes VDP employes restent a identifier.

## Dispatch des etats

`414D` incremente `C003`, lit `C000` dans C et `C001` dans B, puis utilise
`C000` comme index via `406B`. Cette routine consomme l'adresse de retour du
CALL pour interpreter une table inline de pointeurs. Un desassemblage lineaire
prendrait ces donnees pour des instructions.

| Index visible | Destination |
| --- | --- |
| 0 | `417D` |
| 1 | `41A0` |
| 2 | `41AB` |
| 3 | `41D1` |
| 4 | `422C` |
| 5 | `4257` |
| 6 | `4294` |
| 7 | `42B2` |
| 8 | `4324` |
| 9 | `4377` |
| 10 | `4387` |
| 11 | `43DA` |
| 12 | `43F0` |
| 13 | `4414` |

Les 28 octets entre `4161` et `417C` donnent ces 14 destinations avant le premier
bloc cible. Les noms des etats et la plage reelle des index ne sont pas encore
confirmes. Pour les index inferieurs a 3, le code empile aussi `4391` avant le
dispatch : une traduction C devra conserver cet effet de retour.

## Methode de portage retenue

Reconstruire progressivement le comportement en C en gardant une correspondance
explicite entre routines, variables et adresses d'origine. Pour chaque fonction,
documenter les entrees, sorties, drapeaux utiles, effets en RAM et mapping actif.
Le Z80 present dans la Mega Drive n'est pas une machine MSX2 complete : executer
la ROM telle quelle demanderait aussi de reproduire son environnement materiel.

| Element | Travail prevu sur Mega Drive |
| --- | --- |
| Logique Z80 | Traduction C pour 68000, avec conservation des calculs 8/16 bits |
| Variables RAM | Structures nommees, provenance MSX conservee dans les commentaires |
| Adresses bankees | References de ressources explicites et offsets physiques verifies |
| Affichage MSX2 | Reconstruire les ressources en tuiles, plans et sprites adaptes au VDP MD |
| Clavier et joystick | Couche d'entree SGDK, equivalence des actions apres identification |
| Son | Identifier le driver et les sequences, puis choisir une restitution MD |
| Temporisation | Mesurer la cadence originale et definir le comportement PAL/NTSC |

La base SGDK utilise pour l'instant une largeur de 256 pixels comme choix de
travail, une hauteur de 224 lignes et la police integree. Ce n'est pas une
validation du cadrage final. Le son et le format des graphismes ne sont pas
encore convertis.

## Jalons suivants

1. Tracer les changements de banques et de `C000/C001` du boot a la premiere salle.
2. Nommer les etats et les variables a partir de ces observations.
3. Retrouver une salle, ses collisions, son graphisme, ses palettes et ses sprites.
4. Obtenir cette salle statique sur Mega Drive, avec comparaison a la capture MSX2.
5. Porter deplacements et collisions, puis fouet, objets et premier ennemi.
6. Etendre aux transitions, menus, son et autres salles, avec traces comparatives.

Premier objectif jouable propose : une salle, le personnage, les collisions et le
fouet. Les durees ne sont pas estimees avant la premiere trace et l'identification
du format des ressources.

## References techniques consultees

Les constats sur le jeu viennent des octets de la ROM. Ces references servent a
interpreter les appels materiels et a preparer la construction SGDK :

- MSX Assembly Page, appels BIOS : https://map.grauw.nl/resources/msxbios.php
- MSX Assembly Page, variables systeme : https://map.grauw.nl/resources/msxsystemvars.php
- openMSX, heuristique de detection des mappers dans RomFactory.cc : https://github.com/openMSX/openMSX/blob/master/src/memory/RomFactory.cc
- SGDK, utilisation : https://github.com/Stephane-D/SGDK/wiki/SGDK-Usage
- SGDK, makefile officiel : https://github.com/Stephane-D/SGDK/blob/master/makefile.gen
- SGDK, declarations VDP : https://github.com/Stephane-D/SGDK/blob/master/inc/vdp.h
- SGDK, declarations manette : https://github.com/Stephane-D/SGDK/blob/master/inc/joy.h

References consultees le 26 octobre 2026. Aucune version precise de SGDK n'a
encore ete installee ou figee pour ce projet.
