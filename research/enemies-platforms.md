# Ennemis et plateformes mobiles - état de la recherche

> **Mise à jour** : la section 5 (plan d'implémentation) est réalisée dans
> `sgdk/src/enemies.c`, avec les corrections de `enemy-behavior.md`. Le reste de cette note
> reste une trace de la première recherche.

Statut : **un ennemi est observé et suivi** (deux ennemis en salle 1, trajectoires et cadence mesurées). **Aucune plateforme mobile** n'est trouvée. La table de spawn par salle n'est **pas** identifiée ; la routine `5A49` décode un autre flux (carte de cellules, voir 1.4).

Ce document remplace la version précédente, qui disait que Simon ne bougeait pas et que le mapping des touches était inconnu. Ces deux points sont faux (voir 1.1).

Adresses CPU sauf mention contraire. Banque fixe `4000-5FFF` (banque 0). Dump : SHA-256 de la ROM `99eabaae...` (ROM extraite du zip, voir `research/input-gate.md`).

## 1. Prouvé (exécution openMSX ou lecture de la ROM)

### 1.1 Entrées et marche de Simon

- `keymatrixdown 8 N` prend un **masque de bits** sur la ligne 8, pas un index de colonne. Mapping utilisé : SPACE = masque 1 (fouet), LEFT = 16, UP = 32, DOWN = 64, RIGHT = 128. Détail et mesures : `research/input-gate.md`, `research/input-mapping-openmsx.md`.
- Marche à droite : Simon avance de **1 px par VBLANK** (`C427` +2 toutes les 2 images). Mesure refaite dans cette recherche : x passe de 16 à 232 en 13,5 s environ.
- Le contrôle de Simon ne dépend pas de `C00B`/`C00C` (hypothèse précédente fausse).

### 1.2 Trace de l'ennemi (salle de départ puis salle suivante)

Script : `scratchpad/enemies6/trace.tcl` (lecture seule, VBLANK `4028`, RIGHT maintenu de 25 s à 60 s, SPACE au titre). Sortie : `scratchpad/enemies6/frames.txt` (2217 images : E000-E47F, C420-C5FF, D600-D67F, C000/C001, D000, C425/C427 à chaque image). Analyse : `scratchpad/enemies6/an2.py`, `an4.py`, `an5.py`.

Chronologie (images VBLANK, 60 Hz ; `C000` = état de jeu) :

| Image | Temps | Événement |
|---|---|---|
| 1-838 | 24 s-38 s | Salle de départ (`D000=0`), Simon marche de x=16 à x=232. **Aucun ennemi mobile.** |
| ~776 | ~37 s | Apparition de 8 sprites fixes à `X=E0`, `Y=8F/9F`, N=`20..2C` (Simon à x=216). Ils ne bougent pas jusqu'à la sortie de salle. Nature non identifiée (pas le fouet : N diffère de `research/whip-hitbox.md`). |
| 839 | 38,0 s | `C000=10` (transition), puis `C000=4`, `D000=1` : chargement de la salle suivante. |
| 857-858 | 38,6 s | `E000-E47F` réécrit (chargement de la carte de la salle 1). |
| 908 | 39,1 s | **Premier ennemi** : 4 entrées SAT (s14-s17) à `X=E5`, `Y=A0/B0`, Simon à x=48. Première image à `Y=E1`, `X=99` (valeur provisoire), puis position réelle. |
| 1000-1025 | 40,6 s-41,1 s | Contact avec Simon (ennemi 1 à x=136 au moment où Simon est à 136) : Simon recule de x 136 à 108 et son Y (`C425`) fait un arc 192 -> 176 -> 192. |
| 1012 | 40,9 s | **Deuxième ennemi** (s18-s21) : spawn à `X=0A`, `Y=A0/B0`, va vers la droite. |
| 1126 | 43,5 s | Ennemi 1 sort à gauche (x proche de 0) : disparaît du SAT. |
| 1208 | 44,8 s | Ennemi 2 disparaît (x=216). Entrée `C000=9` avec Simon à x=10 (voir 1.5). |
| 1256 | 45,9 s | Ennemi 1 réapparaît à droite. Cycle de passages répété (1474, 1564, 1585, 1694). |

**Vitesse et cadence de l'ennemi 1 (mesurées)** :

- Déplacement : **-1 px par image** (x = E5 en 909-910, E3 en 911-912, E1 en 913-914 ...). Pas de pause observée sur 218 images. Le temps de traversée d'un bord à l'autre est d'environ 218 images (3,6 s).
- Animation : seules les 4 entrées du bas (N=48/4C ou 50/54) alternent. Pose 48/4C pendant 8 images, pose 50/54 pendant 8 images. Cycle complet de 16 images (1st cycle : 14 images, car le spawn compte). Les entrées du haut restent à N=40/44.
- Simon marche à 1 px par image, donc la distance relative se ferme à 2 px par image.

**Deuxième ennemi** : même vitesse en valeur absolue (+1 px par image vers la droite), même cadence d'animation (8 images par pose), patterns différents (N=58/5C en haut, 60/64 ou 68/6C en bas).

**Cadence de réapparition** : les passages alternent entre bord droit et bord gauche. Intervalles observés entre une disparition et la réapparition suivante : 130, 111, 152 images. La cause (minuterie, compteur, ou événement) est **inconnue**.

### 1.3 Sprites et SAT

- La SAT est en `D600-D67F` (32 entrées de 4 octets : Y, X, N, couleur). Une entrée à `Y=E0` est masquée (vérifié).
- Simon occupe 4 entrées (s0-s3). Le sprite est en Y = `C425-16` et `C425-32` : pour Simon à `C425=C0`, Y = `B0` et `A0`. L'ennemi 1 occupe la même bande verticale (Y `A0` et `B0`), donc il fait la même taille que Simon.
- Les entrées d'un même sprite partagent le même X (paires N=40/44 et 48/4C). La géométrie exacte (16x16 ou 8x8, l'ordre des motifs) n'est **pas** vérifiée : il faudrait un décodage de la VRAM (la SAT réelle est en `F600`, voir `research/whip-hitbox.md`). Les captures d'écran ne fonctionnent pas avec le rendu de la machine de test.

