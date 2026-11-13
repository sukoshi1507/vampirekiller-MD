# Stage 5 : objets 06 et 08, plateformes mobiles, objets de salle

Code : `sgdk/src/platforms.c` (902D, 90B8, 8524, 6BAF), `actors.c` (`vk_step_platforms`), `player.c`
(`vk_player_platform`, C439), `enemies.c` (06 et 08 étendus au stage 5). Tuile : `tools/build_stage5_platform_md.py`
sur une trace openMSX non versionnée (`scratchpad/stage5/out/R`, instantané `r1_D0D4`). Bougies et blocs :
`tools/build_candles_md.py`. Tests : `tests/test_platforms.c`, `tests/md_scenarios/zv_stage5_enemies.txt`.

## Objets vus (trace, 500 images par salle, Simon immobile)

| Salle | Drapeau | Vus | Fixes |
|---|---|---|---|
| 0, 5 | a0 | 08 | - |
| 1, 4 | 20 | 08, plateformes (salle 1 : 1, salle 4 : 2) | - |
| 2 | 80 | 06 | (80, 96) |
| 3 | 80 | 06 | (160, 96), (160, 160) |

Les autres entrées vues dans une salle juste après un changement forcé sont des restes de la salle précédente.
Aucun autre type en `C800`. Motifs de sprites 40-5C et 80-BC : identiques octet à octet au stage 4 (VRAM), mêmes
tuiles. Rien de nouveau au stage 5 hors plateformes.

## Prouvé (ROM et trace)

- Objet 06 : positions X égales à la trace, par pas de 60 images, salle 3 (`168,168`, `143,127`, `184,121`, `157,113`,
  `154,154`, `173,173`) et salle 2 (`121, 137, 96, 124, 135, 93, 104, 146`), Simon immobile en (232, 96).
- Objet 08 : poses 73 et 74 vues (motifs 50 et 58) : le « déduit » de `stage4-enemies.md` est confirmé.
- Plateformes, table `906C` (salle, Y, X, pas, longueur) : salle 1 `5F 60 +1 40` ; salle 4 `5F 20 +1 30`, `5F B8 -1 38`.
  Enregistrements `C598`, `C59F`. Un pas au chargement (`6398`), puis un par passe (`5C80`, `90B8`) ; la passe du
  demi-tour ne bouge pas. Bornes : salle 1, 96 à 159 ; salle 4, 32 à 79 et 129 à 184. Trace : mêmes bornes.
- Sprites : deux cellules X et X + 10h, SAT Y - 1 (`9112`), motifs D0 couleur 2 et D4 couleur 44 (`90F1`, `913F`).
  Sur MD, le bloc partagé est sans D0 : la tuile composée prend la cellule libre du motif 60.
- Simon (`8524`, C439) : posé si Y - pY < 8 et si X - 7 - pX < 20h ou X + 7 - pX < 20h. Le sol vaut alors plateforme
  (`6B57`, `6D2B`, `6F3D`, `6FEB`). À l'atterrissage, `Y &= F8` (`6F6A`), soit 96.
- Transport (`6BAF`) : Simon suit le pas, sauf mur (`7BA9`, `7C05`). Au demi-tour la plateforme reste, Simon suit le
  nouveau sens : l'écart perd ou gagne 1. Trace : 5 vers la droite, 4 vers la gauche, sur plusieurs tours.
- Trace du saut depuis la plateforme (touche haut) : X fixe en l'air, arc 96 à 64, retombe à 96, transport repris.
  Trace de la sortie à droite : chute à X - pX = 39.
- Forcer le stage demande `D002` = 1 (voir `stage4-enemies.md`).

## Déduit

- `vk_platforms_probe` ignore le test de garde (`8524`, C420 = 4 et C428 < 3) pendant la fin d'un saut : le port passe
  en mode 4 après le pas 19, le MSX reste en état 1 (`research/jump-compare.md`). Sans cela, Simon traverse la
  plateforme au retour de saut. Comportement égal à la trace, implémentation du port.
