# Stage 7 : squelette 0B et ses os, puce 0D, créature volante 0C, clé, coffres, porte et sortie

Code : `sgdk/src/enemies_stage7.inc` (inclus par `enemies.c` ; routines Z80 traduites ligne à ligne sur les octets +0 à +1F de l'objet),
crochets dans `enemies.c` (types 11, 12, 13, boîtes, dégâts, points de vie, sprites), `actors.c` (tuiles). Tuiles :
`tools/build_stage7_enemies_md.py` (instantanés VRAM d'openMSX non versionnés, `scratchpad/stage7/out/{R,L,Q7}`). Bougies, blocs, clés,
coffres et murs : `tools/build_candles_md.py` (`stage > 7` aux deux endroits, `WALL_STAGES = 8`). Tests : `tests/test_stage7.c` (natif),
`tests/stage7_vectors.h` (généré par `tools/gen_stage7_vectors.py`), scénario `tests/md_scenarios/zza_stage7.txt`.

Forcer le stage 7 dans openMSX : `D000` = 7, `D002` = 2 (table `5E6A`, octet de fichier `1E6A`, indice 7 : 2, comme attendu), `D001` = salle,
`C000` = 4. Le stage a 9 salles (0 à 8 ; liens à l'indice 7 de `MSX2_ROOM_LINKS.json`). Le filtre `D000` de `t5_extra.tcl` passe à 7.

## Objets vus (trace R, 500 images par salle) et liste fixe (`vk_fixed_enemies`, extraite de `6154`)

| Salle | Drapeau (85A6) | Liste fixe | Objets observés | Fixes (clés 17, coffres 19) |
|---|---|---|---|---|
| 0 | 80 | - | aucun | mur (32, 48) |
| 1 | 80 | 0D (80, 144), 0D (160, 144) | deux puces | coffre (64, 128), clé (224, 64) |
| 2 | 80 | 0B (144, 176) | squelette, os | clé (32, 160) |
| 3 | 80 | 0B (176, 192) | squelette, os | coffre (208, 96), bloc caché (192, 128) = clé blanche |
| 4 | 00 | 0B (208, 192), 0D (176, 128), 0D (80, 80) | les trois | coffres (64, 64) et (160, 112), clé (160, 64) |
| 5 | 80 | 0B (48, 144) | squelette, os | coffre (32, 128), clé (176, 96) |
| 6 | 00 | 0B (144, 176) | squelette, os | clé (176, 144) ; porte (236, 96) |
| 7 | 00 | 0C (112, 80) | créature volante | clé (224, 112) |
| 8 | 00 | 0C (112, 80) | créature volante | mur (32, 144) |

Le bit 7 du drapeau n'appartient à aucun canal (`5EF4`-`5F19` testent les bits 0 à 6) : aucun ennemi n'apparaît en cours de salle. Pas de
boss, pas de pics, pas de plateformes (`C580` nul sur toute la trace). Les clés, coffres, murs et le bloc caché sont dans `candles_md.c`
(6 objets 0x17 et 5 objets 0x19, ceux du tableau).

## Méthode de preuve (nouvelle)

Les routines d'objet de la ROM (`993B`, `99B9`, `99E5`, boucle D700 `9E31`) tournent dans l'émulateur Z80 de `tools/castle_room_objects.py`
après `5A49`, `5B1B` et `4F83` (carte de collision en `D100`). Deux comparaisons :
1. **Trace contre ROM exécutée** (`scratchpad/stage7/cmp_trace.py`) : pour chaque objet et chaque salle, on prend les échantillons
   successifs distincts de `sat_s2.csv`, on place l'objet comme au premier échantillon (après l'init), puis on avance la ROM de 1 à 3
   passes, avec le X et le Y de Simon de la même image, jusqu'à retrouver l'échantillon (comparaison passe par passe,
   `research/stage4-enemies.md`). 1982 échantillons identiques sur 2010 (neuf salles, onze objets). Les 28 autres sont des premiers
   échantillons d'un objet (le bloc `C800` garde les octets de l'occupant précédent : +10, +12), des Simon déplacés par un recul entre deux
   échantillons, et la puce, dont le compte initial et la hauteur du saut viennent de `LD A,R` : ils n'ont pas été examinés un à un.
2. **Port contre ROM exécutée** (`tests/test_stage7.c`, `tools/gen_stage7_vectors.py`) : 17 scénarios, 4122 passes, état, X, Y, pose et
   os de chaque passe : **0 écart**. `LD A,R` est remplacé dans l'émulateur par `LD A,n` (`Enemies_s7R` côté port) pour que le jeu de
   référence soit reproductible.

La chaîne trace = ROM = port est donc prouvée pour les passes ; le port n'a pas été comparé directement à la trace (Simon y bouge à
chaque image, les vecteurs le font bouger de la même façon dans les deux modèles).

## Prouvé (ROM, trace et ROM exécutée)

