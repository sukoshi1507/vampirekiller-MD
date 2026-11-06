# Formats de salles retrouves — jalon 2

## Chargement de la carte

Le code `4F83` lit le stage a `D000` et la salle a `D001`, puis entre dans
`4FAF`. Il selectionne les banques `0B/0C/0D` pour les fenetres `6000/8000/A000`.
Le chemin decrit ici correspond a `C41A=0`. Le chemin alternatif utilise une
autre carte et un autre jeu de metatuiles; il n'est pas porte.

1. Lire l'octet `6000 + stage` : index initial du stage dans la table de salles.
2. Ajouter le numero de salle, doubler et lire le pointeur little-endian dans
   la table commencant a `6013`.
3. Lire 48 indices de metatuiles, organises en 8 colonnes et 6 lignes.
4. Lire le pointeur du jeu de metatuiles dans `7EBB + 2*stage`.
5. Chaque metatuile contient 16 indices de tuiles, soit 4 x 4 elements.
6. Developper les metatuiles dans le tampon `D100-D3FF`, 32 x 24 octets.
7. Restaurer les banques `01/02/03` et leurs miroirs `F0F1-F0F3`.

Les 19 offsets commencant a `6000` incluent le stage 0. Les trois salles de ce
stage ne sont pas exposees dans l'explorateur. Les stages 1 a 18 referencent
153 entrees. Le pointeur qui suivrait la 156e entree globale est nul.

La routine de rendu `4F91` commence a `D140`, saute donc les deux premieres
lignes de tuiles, et copie 22 lignes de 32 tuiles vers les coordonnees `(0,32)`.
Le fond visible fait 256 x 176 pixels. Les 32 lignes superieures sont reservees
au HUD; l'explorateur affiche a cet endroit ses propres informations.

## Chargement des tuiles

`564C` selectionne les banques `04/05/06`. Pour les stages a partir de 13,
`538C` remplace les deux dernieres par `07/08`. La table a `5742 + 2*stage`
donne le pointeur des graphismes. Le code charge 191 tuiles via `4A66`.

Chaque tuile fait 8 x 8 pixels, quatre bits par pixel, 32 octets consecutifs.
Dans chaque octet, le demi-octet haut est le pixel de gauche. La tuile zero
est la case vierge; les donnees chargees occupent les cases 1 a 191.

Ce format est directement representable dans le VDP Mega Drive : les huit
lignes d'une tuile deviennent huit mots de 32 bits, chacun contenant huit
indices de couleur. Les ressources C utilisent l'ordre big-endian explicite.
Le code SGDK ajoute `TILE_USER_INDEX` aux indices de la carte.

## Palettes

Les routines `5727`, `570D` et `5780` selectionnent les banques `09/0A` dans
les fenetres `8000/A000`. `483E` interprete une liste de triplets :

| Octet | Sens |
| --- | --- |
| 0 | Index couleur 0..15; FF termine la liste |
| 1 | Rouge sur les bits 4..6, bleu sur les bits 0..2 |
| 2 | Vert sur les bits 0..2 |

L'ordre des modifications retenu est :

1. Palette commune, liste a `BF88`.
2. Palette du stage, pointeur dans `BEA7 + 2*stage`.
3. Palette de la salle : table de stage via `9AB0 + 2*(stage-1)`, puis
   second pointeur du couple de quatre octets de la salle.

Certaines couleurs sont conservees par le jeu sans etre redefinies dans ces
listes. L'extracteur verifie qu'elles ne sont pas utilisees dans le fond de la
salle, puis les remplit de noir. Le manifeste les nomme
`undefined_but_unused_colors`. Il ne suppose pas une valeur heritee pour une
couleur effectivement utilisee.

La CRAM Mega Drive encode les composantes sous la forme `0000 BBB0 GGG0 RRR0`.
Les trois bits de chaque composante sont conserves. Les PNG d'extraction
utilisent une conversion RGB333 lineaire; les captures de l'emulateur utilisent
sa conversion RGB565. La verification tient compte de cette difference.

## Exemple : stage 1, salle 0

| Donnee | Adresse CPU | Offset dans la ROM |
| --- | --- | --- |
| Carte de 48 metatuiles | `620B`, banque 0B | `1620B` |
| Table des metatuiles | `80B1`, banque 0C | `180B1` |
| Graphismes, 6112 octets | `7220`, banque 04 | `09220` |
| Palette commune | `BF88`, banque 0A | `15F88` |
| Modification du stage | `BEE3`, banque 0A | `15EE3` |
| Modification de la salle | `9FFE`, banque 09 | `13FFE` |

La derniere liste traverse la limite de fenetre `9FFF/A000`. La lecture doit
changer de banque physique en franchissant cette limite; une simple tranche
calculee dans une banque unique ne suffit pas en general.

## Verification par execution de la ROM

`verify_z80_maps.py` execute le veritable code `4F83/4FAF` avec le moteur Z80
de kosarev/z80 1.2.0. Le harness installe uniquement la ROM, la RAM, les
selecteurs de banques, une pile et un point d'arret de retour. Il n'emploie
aucun remplacement des instructions de la routine.

Pour les 153 entrees, les 768 octets produits par la routine correspondent
exactement a la carte de l'extracteur. La selection puis la restauration des
banques est egalement verifiee. Les conditions sont `C41A=0`, interruptions
externes absentes et registres initiaux du harness. Cela ne mesure pas la
temporisation du jeu et ne constitue pas une execution complete MSX2.

Les 152 fonds pris en charge sont aussi affiches par la ROM SGDK dans Genesis
Plus GX. La zone `(0,32)-(256,208)` correspond pixel par pixel au rendu attendu
pour chacun de ces fonds. Ce controle couvre la conversion, le chargement VDP,
les palettes et la navigation du programme MD.

## Limites identifiees

Le stage 18 / salle 9 reference les tuiles 192..245. La carte est bien identique
a celle produite par la routine originale; c'est son alimentation graphique
complete qui reste a comprendre. Le PNG de diagnostic remplace les cases
non resolues par du magenta. Cette entree ne figure pas dans la ROM MD.

Le fond peut etre modifie par le jeu apres son chargement. Les objets, portes,
coffres, destructions et animations ne sont pas reconstruits ici. La presence
d'une entree de table ne prouve pas son accessibilite en jeu normal.

Le champ `C000` reste l'etat principal et `C001` un sous-etat. Le bloc `4257`
consulte plusieurs indicateurs et bascule vers d'autres etats; il constitue
une piste centrale pour la prochaine analyse de la boucle de jeu.

## Sources techniques

- SGDK 2.11 : https://github.com/Stephane-D/SGDK/releases/tag/v2.11
- Marsdev v1.0.0-rc1 : https://github.com/andwn/marsdev/releases/tag/v1.0.0-rc1
- z80dis : https://github.com/lwerdna/z80dis
- Emulateur CPU Z80 : https://github.com/kosarev/z80
- Genesis Plus GX : https://github.com/libretro/Genesis-Plus-GX

Les algorithmes propres au jeu ont ete derives de la ROM fournie. Les listes
d'instructions correspondantes sont dans `loading-and-states.asm`.
