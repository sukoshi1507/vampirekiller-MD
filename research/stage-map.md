# Carte du stage, objet 0F

Code ROM lu en banque 2 (`9552`-`98D0`), comparé aux traces openMSX
`scratchpad/objects-0F-10-11/trace_0F_*` (hors dépôt). Port : `sgdk/src/stage_map.c` (rendu, testé
par `tests/test_stage_map.c`), `sgdk/src/stage_map_view.c` (affichage), tables générées par
`tools/build_stage_map_md.py`.

## Ouverture et usages

- `8E33` : prendre 0F met `C701` bit 7 et `C70F` = 3. La mort garde le bit (`70DC`).
- `9552` (état 5, `CE00` = 0) : F2 avec `C701` bit 7 et `C70F` > 0 : `C70F` − 1. À 0, le bit 7 est
  effacé et la case redessinée (`8EE6`), mais ce troisième usage montre encore la carte.
- Son 19 à l'ouverture. Jeu figé tant que `CF38` != 0. Second F2 : salle redessinée (`95B3`),
  sprites remis.

## Dessin

1. Sprites coupés, sauf ceux du HUD ; noir de Y = 32 à 211 (`9585`) ; panneau couleur 3, x 24..231,
   y 48..173 (`9591`).
2. Pour chaque salle (`95F6[D000]` salles) : vignette 32 x 22 opaque, une cellule = un pixel, lignes 2
   à 23 de la carte. Place : `9757[9695[D000][salle]]`, paires (y, x) ; les vignettes se recouvrent
   d'une ligne.
3. Couleur (`97AE`) : cellule ≥ 14 → 0 ; classe `97EC[cellule]` : 1, 2 → 14 ; 3 → 14 si `D002` vaut 0
   ou 5 ; 4 → 14 si `D002` vaut 5 ; sinon 0. `D002` = table `5E6A[D000]`.
4. Blocs cachés encore là (`9609`, `964C`) : 4 x 4 cellules de valeur 1, ligne (y − 16) / 8, colonne
   x / 8. Salle courante : `C470` ; autres salles : flux du stage.
5. Simon (`9815`) : 4 x 4, données `984B`, en x + `C427` / 8, y + (`C425` − 40h) / 8 signé.
6. Sortie (`9853`) : 8 x 7, table `98AC[3 x D000]` = (y, x, sens), données `9874` ou `9890`.

## Choix du port

- F2 devient A pressé seul. Les raccourcis de debug avec A (A+gauche, A+droite, A+START) restent
  utilisables tant que A n'est pas pressé en premier avec l'objet 0F.
- **Écart** : la ROM dessine en plusieurs passes (`CF38` : 1 à 4 images, 2 à environ 22 images). Ici, tout
  est préparé en RAM puis écrit en une fois, le délai dépend du nombre de salles.
- La carte prend les tuiles 16..223 de la salle (sauvées puis restaurées, sans celles du HUD) et les
  lignes 4..27 du plan A (sauvées puis restaurées).
- START ne fait rien pendant la carte (la ROM n'a pas été lue sur ce point : provisoire).
- `tests/md_scenarios/zp_stage_map.txt` : les 3 usages dans la cour.
