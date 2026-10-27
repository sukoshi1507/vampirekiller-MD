# Déclencheurs sonores du pilote MSX2 de Vampire Killer

Statut global : **partiel**. Trois déclencheurs sont établis (fouet, démarrage de musique à deux occasions, arrêt du fouet). Les autres événements (saut, marche, accroupi, dégâts, ramassage, changement de salle, fin) n'ont pas été déclenchés de façon exploitable : ils sont listés comme **inconnu**.

Données : `research/audio/triggers.json` (même contenu, format stable).
Capture : openMSX 21.0, C-BIOS_MSX2, ROM de référence (`scratchpad/speed/game.rom`). Les masques clavier sont ceux de `research/input-mapping-openmsx.md` (SPACE = ligne 8 masque 1, UP = 32, RIGHT = 128, LEFT = 16, DOWN = 64).

## Méthode

1. **Instantané par image** : au point d'arrêt VBLANK `4028`, lecture de `C010..C016`, `C094`, `C080..C0BF` (le bloc de variables son). Une ligne est écrite seulement quand un octet change. Cela suffit à voir les changements de pointeurs de canal, de `C0A7` et de `C096`.
2. **Auteur des écritures** : points de surveillance d'écriture sur **un seul octet** (`C016`, `C096`, `C0A7`, `C010`, `C007`, `C42F`, `C000`, `C001`), avec le PC de l'écrivain. Un watchpoint sur un octet isolé est rapide ; c'est la surveillance large de `C080..C0BF` qui était trop lente.
3. **Scénario** : titre passé par SPACE, puis, une fois `C000=5` atteint (partie en cours), pulsations contrôlées : SPACE, UP, RIGHT, LEFT, DOWN, UP+SPACE, SPACE répété 5 fois. Trois runs ont été faits (`scratchpad/sound/run1..run3`, scripts `scratchpad/sound/snap.tcl` et `snap_w.tcl`). Le run 3 (avec watchpoints) donne les PC. Les runs sont déterministes pour des entrées identiques.

## Tableau des événements

Les délais sont en images VBLANK (une image = 1/60 s). « Touche » = image du `keymatrixdown` vue par le script ; la première image de changement RAM est donnée dans la colonne suivante.

