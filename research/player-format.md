# Simon, collisions et saut — jalon 3

Etat initial du jalon 3. Le jalon 4 ajoute les trois cartes exterieures aux
comparaisons, pour un total de 53 872 cas, et un parcours avant le hall.
Voir `intro-format.md` pour l'enchainement courant.

Le mode accessible avec B permet de marcher et sauter dans la salle 1/0.
Il combine des primitives traduites du Z80 et un controleur provisoire en C.
Il ne reproduit pas encore le dispatch complet du personnage.

## Graphismes et placement

`56E1` selectionne les banques 0B/0C/0D et charge deux flux de motifs via
les tables `A281/A2D1`. Le decodeur `46EB/46F1` interprete :

| Commande | Effet |
| --- | --- |
| 00 | Fin du flux |
| 01..7F | Repeter l'octet suivant n fois |
| 80 | Continuer au pointeur little-endian suivant |
| 81..FF | Copier les n & 7F octets suivants |

Les poses retenues ont deux masques de 16 x 16 par partie du corps. Un masque
occupe 32 octets : 16 lignes de la colonne gauche, puis 16 de la colonne droite.
`7837/7899` utilise les tables `7985/79D5` pour placer les masques autour de
`C427` (x) et `C425` (y). Le decalage d'un pixel selon l'orientation est conserve.
Le saut des motifs 6/16 applique la correction de six pixels de `78B2..78C0`.

`790C` donne les couleurs 1 puis 42 dans le cas normal retenu. Le bit CC
combine les deux couches : une intersection utilise la couleur 3.
Les couleurs 1/2/3 de la palette originale sont conservees.

| Pose MD | Motifs inferieur / superieur |
| --- | --- |
| 0..2, droite | 0/0, 1/1, 2/2 |
| 3..5, gauche | 10/15, 11/16, 12/17 |
| 6, saut droite | 6/0 |
| 7, saut gauche | 16/15 |

Les motifs MD font 24 x 32 pixels pour conserver le decalage d'orientation
sans couper de pixels. Un sprite materiel de 3 x 4 tuiles suffit. Le placement
est `(x-8,y-31)` et les tuiles sont emises en ordre par colonnes.

## Collisions

Le lookup `7D2F` opere sur la carte complete de 32 x 24 octets a `D100` :

`index = (((y - 16) & 255) & 248) * 4 + (x >> 3)`

Un index au-dela de 767 retourne zero. Le mode personnage conserve la carte
complete, distincte du tampon visible tronque de l'explorateur.

`7C5E` juge solide un identifiant pour lequel `(tile-1) & 255` est inferieur
a la limite du stage dans `7C78`. Le cas `CE00=6` utilise la limite 6.
Le stage 1 utilise la limite 4 : indices 1 a 4 solides.

- `7B88` : sol a `(x+5,y)` et `(x-5,y)`; aucun appui si y >= 208.
- `7BA9` : cote droit a x+8, y-2, y-10, y-18 et y-26.
- `7C05` : memes sondes a x-8.
- Pour l'etat accroupi 2, la derniere sonde laterale est omise.

Les coordonnees conservent le rebouclage sur huit bits. Le blocage aux bords
de la salle est propre au prototype : les sorties ne sont pas encore portees.

## Mouvement

`6C2F/6C53` utilise une vitesse normale de 0200 en 8.8, soit deux pixels par
tick. Le prototype reprend cette vitesse et l'ordre des poses de marche.
Le saut utilise les 19 valeurs de `6D95` :

`-6,-6,-5,-5,0,-4,-3,-2,-1,0,0,1,2,3,4,5,5,6,6`

Le zero intermediaire appartient aux donnees originales. `vk_jump_step`
reprend l'increment du compteur, le plafonnement de l'index et l'addition
sur huit bits de `6D4D`. Le test porte sur les phases utiles avant
rebouclage artificiel a 255. La chute utilise 2,4,6,6 de `6F81`.

Les entrees d'etat, leur delai initial, les interactions et les effets
annexes ne sont pas tous traduits. L'equivalence des primitives ne prouve
pas une equivalence image par image avec le jeu complet. Le prototype ne
traite pas les escaliers, les attaques, les sorties ou les equipements.

## Verification

`verify_player_primitives.py` compare le C natif a l'execution des veritables
instructions de la ROM sur Z80. Les cartes des 153 entrees alimentent les tests.

| Primitive | Cas compares |
| --- | ---: |
| Classification, 19 stages et 2 modes | 9728 |
| Adressage de carte | 3672 |
| Appui au sol | 7344 |
| Sondes laterales | 29376 |
| Calcul elementaire du saut | 2960 |
| Total | 53080 |

Tous concordent. `verify_play_mode.py` pilote la ROM MD dans Genesis Plus GX
et lit son etat RAM : 60 pixels en 30 images a droite, 20 pixels en 10 images
a gauche, saut, retour a y=192 et orientation gauche. Cinq images stables
correspondent pixel par pixel au fond et aux masques reconstruits depuis
la ROM. Ce test ne mesure pas les delais de controle du jeu MSX complet.
