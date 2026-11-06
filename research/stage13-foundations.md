# Stage 13 — journal ROM et intégration préparatoire (6 novembre 2026)

**STATUT : non complet, non jouable, non raccordé.** Ce fichier donne les données réellement extraites; il ne remplace pas une validation des objets/ennemis.

## Preuves et méthodologie

- ROM MSX2 vérifiée SHA-256 : `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`. Le dump original est exclu de l’archive.
- 12 salles : routine de décodage statique de `tools/extract_rooms.py`; 768 identifiants par carte et palette RGB333 par salle.
- 12 entrées de navigation : table `B9D3`, pointeur du stage 13 `0xBAB5` (banque 13).
- Flags de canaux : table de pointeurs à `0x1C5C0`, pointeur CPU `0x862A`, offset fichier `0x1C62A`.
- Groupe de stage : octet ROM `4` à `0x01E77`.
- Les flags de canaux ne sont PAS des types d’ennemis. Le tableau des ennemis fixes n’est PAS extrait.

## Navigation brute (la faisabilité des franchissements reste à valider)

| Salle | Haut | Bas | Gauche | Droite | Drapeau canal |
|---:|---:|---:|---:|---:|---:|
| 0 | 3 | — | — | 1 | 0 |
| 1 | — | — | 0 | 2 | 0 |
| 2 | 5 | 7 | 1 | — | 128 |
| 3 | — | 0 | 6 | 4 | 0 |
| 4 | — | — | 3 | 5 | 0 |
| 5 | — | 2 | 4 | 6 | 0 |
| 6 | 11 | — | 5 | 3 | 128 |
| 7 | 2 | 9 | — | — | 128 |
| 8 | — | — | — | 9 | 128 |
| 9 | 7 | — | 8 | 10 | 128 |
| 10 | — | 11 | 9 | — | 128 |
| 11 | 10 | 6 | 9 | — | 128 |

- **Asymétrie préservée** : salle 11, gauche → 9, mais salle 9, droite → 10. Valeurs brutes, pas une erreur corrigée par intuition.
- Les octets de drapeau `0x80` présents dans les salles 2 et 6 à 11 sont maintenus tels quels. Ils ne prouvent ni le nombre ni le type d’ennemis.

## Fichiers ajoutés et usage

| Fichier | Rôle |
|---|---|
| `tools/build_stage13_foundations.py` | Réextraction reproductible ROM → tableaux C et manifeste JSON |
| `tools/verify_stage13_rom.py` | Recalcule et compare pointeurs, hash des cartes, palettes, navigation, flags |
| `sgdk/src/generated/stage13_foundations.c/.h` | 12 fonds statiques de 768 octets, 12 liens, 12 flags |
| `research/stage13-foundations.json` | Inventaire machine-lisible avec SHA-256 par salle et offsets |
| `tests/test_stage13_foundations.c` | Contrôles compilés des 12 tableaux et asymétries ROM |
| `tests/run_stage13_foundations.sh` | Compilation et exécution indépendante du test statique |
| `LIVRAISON/logs/*` | Journaux des contrôles exécutés |

## Tests réellement exécutés

- `sh tests/run_stage13_foundations.sh` : PASS (valeurs statiques, **pas** exécution des routines ennemies).
- `python tools/verify_stage13_rom.py /path/to/reference.rom` : PASS (12 cartes, 12 liens, 12 flags et palettes recoupés avec la ROM).
- `sh tests/run_native.sh` : PASS (suite cumulative du jeu existant ; elle ne valide pas le gameplay du stage 13).
- **Pas de compilation SGDK ni de test de ROM Mega Drive pour le stage 13.**

## Travail restant indispensable pour une livraison complète

1. Exécuter la routine Z80 `4F83` pour toutes les salles et produire les cartes de collisions utilisées par le gameplay. Le fond statique n’est pas automatiquement une carte de collision validée.
2. Exécuter `5A49`/`5B1B` sur les salles pour toutes les bougies, objets, blocs, clés et portes et consigner coordonnées, contenus et conditions.
3. Exécuter `6154` pour les ennemis fixes, compléter les canaux d’apparition, identifier types et états depuis les routines Z80 et traces openMSX.
4. Répertorier et porter chaque comportement d’ennemi, projectile et, le cas échéant, boss, leurs événements audio et leur système de dégâts ; établir vecteurs de comparaison Z80.
5. Valider les transitions verticales et horizontales et les éventuelles mécaniques supplémentaires par traces d’exécution ; aucune mécanique de passage secret n’est présumée.
6. Intégrer dans SGDK : limites de tableaux (>11), chargeur de cartes de collisions, objets, ennemis, porte et fin de stage ; effectuer une compilation et des tests image par image.

**Contrainte :** la dépendance Python `z80==1.2.0` requise par `tools/castle_room_objects.py` est absente de cet environnement. Aucun résultat qui nécessite son exécution n’a été fabriqué.

## Portée exacte de la livraison

Ce ZIP conserve les changements cumulés des étapes précédentes. Les ressources stage 13 sont **isolées** et **non raccordées** à la progression du jeu comme demandé. Les ennemi/objets/boss du stage 13 ne sont pas prétendus portés.