| Événement | Écriture RAM qui précède | Commande / routine | Canal | Délai mesuré | Statut |
|---|---|---|---|---|---|
| Attaque au fouet (SPACE, au sol) | `C42F` passe de 0 à 6 (PC `721D`), `C007` passe à 16 | `C096 = 05` (PC `517D`) puis `C016 = 9C` (PC `519E`) | pointeur 4 (`C016`), effet | touche +1 ou +2 images, `C42F` à la même image que le début du son ; 5 occurrences | **prouvé** (5 occurrences) |
| Fin de l'effet fouet | `C42F` = 0 (PC `7280`) | `C096 = 00` (PC `89D7`) et `C016 = F1` (PC `89E9`), dans le pilote | pointeur 4 (`C016`) rétabli à F1 | 13 images après le début du fouet (692 → 705, 1262 → 1275, 1352 → 1365, 1472 → 1485) | **prouvé** (4 occurrences) |
| Attaque en l'air (UP+SPACE) | même que le fouet (`C42F` = 6, puis 22) | même que le fouet | pointeur 4 | +2 images depuis la touche | **prouvé** (même chemin, 1 occurrence en l'air) |
| Démarrage musique (C000 = 3, C001 = 1, titre/intro) | `C000` = 3 (PC `43B5`, image 13) | écriture de `C010 = 64`, `C016 = 9E`, `C0A7 = 07` par la routine `5109..5136` | canaux 1 et 4 (`C010`, `C016`) | 1 image après l'écriture de `C000` | **prouvé** (routine 5109, 2 occurrences) ; l'identité « titre » est **déduite** |
| Démarrage musique (C000 = 5, C001 = 0, partie en cours) | `C000` = 5 (PC `4251`, image 511) | même routine `5109..5136`, même valeur `C0A7 = 07` | canaux 1 et 4 | 0 image (même image que `C000`) | **prouvé** (routine 5109, 2 occurrences) ; l'identité « cour » est **déduite** (`C000=5` = partie en cours, `research/input-gate.md`) |
| Arrêt de la musique titre/intro | aucune écriture de `C000` ou `C001` identifiée dans la fenêtre (image 399, sans touche) | `C0A7` = 06, 04, 00 et `C010 = F1` (PC `8C8F`, `89E9`) | canal 1 coupé | inconnu | **inconnu** (1 occurrence, cause non trouvée) |
| Saut seul (UP) | aucune écriture de `C096`, `C016`, `C0A7` ou `C010` (`C007` = 33 à l'image 822) | aucune | aucun | aucun son vu | **inconnu** (absence observée 1 fois) |
| Marche à droite (RIGHT) | aucune écriture de ces octets (image 931 à 991) | aucune | aucun | aucun son vu | **inconnu** (absence observée 1 fois) |
| Marche à gauche (LEFT) | aucune écriture de ces octets (image 1081 à 1117) | aucune | aucun | aucun son vu | **inconnu** (absence observée 1 fois) |
| Accroupi (DOWN) | aucune écriture de ces octets (`C42E` = 16, image 1171 à 1201) | aucune | aucun | aucun son vu | **inconnu** (absence observée 1 fois) |
| Impact sur ennemi | non déclenché (aucun ennemi touché dans les runs) | inconnu | inconnu | inconnu | **inconnu** |
| Dégâts subis | non déclenché | inconnu | inconnu | inconnu | **inconnu** |
| Ramassage (objet) | non déclenché | inconnu | inconnu | inconnu | **inconnu** |
| Changement de salle | non déclenché (`C000` ne change qu'une fois, de 3 à 5) | inconnu | inconnu | inconnu | **inconnu** |
| Fin / game over | non déclenché | inconnu | inconnu | inconnu | **inconnu** |

## Ce qui est établi sur le pilote

- Les octets `C010`, `C012`, `C014` (pointeurs des canaux de ton) et `C016` (pointeur du 4e canal) sont réécrits par le jeu ou le pilote. Le fouet et la musique utilisent `C016`. Une même voie peut donc être volée par l'effet du fouet pendant la musique : le fouet écrit `C016 = 9C` par-dessus le pointeur de musique, puis le pilote le rétablit à `F1` à la fin.
- `C096` agit comme un drapeau d'effet (0 = repos, 5 = fouet). Il est écrit par le jeu (`517D`) et remis à 0 par le pilote (`89D7`).
- `C097` alterne entre `B8` et `9C` à presque chaque image, indépendamment des actions du joueur (masque mixeur, `8964` / `896A`). Ce n'est pas un déclencheur.
- `C0A7` vaut `99` au boot, `07` au démarrage de musique (titre et cour), et prend les valeurs `06`, `04`, `00` lors de l'arrêt à l'image 399. Son rôle exact de file de commandes n'est pas établi.

## Ce qui n'est pas établi

- La file de commandes `C080..C0BF` n'a pas été désassemblée. Les octets `C0A7` et `C096` ont été lus comme des registres de commande, pas comme une file à longueur et terminateur connus.
- Le désassemblage de l'entrée `8964`/`896A` (banque 14) n'a pas été fait.
- Les événements de dégâts, de ramassage, d'ennemi, de changement de salle et de fin n'ont pas été produits. Il faut un scénario qui atteigne une salle avec ennemis ou objets.
- Le PC `721D` (attaque) se trouve dans la fenêtre banque `6000-7FFF` ; la banque n'a pas été relevée. Les adresses sont celles du CPU.

## Fichiers

- Données : `research/audio/triggers.json`
- Captures brutes (hors dépôt) : `scratchpad/sound/run1/snap.txt`, `scratchpad/sound/run2/snap.txt`, `scratchpad/sound/run3/snap.txt` et `scratchpad/sound/run3/writers.txt`