### 1.4 Routine `5A49` : décodage de carte, pas de table d'ennemis

- `5A49` efface `E000-E47F`, puis lit un flux à partir de `5A98` :
  - Stage 0 (`D000=0`) : flux à `800C` (banque 14).
  - Autres stages : table de pointeurs en `8000` (banque 14), indexée par `D002`.
- Format prouvé par le code (`5A65-5A90`) :
  - Un octet `00` termine le flux.
  - `FE` : passe à la ligne suivante (+0x18 octets dans `E000`).
  - `FF` : saute +0x180 octets.
  - Sinon : deux octets copiés (index de cellule, attribut). Si le deuxième vaut `7F`, un troisième octet est copié.
- Vérifié : un décodeur Python (`scratchpad/enemies6/decode5a49.py`) reproduit exactement `E000-E003`, `E018-E01B`, `E030-E031` de la RAM à l'image 1 (salle 0).
- Conclusion : `E000-E47F` est une **carte de cellules à 2 octets** (12 cellules par ligne), réécrite à chaque chargement de salle et **constante** pendant la salle (aucun octet ne change en jeu, seulement au changement de salle). Ce n'est pas une liste d'ennemis : aucun ennemi n'apparaît dans ce bloc. Le flux `5A49` n'est donc **pas** la table de spawn.

### 1.5 Ce qui est écrit par qui (watchpoints, 60 s de jeu)