**Objet 0B, squelette** (init `B344`, routine `B37E`, pose `B364`, états `B38D`, `B451`, `B4A9`, points de vie 4, 200 points, classe 1 :
boîte 8378 `(5, 18h)`, fouet 8380, armes 8388). Il garde ses distances : à moins de 30h de Simon il s'éloigne, à plus de 50h il s'approche
(vx 240h, plus 20h par unité de D012), et quand le compteur de pose `+12` a ses bits 1 et 2 à zéro (`B3C6`) il s'arrête 16 passes (état 2),
puis lance un os (`9F61`). Sans sol 4 ou 9 px devant lui il saute (vy FB8Fh, état 1) ; il retombe quand le sol est trouvé (`B499` : Y et
fractions alignés à 8). Pose 47 ou 49 (gauche, droite) plus 1 quand le bit 3 de `+12` est nul.
- Trace R salle 2 : X 146, 148, 150, 153 aux quatre premières passes (92h, 94h, 96h, 99h en trace), chute du bord en X 216 Y 192 (passe
  38), arrêt de 16 passes, premier os en (217, 171). Le scénario le reproduit.

**Os (D700 de type 4, `9F07`, créé par `9F61`)** : en (X, Y - 16), vy F800h ou FB00h (bit 0 de R), vx +-180h vers Simon ; chaque passe :
`+C` + 1 modulo 12, pose 4B + (`+C` >> 2), vy + 50h (`A549`), déplacement, suppression à Y >= E4h, X >= F1h ou X < 7. Boîte classe 2
de `830D` : 8358 `(6, 0Ch)`, fouet 8360, armes 8368, 100 points (`822A`), dégât 1 (`7F3D`, B = 1), détruit au contact (`99F6`).
`+C` reste dans le bloc D700 d'un os à l'autre (trace : premier échantillon d'un os à `+C` = 1, 6, 7, 5, 8...) : le port garde huit compteurs
et prend le premier bloc libre sur huit.
- Trace : énergie 30 -> 29, 29 -> 28 et 28 -> 27 (salles 3, 4 et 6), un point à chaque fois ; un os est la seule cause possible (les D700 de ce stage sont des os). Le port : 32 -> 31 et l'os disparaît (test natif).

**Objet 0D, puce** (init `B212`, routine `B248`, états 4 `B31A`, 3 `B2C7`, 2 `B285`, 1 `B268`, 0 `B258`, 200 points, 1 point de vie, classe 2 :
boîte 8390 `(8, 10h)`). Elle commence à l'état 4 (pose 67 ou 6A face à Simon), saute vers lui (vy FFFFh puis gravité A0h, vx +-220h) quand
il est à moins de 3Ch ou quand `+C` (3Ch - R/8 au départ) tombe à 0, retombe (`B279`, état 2, deux passes), puis saute de nouveau (vy FDA0h,
ou F8E0h une fois sur quatre quand Y >= 50h). Murs : demi-tour (`B300`). Les états 0 et 1 ne sont pas atteints par ce stage (la ROM les code).
- Salle 1, Simon (232, 176) et R = 0 : attente de 60 passes, puis (82, 143), (84, 144), (84, 144) au sol, (86, 137), (88, 130) comme la ROM
  exécutée ; le contact fait 32 -> 30 (trace : 1475, 32 -> 30).

**Objet 0C, créature volante** (init `A670`, routine `A67D`, huit états `A696` à `A793`, 1 point de vie, 100 points, classe 2 : boîte 8390).
Attente de 18h passes (état 0), vol (`A7BD` : vx +-200h vers Simon sauf près des bords, vy FDE0h à FB80h ou 220h à 380h selon la parité de `+18`),
freinage de vy de 20h par passe jusqu'à 0, immobilité 18h passes, puis soit nouveau vol, soit charge (vx +-300h, 30h passes, `A6FE`) quand Simon
est dans les 18h sous elle ; sortie des bornes (`A82D`) : retour à l'immobilité. Pose 87, 88 (gauche) ou 8A, 8B (droite) selon vx.
- Salle 7, Simon (232, 176) : 24 passes d'attente, puis (114, 82), (116, 84), (118, 86) ; contacts de la trace 27 -> 25, 25 -> 23, 23 -> 21.

**Dégâts et points** (`81CE`, paires ; dégât doublé sans armure) : 0B 200 points dégât 2, 0C 100 points dégât 2, 0D 200 points dégât 2.
Mesurés en trace : 0D 2 (une fois), 0C 2 (trois fois), os 1 (trois fois, voir plus haut). Le fouet fait 1 (2 avec la chaîne, `7DE6`) : le squelette meurt en 4
coups ou 2 avec la chaîne (test natif).

**Sprites** (`6445`, tables à `B473`) : squelette poses 47 à 4A, deux cellules (X-8, Y-32) et (X-8, Y-16) (motifs 40/44 et 48/4C, 50/54 et 58/5C, 60/64
et 68/6C, 70/74 et 78/7C, couleurs 2 et 12 en CC) ; os 4B (X-4, Y-15, motif 90), 4C et 4D (X-8, Y-15, 98 et A0), couleurs 2 et 12 ;
puce 67, 68, 6A, 6B : motifs A8, B0, C0, C8 (X-8, Y-15), couleurs 2 et 5 ; créature 87, 88, 89, 8A, 8B : motifs 48, 40, 50, 60, 58, couleurs 2 et 1.
Les cellules sont composées dans un bloc de 128 tuiles (le bloc du type 5, comme au stage 6).

**Décor** : les neuf salles comparées au MSX (instantané VRAM, couleurs MD `(v + 18) // 36`, salle contrôlée par la colonne D001 du fichier de
l'instantané) : 550 à 1180 pixels différents par salle (zone y 32 à 175), tous dans les sprites (Simon, ennemis) et les deux dessins parasites en
(64, 144) et (192, 144) déjà connus. Les clés, coffres, murs et le bloc caché sont conformes.

**Clé blanche, porte et sortie** (`kStageItems[6]`, extrait de la ROM par `tools/extract_stage_exits.py`) : clé dans le bloc caché (192, 128) de
la salle 3 (prise au fouet depuis (188, 160), `items` = 1), porte de la salle 6 en (236, 96) côté droit. Scénario : porte fermée, ouverte
en tenant la touche, clé consommée, stage 8 salle 0, Simon en (16, 144) (`kStageEntries[7]`).

## Déduit (code lu, pas mesuré en jeu)

- Contact du squelette (dégât 2, boîte (5, 18h)) : la trace ne montre aucun contact direct avec lui (seulement ses os). Fouet et armes
  lancées sur les trois objets et sur l'os : testés en natif uniquement (boîtes lues dans `8378`-`8368`, points de vie `60E1`).
- Le bloc `+C`, `+10`, `+12` laissé par l'occupant précédent d'un bloc `C800` : le port garde un tableau d'octets par bloc, remis à zéro à
  chaque partie (`Enemies_init`) ; en jeu la ROM les garde de tout ce que le jeu y a mis avant (autre stage, autre salle).
- Ordre de création des os : le port prend le plus haut bloc libre des sept blocs `C800`, car il n'a qu'une table (la ROM a huit blocs D700 à part).
  Plus de sept objets à la fois perdraient un os (jamais atteint : trois os maximum en trace).
- Temps arrêté et objets figés : un objet figé ne bouge pas (`++age`), comme les autres stages ; non rejoué en stage 7.

## Non vérifié ou non porté

- **`LD A,R`** (`B216`, `B29E`, `B4AD`) : compte initial de la puce (3Ch - R/8), hauteur de son saut (une fois sur quatre) et arc de l'os (bit 0). Le
  port utilise un petit générateur (`s7Random`). Seules ces valeurs aléatoires diffèrent ; le reste est exact.
- Les poses de la puce vers la gauche (67, 68) ont leurs motifs relevés dans un instantané ; elles n'ont pas été comparées image par image.
- Position exacte du sprite d'un os (la SAT MSX le montre une passe en retard, comme les autres sprites) : écart d'une passe en affichage.
- La transition de 28 images entre la sortie du boss et le chargement du stage suivant est portée (voir `research/stage6-boss.md`).
- **Après la sortie** : le port charge le stage 8, salle 0, Simon en (16, 144). Ses ennemis relèvent du portage du stage 8.

## Anomalies relevées

- Le brief appelait `D002` = 2 pour le stage 7 : la table `5E6A` le confirme (indice 7 : 2).
- Dans une trace, la colonne salle suit `D001` posé à la main ; les instantanés finaux des salles 1, 4 et 7 montraient la salle voisine (Simon renvoyé
  par un coup) : les comparaisons de décor ont pris des instantanés dont la ligne `d 7/n` donne bien la salle.
- Le port gardait `Enemies_s7R` comme une constante (LTO) : la variable est `volatile` pour rester dans la table des symboles du harnais.
- Le test `enemy_integration.c` limitait les types à `ENEMY_TYPE_FIREBALL` : relevé à `ENEMY_TYPE_FLEA`.

## Fusion avec le stage 8

Le squelette 0B, la créature volante 0C et l'os D700 de type 4 sont les mêmes objets ROM que ceux du stage 8 (`B37E`, `A67D`, `9F07`). Le port n'en garde
qu'une implémentation, celle du stage 8 (`hopperStep`, `leaperStep`, `boneStep`, types 11, 12 et 13), prouvée contre les traces openMSX. Les 4122 passes
des vecteurs Z80 du stage 7 (`tests/test_stage7.c`) passent sur ce code partagé sans écart. Seule la puce 0D reste ici (`sgdk/src/enemies_stage7.inc`,
type 14). Écarts relevés : `B344` tombe dans `B364` (+12 + 1 et pose dès la création, que le stage 8 omettait) ; le lancer de l'os lit le bit 0 de R
(`s7Random`, fixable par les tests) ; `A9A2` ne touche que Y (ni fraction ni vy) pour la puce.
