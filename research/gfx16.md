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

## Limites du pilote

- Seuls les stages 1 à 3 ont des graphismes 16 bits ; les autres salles, la cour et les écrans restent en MSX même si l'option est active.
- Simon, les ennemis, les bougies et les objets restent les sprites MSX.
- Restent en pixels MSX dans une salle 16 bits : la clé blanche posée (carré 16 × 16 de mur MSX autour d'elle), les murs cassables, les nervures de voûte du stage 3.
- Les fissures et les escaliers gardent la forme MSX avec une texture 16 bits ; les vrais escaliers 16 bits (rampe) ne sont pas repris.
- Non vérifié : console réelle, combat du boss du stage 3 en mode 16 bits, marchands.

## Vérification

- `tests/md_scenarios/zzv_gfx16_key_door.txt` : le scénario (f) clé blanche / porte / stage 2 rejoué avec `vk_gfx16 = 1` ; toutes ses assertions tiennent (19/19).
- Aperçus hors ROM : `python3 tools/build_gfx16.py ROM --preview DOSSIER`.
