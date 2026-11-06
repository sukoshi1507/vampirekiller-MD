> État actuel : voir GLOBAL_LEVEL_VALIDATION.md. Cartes complètes restaurées et points d’apparition du mode test contrôlés.

> Mise à jour : les liaisons entre salles sont désormais extraites de la ROM. Voir ROOM_TRANSITIONS_VERIFICATION.md pour les tests et limites actuels.

# Sorties MSX2 appliquées au portage Mega Drive — 4 novembre 2026

Cette version remplace les sorties de test des niveaux 4 à 10. Les salles, coordonnées,
types de clé (cachée ou visible), côtés des portes et positions d’entrée sont extraits
de la ROM fournie. Les sorties des niveaux 1 à 3 sont également comparées à cette ROM.

## Données vérifiées

Numéros de salle à partir de 0. Coordonnées en pixels MSX2.
Le X de porte est l’ancre du moteur original, pas le bord du rectangle dessiné.

| Niveau | Clé : salle, (X,Y) | État initial | Porte : salle, (X,Y) | Côté | Entrée Simon (X,Y) |
|---|---|---|---|---|---|
| 1 | 2, (128,160) | bloc caché | 7, (236,48) | droite | (16,192) |
| 2 | 5, (192,112) | bloc caché | 1, (236,48) | droite | (16,96) |
| 3 | 2, (160,160) | bloc caché | 4, (236,128) | droite | (16,96) |
| 4 | 0, (32,64) | bloc caché | 3, (12,48) | gauche | (16,192) |
| 5 | 3, (192,64) | bloc caché | 5, (12,80) | gauche | (232,96) |
| 6 | 0, (192,160) | bloc caché | 4, (12,80) | gauche | (232,128) |
| 7 | 3, (192,128) | bloc caché | 6, (236,96) | droite | (232,176) |
| 8 | 7, (32,64) | visible | 7, (236,64) | droite | (16,144) |
| 9 | 8, (208,96) | visible | 8, (12,128) | gauche | (16,112) |
| 10 | 0, (48,48) | visible | 8, (236,128) | droite | (32,144) |

## Méthode et preuves

ROM SHA-256 : `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`.

- Clés : exécution Z80 des routines 5A49 puis 5B1B pour les salles des niveaux concernés.
  Les blocs ont le mode 3 et l’objet 0x18 ; les clés visibles ont le type 0x83 et l’objet 0x18.
- Portes : table physique 0x1BB61 + 3 × niveau et exécution de BB37 en banque 13.
- Entrées : exécution de 6402, lecture de C425/C427/C42C (Y/X/orientation).
- Zone de contact : routine 8580 comparée au prédicat C compilé sur 56 320 cas
  (tous les X, deux orientations, hauteurs limites et valeurs extrêmes), sans divergence.
- Tests natifs C des dix niveaux : coordonnées d’entrée, révélation des blocs, collecte,
  absence de doublon, refus sans clé, mauvaise orientation, mauvaise hauteur, escaliers,
  ouverture, déplacement dans le bon sens, événement de sortie et remise à zéro.
  Tests compilés avec AddressSanitizer et UndefinedBehaviorSanitizer : réussis.
  Détection de fuites désactivée car incompatible avec le traceur de cet environnement.
- Vérification antérieure du niveau 3 réactualisée : 10/10 contrôles réussis ;
  les 75 chandelles/blocs et 19 objets fixes des niveaux 0–3 sont conservés.
- Audit structurel de progression : réussi. Les tables de salles restent dans leurs limites.

## Changements du moteur

Les portes utilisent maintenant leurs coordonnées et leur côté pour l’affichage et le
contact. La marche de sortie peut aller à gauche. Les clés des niveaux 4–7 doivent être
révélées au fouet ; celles des niveaux 8–10 sont visibles dans leur véritable salle.
Quatre blocs de clé sont ajoutés au système existant des chandelles. Leurs tuiles sont
lues dans la variante de table ROM correspondant au niveau. Aucun autre ennemi ou objet
avancé n’est ajouté dans cette passe. Les index de chandelles prennent en charge neuf
salles avec une sentinelle de fin pour éviter un accès hors limites.

## Limites à connaître avant le test global

- Pas de compilation SGDK ni de test Kega Fusion ici : Docker et la toolchain m68k
  ne sont pas disponibles. L’archive contient les sources, pas une nouvelle ROM MD.
- Les liaisons entre salles et collisions provisoires des niveaux 4–10 ne sont pas
  corrigées par cette passe. La présence d’une clé/porte aux coordonnées authentiques
  ne garantit donc pas encore qu’elle soit accessible en jouant normalement.
- L’animation des portes réutilise le rythme du moteur MD (168 images), étendu aux
  sorties à gauche. Ce rythme et le rendu des portes ne sont pas validés image par image
  sur MSX2. La comparaison Z80 valide la zone de contact, pas toute l’animation.
- La sortie du niveau 10 conserve le retour au titre du prototype. Ce n’est pas la fin
  du jeu MSX2 : sa ROM possède d’autres niveaux que les dix accessibles dans ce portage.
- L’animation saccadée de Simon n’est pas traitée ici.

## Reproduire les contrôles

Depuis le dossier du projet :

```sh
sh tools/test_stage_exits.sh
python3 tools/check_stage_exits.py
python3 tools/audit_stage_progression.py
```

Pour comparer à la ROM originale (fichier .rom ou ZIP), installer `z80==1.2.0`
dans un environnement Python compatible, puis :

```sh
python3 tools/check_stage_exits.py /chemin/vers/reference.rom
python3 tools/verify_stage3_rom.py /chemin/vers/reference.rom src
```

Régénération : `tools/extract_stage_exits.py ROM`, puis
`tools/build_candles_md.py ROM src/generated`. La ROM originale n’est pas incluse.

Recompiler avec la même commande Docker SGDK qui fonctionnait dans ton environnement.
Pour isoler le test des sorties des défauts de topologie, le débogage existant permet
A + gauche/droite de changer de salle. Tester ensuite le cheminement normal séparément.

Ce rapport remplace les positions provisoires des sorties de test des niveaux 4 à 10
(version antérieure, retirée).