- `C425` (Y de Simon) est écrit par : `6D5D` (dans une routine `6D4D-6D60` qui ajoute une valeur d'une table à Y : arc de saut ou de recul), `63C3`/`6411`/`63D8` (chargement de salle), `44D2`, `40A5`, `830F` (initialisation).
- `C427` (X) est écrit par `6C98` (732 écritures, marche de Simon, contrôle) et quelques écritures d'initialisation.
- **Aucune écriture de `C425` ou `C427` hors de ces routines** n'a été vue. En particulier, aucune plateforme mobile n'a déplacé Simon dans les salles 0 et 1 pendant ce test.
- `C000=9` apparaît quand Simon atteint x=10 après être passé à droite (images 293, 1208, 1564). Cela ressemble à un défilement ou à une réapparition de l'écran (x repart de 10), pas à une mort. **Déduit**, non vérifié.

## 2. Déduit du code (non vérifié en exécution)

- La routine `6D4D-6D60` applique une table de delta à Y. Elle est utilisée par le saut et par le recul. Elle pourrait servir à une plateforme, mais rien ne le prouve.
- `C476` et `C486` (dans `C420-C5FF`) s'incrémentent d'une unité toutes les 2 images pendant la marche : compteur de défilement probable. Déduit.
- La table de spawn des ennemis est probablement dans la banque 14 (là où `5A49` lit `8000`/`800C`), mais ce n'est pas prouvé.

## 3. Inconnu

- Table de spawn par salle : position, type, nombre, et l'état qui déclenche le spawn (`C426` et `C43x` sont candidats).
- Cause des intervalles de réapparition (130, 111, 152 images).
- Nature des 8 sprites fixes `X=E0` (salle 0, image 776). Peut-être un objet de décor ou un cierge.
- Géométrie exacte d'un ennemi (16x16 ou 8x8, boîte de collision) et dégâts au contact.
- Mécanique du recul : table de delta, durée, invulnérabilité.
- Plateformes mobiles : **aucune trouvée** dans les salles testées (0 et 1). Il faut tester d'autres salles (stage 1 et plus) avec un scénario qui tombe sur une plateforme.
- Ce qui se passe en `C000=9`.

## 4. Statut des touches (openMSX)

| Touche | Masque ligne 8 | Effet vérifié |
|---|---|---|
| SPACE | 1 | Fouet |
| LEFT | 16 | Marche à gauche |
| UP | 32 | Saut |
| DOWN | 64 | Accroupi |
| RIGHT | 128 | Marche à droite (1 px/image) |

## 5. Plan d'implémentation SGDK minimal (rien n'est créé)

Objectif : **un ennemi simple** d'abord (aller-retour ou traversée, 1 px/image, animation 8 images par pose, spawn au bord). Les plateformes viennent après, quand une salle en contient une.

Fichiers à créer (aucun fichier existant n'est modifié par cette recherche) :

- `sgdk/src/enemies.h` et `sgdk/src/enemies.c` :
  - structure `Enemy` : x, y, sens (`dx` = -1 ou +1), indice d'animation, actif ;
  - `enemies_spawn_edge(side)` : spawn à `X=0xE5` (droite, vers la gauche) ou `X=0x0A` (gauche, vers la droite) avec `Y` de la bande du sol ;
  - `enemies_update()` : `x += dx` chaque image, animation toutes les 8 images (poses 48/4C et 50/54 pour l'ennemi 1), désactivation hors écran ;
  - `enemies_draw()` : `SPR_setPosition` pour les 4 sprites (le haut est fixe, le bas alterne).
- Données : `sgdk/src/generated/enemy1_md.h` (motifs), produits par un outil dédié, par exemple `tools/build_enemy_assets.py`. Ce script doit lire la ROM de référence via `tools/probe.py` (`reference_rom()`), sans rien committer de la ROM.
- Données de test : `tools/verify_enemies.py` (vérifie les 218 images de traversée et la cadence 8/8 à partir de `scratchpad/enemies6/frames.txt`).

Points d'intégration (à faire par le responsable, pas ici) : appeler `enemies_update()` et `enemies_draw()` dans la boucle de jeu (`sgdk/src/gameplay.c` ou `sgdk/src/main.c`). Ces fichiers sont interdits dans cette tâche.

Ne pas faire encore : la table de spawn, les plateformes, la collision et les dégâts (à prouver d'abord, voir section 3).

## 6. Traces et outils

- Trace : `scratchpad/enemies6/trace.tcl` (script openMSX, lecture seule), résultat `scratchpad/enemies6/frames.txt`, ROM de travail `scratchpad/gate/game.rom`.
- Analyse : `scratchpad/enemies6/an2.py` (états), `an3.py` (octets qui changent), `an4.py` (entrées SAT), `an5.py` (suivi des ennemis), `decode5a49.py` (décodeur de `5A49`).
- Écritures de `C425` : `scratchpad/enemies6/watch_y.tcl`, sortie `scratchpad/enemies6/watch_c425.txt`.
- Désassemblage : `scratchpad/dis.py` (banque et fenêtre explicites).
- Tracer d'entités précédent : `tools/trace_entities_live.tcl` (en lecture seule). Il lit `C420-C44F`, `D600-D67F` et `C470-C4FF` ; il ne lit pas `E000` et ne couvre donc pas la carte.
