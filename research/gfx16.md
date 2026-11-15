# Mode graphique 16 bits (pilote : stages 1 à 3)

Option de l'écran titre : `GRAPH < MSX >` (défaut) ou `GRAPH < 16B >`, changée avec BAS. Elle ne touche qu'au dessin des salles du château : la logique du jeu (cartes de collision, objets, ennemis, sons) ne lit aucune donnée graphique, et le mode MSX reste le portage tracé.

## Source des graphismes

Les graphismes 16 bits sont ceux de Castlevania-MD (autre projet de l'auteur du portage), c'est-à-dire les tuiles redessinées par Fiuke Bnuy pour le portage SNES de Rumbleminze. Fiuke Bnuy les a données à l'auteur, qui en dispose librement : elles sont donc versionnées ici.

- `assets/gfx16/hall16.png` : premier écran du hall (monde du niveau 1 de Castlevania-MD, colonnes 768 à 1023, lignes 40 à 231), rendu avec les tuiles 16 bits.
- `assets/gfx16/under16.png` : salle souterraine `room_2_0` (lignes 48 à 239).

Les deux jeux suivent le même découpage du château (blocs 1-3, 4-6, 7-9, 10-12, 13-15, 16-17, 18) et les mêmes thèmes par bloc. Dans le hall, les motifs ont les mêmes dimensions : colonne 32 × 160, fenêtre 32 × 96 (carreau 16 × 16), grand rideau 32 × 144, blocs du sol 16 × 16.

## Principe : l'agencement MSX, case par case

`tools/build_gfx16.py` garde la carte MSX de chaque salle (32 × 22 cases de 8 × 8) et donne à chaque numéro de tuile MSX un dessin 16 bits par une règle (`block1_rules`) :

- `wall` : le fond de la salle, papier peint (32 × 32) si la salle contient la tuile 0E, sinon mur de briques sombres ;
- `tex` : une texture d'une image source, prise à la position de la case modulo la période de la texture (colonne, carreau de fenêtre, blocs, grille, eau, herbe, mousse) ;
- `mask` : une case de l'image source posée sur le fond (rideau : les tuiles du grand rideau de la salle 0 du stage 1 prennent la case du rideau 16 bits au même endroit) ;
- `stencil` : la forme de la tuile MSX, remplie de textures 16 bits (fissures du papier peint, escaliers) ;
- `bstencil` : idem sur le mur de briques (trous, arches) ;
- sans règle, la tuile garde ses pixels MSX (nervures de voûte du stage 3).

Par salle, l'outil produit :

- un jeu de tuiles d'au plus 180 emplacements (les tuiles du cadre du HUD suivent en VRAM). L'emplacement t < 192 est le dessin de la tuile MSX t à sa première position dans le bloc, pour que le code qui dessine des numéros de tuiles MSX (blocs cachés, `vk_key_block_tiles`) les trouve toujours ; les emplacements libres portent les variantes dont la salle a besoin quand une texture est plus large que la période de la tuile MSX (papier peint 32 × 32, eau). Mesuré : 134 à 138 emplacements, 2 à 30 variantes par salle ;
- la carte 32 × 22 (emplacements) ;
- une palette de 15 couleurs + noir (regroupement des couleurs les plus proches, comme `art.py` de Castlevania-MD) ;
- `msx[c]` : l'entrée de cette palette la plus proche de la couleur MSX c (pas encore utilisée par le code).

## Côté ROM (`sgdk/src/gfx16.c/.h`, `generated/gfx16_md.c/.h`)

Chaque dessin de salle recharge déjà les 192 tuiles du décor : en mode 16 bits, `draw_room` charge à la place le jeu de tuiles, la carte et la palette PAL0 de la salle (`vk_gfx16_room`). Aucun coût par image.

Les quatre lignes de palette sont prises (PAL0 salle, PAL1 HUD, PAL2 palette MSX de la salle pour les sprites, PAL3 Simon). En mode 16 bits PAL0 porte la palette du décor 16 bits ; tout ce qui est dessiné avec les couleurs MSX de la salle et qui utilisait PAL0 passe sur PAL2, qui contient toujours la palette MSX de la salle : macro `VK_PAL_OBJ` (bougies, objets tombés, clé d'or, icônes, porte, boîtes du HUD, murs cassables, contour des objets révélés, fenêtre du marchand). L'orbe des boss (entrées rouges) n'est plus écrit dans PAL0 en mode 16 bits.

Le code qui remet une case du décor (bloc caché cassé, bougie éteinte de la salle 0, clé prise) passe par `vk_room_cells` / `vk_room_tile`, qui rendent les données 16 bits quand la salle est dessinée ainsi.

La salle 0 du stage 1 est dessinée depuis la scène 6 de la ROM : en mode 16 bits ses 22 lignes de salle sont redessinées par-dessus avec les cases 16 bits.

## Cour du château

Les trois écrans de la cour (scènes 3, 4 et 5 de la ROM, des images complètes et non des salles en tuiles) sont remplacés par des images libres de 256 × 176, composées par `courtyard_pictures` de `tools/build_gfx16.py` :

- `assets/gfx16/court16.png` : la cour de Castlevania-MD (monde du niveau 1, colonnes 0 à 511, lignes 48 à 223), `assets/gfx16/gate16.png` : la tour du château (image `gate`, mêmes lignes). Le sol 16 bits tombe sur la ligne de sol MSX (y 144 de la salle).
- sous chaque torche MSX (x 64 et 192) un pied de brasero 16 bits (`assets/gfx16/simon/obj_0.png`, moitié basse) ; la flamme reste le sprite du jeu. Une torche éteinte montre les cases 16 bits (pas le haut de brasero MSX).
- troisième écran : la tour posée pour que son ouverture tombe sur la porte MSX (x 224 à 239) ; les 8 cases du montant passées devant Simon (`castle_door_front`) sont les cases 16 bits (`vk_scene_cell`).
- 37, 37 et 92 tuiles. `draw_scene` dessine ces scènes directement en 16 bits (même quantité de travail VDP que la scène MSX : le dessin du hall tient toujours dans l'image attendue par le scénario d'entrée). Le cadre des cases d'objets (lignes 1 à 3, colonnes 15 à 31) fait partie de la scène MSX : ses quelques tuiles sont rechargées après les tuiles 16 bits et dessinées en PAL2.

## Introduction

Avec l'option 16B, la scène MSX du château (scène 2 et ses sprites) est remplacée par l'introduction NES en 16 bits de Castlevania-MD (Simon marche vers la grille, chauves-souris, nuage), décision de l'utilisateur. `tools/import_intro16.py` copie les données de `castlevania-md/src/screens.c` dans `generated/intro16_md.c/.h` : 136 tuiles, 4 palettes, 32 × 28 cases, 458 images de sprites 8 × 16 (10 au plus par image). La barre d'état NES (5 rangées, et ses sprites) est retirée. L'introduction dure autant que celle du MSX (320 tours, START la saute) : ses 458 images sont réparties sur ces tours ; mesuré, 311 à 314 images entre le mode CASTLE et la cour dans les deux modes. La musique est celle du MSX.

## Simon 16 bits

Avec l'option 16B, Simon est dessiné en 16 bits sur tous les écrans du jeu (c'est un sprite : il ne dépend pas du décor).

- Source : les images de Simon de Castlevania-MD (`src/level.c`, listes de sprites 8 × 16), copiées une fois dans `assets/gfx16/simon/` par `tools/import_simon16.py` (image 96 × 44, bord gauche de Simon en x = 32).
- `tools/build_simon16.py` écrit `generated/simon16_md.c/.h` : un tableau de même disposition pour chaque tableau MSX (12 poses 24 × 32, 8 poses d'escalier, 2 morts 32 × 16, 18 poses au bouclier, 3 images d'attaque 64 × 32, 6 du fouet long, 6 du lancer, 4 accroupies, 6 accroupies par niveau). Le code de dessin ne change pas de géométrie : il choisit le tableau (`vk_simon_gfx_*` de `gfx16.c`).
- Placement : chaque image MSX est relue de son tableau généré ; l'image 16 bits est posée pieds sur la même ligne, corps sur les mêmes colonnes (boîtes centrées pour les poses 24 × 32 ; colonnes 16 à 31 pour les compositions, miroir pour la gauche).
- Correspondance : marche 1-2-3 → debout, marche 1, marche 2 ; saut et réception → accroupi ; recul → pose d'escalier descendant (provisoire : Castlevania-MD n'a pas de pose touché) ; escaliers → montée a, montée b, descente ; niveau 0 → fouet de cuir ; niveau 1 → chaîne longue ; niveaux de lancer → images de lancer NES ; lancer accroupi (absent du jeu NES) → images du fouet accroupi sans lanière.
- Lanière tendue : rééchantillonnée pour finir sur la dernière colonne de la lanière MSX (cuir : 44 → 41 pixels depuis le bord gauche de Simon ; chaîne longue : 60 → 48). La boîte de coup reste celle du MSX ; l'image ne promet ni plus ni moins de portée.
- Bouclier (absent du jeu NES) : dessiné par l'outil, une barre d'acier de 5 × 16 pixels tenue devant Simon (15 de haut accroupi). Validé sur planche par l'utilisateur.
- Mort : l'image de marche couchée face contre terre (variante A choisie par l'utilisateur), dans le canevas 32 × 16 du MSX.
- Palette : 13 couleurs pour toutes ces images, dans PAL3 entrées 1 à 13 (14 et 15 restent celles du portage). Simon 16 bits est toujours dessiné en PAL3. Le clignotement tracé (790C : silhouette d'une seule couleur une passe sur deux) réécrit ces 13 entrées avec la couleur MSX de la silhouette, puis les remet à la passe suivante, dans le même VBlank que la table des sprites.

## Limites du pilote

- Seuls les stages 1 à 3, la cour et l'introduction ont des graphismes 16 bits ; les stages 4 à 18, le titre, le logo, la carte et la fin restent en MSX même si l'option est active.
- Les ennemis, les bougies, les objets et les armes lancées restent les sprites MSX.
- Restent en pixels MSX dans une salle 16 bits : la clé blanche posée (carré 16 × 16 de mur MSX autour d'elle), les murs cassables, les nervures de voûte du stage 3.
- Les fissures et les escaliers gardent la forme MSX avec une texture 16 bits ; les vrais escaliers 16 bits (rampe) ne sont pas repris.
- Simon 16 bits : attaque sur escalier dessinée avec les images debout (comme en MSX) ; clignotement, bouclier, mort et fouet long non vérifiés en jeu image par image.
- Non vérifié : console réelle, combat du boss du stage 3 en mode 16 bits, marchands.

## Vérification

- `zzw_gfx16_hall_simon` (cadre du HUD de la salle 0), `zzx_gfx16_castle_entrance` (le scénario (h) entrée du château rejoué en 16 bits, 18/18), `zzy_gfx16_intro` (introduction 16 bits, durée, cour).
- Tous les scénarios MD qui démarrent par `boot_to_courtyard.inc` (46 sur 119) ont été rejoués une fois avec `vk_gfx16 = 1` : 43 passent tels quels ; les 3 autres (`t_item_effects`, `w_simon_blink_phase`, `zy_candle_flames`) n'échouent que sur des assertions propres au rendu MSX (ligne de palette de Simon, pixels du décor MSX). Ce contrôle a trouvé un défaut, corrigé : le fond blanc de l'objet 05 (registre 7) prenait sa couleur dans PAL0 ; il la prend dans PAL2 en mode 16 bits.
- `zzz_gfx16_blink` : silhouette de Simon 2 images sur 4 à l'écran en mode 16 bits.
- `tests/md_scenarios/zzv_gfx16_key_door.txt` : le scénario (f) clé blanche / porte / stage 2 rejoué avec `vk_gfx16 = 1` ; toutes ses assertions tiennent (19/19).
- Aperçus hors ROM : `python3 tools/build_gfx16.py ROM --preview DOSSIER`.