- **Chute d'un rebord ou d'un saut sur une plateforme (stage 10 salle 4, openMSX 2026-11-13, `tools/platform_fall_trace.tcl`, comparé au port par `tools/platform_fall_compare.py`)** : par passe de 2 images, `90B8` avance la plateforme à la première image, puis `6B39` (sonde `8524`, transport `6BAF`, Simon) tourne à la seconde. Chute depuis le sol (`6BA5`, C420 = 4) : `8524` rend la main tant que C428 < 3 (Y 144, 146, 150, 156 sans sonde), puis Y + 6 par passe ; la sonde voit Y - 167 < 8 (Y 168 ou Y 170-174), `6F6A` pose Simon à `Y &= F8` (168), C420 := 0, et le transport commence à la passe suivante (+2 de marche +1 de la plateforme). 210 cas (Y de départ 138 à 172, X du bord gauche -9 à +40, deux phases de passe, 36 sauts) : le port suit Y et X image par image, y compris les chutes qui manquent la plateforme et le Y 164-166 que le saut des 3 premières passes laisse passer. Résidu connu : après un saut qui se termine sur la plateforme, le port répartit le +3 de la passe sur deux images (+2 / +1) quand la MSX le fait en une (écart d'un pixel d'une image, jamais cumulé ; essai de caler le pas des plateformes sur l'horloge de Simon : décalage permanent de 2 px, abandonné). Scénario `zzm_stage10_platform_fall.txt`.
- Le mur qui arrête le transport n'a pas été observé en trace (aucun mur sur le chemin des plateformes en salle 1).

## Provisoire, ouvert

- Le temps arrêté (objet 0A, `D010`) ne gèle pas les plateformes dans le port ; `5C80` n'a pas été suivi sur ce point.
- Le passage d'une salle à l'autre efface les plateformes tout de suite ; dans la trace les sprites restent 12 images.
- Bit 7 du drapeau (salles 0, 2, 3, 5) : rien de visible en trace, canal non porté, comme au stage 4.
- Blocs et bougies générés mais non rejoués à la main. Clé, porte et sortie du stage 5 : rejouées (voir Vérifications). Pas de comparaison
  pixel. Couleurs : la tuile compose 2 et 4 (index 6 en recouvrement), vérifiée par capture seulement.
- Un second saut par le sélecteur a laissé un dessin blanc immobile en (47, 80) en salle 4 ; absent en entrant
  directement. Même famille que les parasites (64,144) et (192,144) du debug, cause non cherchée.
- L'instantané `r4` de la trace R montre l'image de la salle 3 (le plan C montre bien la salle 4) : non expliqué, sans
  effet sur les objets relevés.
- Table `906C` : le stage 10 (salles 0, 2, 3, 4, motifs D8, DC) est porté : `stage10-level.md`. `8F9A` : trois objets `C580` en stage 6
  salle 1, à lire pour le stage 6.

## Clés dorées et coffres

Les 7 objets fixes du stage 5 (0x17 et 0x19, salles 1 à 4) sont générés. Décor comparé au MSX : salles 0 à 4 conformes,
hors sprites et dessins parasites. Le parasite (192,144) recouvre en partie la clé de la salle 4 quand on arrive par le
sélecteur. Salle 5 non comparée : le seul instantané `r5` montre déjà la salle 4, Simon y étant renvoyé par un coup.

## Chute

Corrigée dans `player.c` : un pas par passe de 2 images, 2, 4, 6, 6, comme la trace (salle 4, Y 104, 106, 110, 116, 122,
128 ; le MSX ajoute parfois une image de retard). À la fin de la table du saut, le port poursuit à +6 par passe, comme
`research/jump-compare.md` l'a relevé. Scénario `zw_fall_speed`.

## Vérifications (agent `zy`, scénarios `tests/md_scenarios/zy_*`)

4. **Trajectoire de l'objet 08** (`zy_d_stage5_obj08`). Salle 0, Simon en (232,96) : durée de vie complète (89 passes du premier
   pas jusqu'à la sortie en X 238) comparée passe par passe à la trace `stage5/out/R` (Y, X, vitesse Y) : 0 écart. Apparition par la
   droite (Simon en (100,96) côté MD, trace : objet né en 240) : 80 passes comparées, 0 écart. Prouvé. Le scénario relit 9 points
   de la trace (toutes les 10 passes). Le décalage de 2 px entre le mur qui arrête Simon (voir 7) n'a pas de lien.
5. **Clés dorées et coffres** (`zy_f_stage5_chests`). 7 objets décodés de la ROM : salle 1 clés (96,112) et (160,144) ; salle 2 coffre
   (160,112) 09 ; salle 3 coffres (176,80) 13 et (176,144) 1E ; salle 4 clés (80,144) et (192,144). Tables du port égales au décodage.
   **Écart corrigé** (`a0f4c79`) : le contenu 13 du coffre de la salle 3 vaut **5000 points** (`8E42` : DE = 5000h, `44EC` ajoute au
   score ; `8E4B` : 14 = 1000 points) ; le port ne faisait rien. Scénario : clé posée par `poke`, coffre (176,80) ouvert, contenu pris,
   score + 5000 ; coffre (176,144) : `C701` bit 3 (eau bénite). Non-réapparition : déjà vue au stage 4 (`zy_e`), non rejouée ici.
   Prise de clé : mêmes routines qu'au stage 4. Pas de trace MSX.
6. **Clé blanche, porte, sortie** (`zy_h_stage5_key_door`). Bloc (192,64) en salle 3 brisé depuis (176,96), clé prise ; porte de la
   salle 5 (côté gauche, (12,80)), Simon en (34,128) : ouverture en 27, clé consommée ; sortie vers le stage 6 : arrivée en salle 0
   (232,128), comme la table de `MSX2_EXITS_VERIFICATION.md`. Déduit (pas de trace du stage 5 en jeu complet).
7. **Chute après sortie de corniche et saut long** (`zy_i_air_control`). Nouvelle trace openMSX (`scratchpad/checks45/lj`, plan :
   `poke D000/D002/D001/C000`, Simon posé en (232,96) salle 2, `hold U+L 36`) comparée à la MD :
   - Y de la chute après sortie en marchant : 96, 98, 102, 108, 114, 120, 126, ... (+6) identique à la trace `J` (salle 1) et à la
     MD (prouvé) ; saut long de la corniche y 96 au sol y 128 : Y par passe 96, 90, 84, 79, 74, 70, 67, 65, 64, 65, 67, 70, 74, 79, 84,
     90, 96, 102, 108, 114, 120, 126, 128 : **identique** à la trace, y compris la poursuite à +6 après la fin de la table.
   - **Écart trouvé et corrigé** (`f531c72`) : sur MSX, `C421` est lu une seule fois à la première passe du saut (`6CC0`, `6CCE`-`6CE5` :
     1 tout droit, 2 gauche, 3 droite) puis gardé jusqu'à l'atterrissage ; trace : touches relâchées à l'image 837, `C421` = 2 jusqu'à
     la fin, X continue de reculer de 2 px par passe (188 à l'arrivée). Une chute depuis une corniche (`6BA5`, `C421` = 0) ne lit
     aucune touche : trace `J`, DROITE tenu, X reste à 149 pendant toute la chute. Le port déplaçait Simon avec les touches dans
     tous les états aériens (X dérivait en chute, s'arrêtait au relâchement). Correctif : direction mémorisée au décollage
     (`player.c`, 2 variables statiques), nulle pour une chute. Le test natif `room_transitions.c` partait de Simon en l'air et comptait
     sur cette dérive ; il part maintenant sur la corniche de la porte (x 228, y porte + 48).
   - Reste : le MD s'arrête en X 200 contre un mur là où le MSX va jusqu'en 198 (2 px, mur de la salle 2 pendant y 79 à 102) ; arrivée
     MD en 190 contre 188. Cause non cherchée (départ à 1 px par image côté MD, une image de latence au décollage). Non vérifié :
     direction mémorisée quand on change de touche pendant la première passe, et saut depuis un escalier (même variable, non tracé).
