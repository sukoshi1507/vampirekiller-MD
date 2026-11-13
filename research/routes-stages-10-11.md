# Routes des stages 10 et 11 : fiche testeur (PARTIELLE)

Etat : routes NON terminees. Ce qui a ete joue a la manette sur la ROM MD (harnais `tools/md_harness.py`, aucun poke de position) est dans
`tests/md_scenarios/zzq_stage10_route_0_3.txt` (stage 10, salles 0 a 3) et `zzq_stage11_route_0_1.txt` (stage 11, salles 0 a 1).

Important : l'entree par le selecteur de stage (Konami, B, A) active `Enemies_god` (1 apres l'entree). Tout ce qui est decrit ici a donc ete joue en mode
invulnerable ; les degats des ennemis (hommes-poissons, chauve-souris, fantomes) n'ont pas ete testes. Le stage 10 et le stage 11 n'ont de toute facon pas
d'objets pris dans les stages precedents (selecteur), alors que la suite peut en demander (voir stage 11).

## Stage 10 (9 salles : 0-1-2-3-4-5 en cercle, 1 monte vers 6, 5 monte vers 8, 6 et 7 se bouclent)

Liens : salle 0 : gauche = 5, droite = 1. 1 : haut = 6. 5 : haut = 8 (porte en (236,128)). Cle blanche visible en salle 0 en (48,48) (rebord haut gauche, pas
d'acces depuis le sol de la salle 0 : on y arrive probablement en quittant la salle 5 par la droite a un niveau haut, a verifier). Cles dorees en salle 1 (144,96) et salle 5.

Parcours verifie :
- Salle 0 (depart (32,144), rebord gauche) : marcher a droite jusqu'a X 96, attendre que la plateforme mobile arrive (X 97, elle va et vient entre 96 et 191), monter dessus
  en marchant, ne plus bouger, descendre a droite quand elle est a son maximum (X 180-190), marcher a droite : on sort a droite (salle 1, X 10).
- Salle 1 : le sol a gauche s'arrete a X 38 : sauter au bord (C+droite depuis X 32-34) pour franchir le trou ; marcher a droite jusqu'au bloc (X 120), sauter dessus
  (C+droite, arrivee Y 112, X 124) ; au bout du bloc (X 152) sauter a droite pour atteindre le rebord haut (Y 80, X 156) ; marcher a droite, sauter au bord (X 226) : salle 2.
  Sur le chemin on ramasse la cle doree (la cle apparait dans l'inventaire).
- Salle 2 (arrivee (36,144) sur le rebord gauche, eau mortelle) : attendre environ 390 images (la plateforme doit revenir vers X 75 en allant vers la gauche), sauter a droite sur elle
  (arrivee Y 168), avancer de 12 pixels sur la plateforme, puis MAINTENIR BAS (s'accroupir) : une stalactite arrete Simon debout vers X 104 ; accroupi il passe dessous et la plateforme
  l'emporte. Quand la plateforme est a son maximum (X ~190), se relever et sauter a droite : arrivee sur le rebord droit (Y 144, X ~234). Marcher a droite : salle 3 (X 10).

Piege : en salle 2, debout, Simon reste coince contre la stalactite et tombe a l'eau quand la plateforme poursuit sa route (mort). Sauter ne sert a rien.

Bloque (non resolu) : salle 3. La plateforme est au niveau du rebord (Y 143). Avec un depart a X 606 images apres l'arrivee on monte dessus (plateforme X ~35) et elle porte Simon
jusqu'a X 88, puis Simon est arrete (debout ET accroupi) par la carte de collision de la salle en RAM (`room_map` : colonne de tuiles pleines "1 2 / 10 11" en X 96-127, Y 96-143, absente
de la table ROM `vk_stage10_collision[3]` qui n'a qu'un rebord en (96..127, 96)). La plateforme continue sans lui, il tombe a l'eau. Je n'ai pas trouve d'autre chemin (saut depuis la plateforme
sans effet : Simon ne se deplace plus a droite en l'air a X 88). A verifier sur MSX : la stalactite de la salle 3 descend-elle aussi bas ? (celle de la salle 4 est connue : commentaire dans actors.c).
Les salles 4 a 8, la cle de la salle 0 et la porte de la salle 8 n'ont pas ete atteintes.

## Stage 11 (6 salles en ligne : 0-1-2-3-4-5, porte salle 5 en (236,128))

Cle blanche cachee dans le bloc (96,80) de la salle 1 (casse au fouet, cle en (96,80)).

Parcours verifie : salle 0 : marcher a droite (aucun obstacle, 2 ennemis lents) -> salle 1 (X 10, Y 176). Salle 1 : marcher a droite jusqu'a X 56 (le premier gradin de pierre arrete),
sauter dessus (Y 144).

Bloque (non resolu) : salle 1. Au-dela du gradin (X >= 64, haut Y 144) vient un mur plein en X 96 (tour de pierre Y 80 a 143, avec la cle doree posee sur le dessus). Depuis le gradin Simon est
arrete a X 88 (le saut apres le gradin ne l'avance pas en l'air : le sondage de mur X + 8 touche la tour tant que Simon est plus bas que Y 80). Le test existant `zzn_stage11` pose Simon en (92,112) pour
casser le bloc de la cle (le coup de fouet depuis X 88 ne l'atteint pas : essais X 84 a 90 sans cle, X 92 avec cle). Donc, sans pose de position, la cle blanche et la suite de la salle 1 semblent
inaccessibles avec le saut de base. Hypothese : il faut l'objet 0D (saut haut, voir `tools/playthrough_12_18_md.py`) rapporte d'un stage precedent ; a verifier contre la FAQ / la trace MSX.

## Ce qui reste a faire
- Stage 10 : franchir la salle 3 (ou prouver sur MSX que c'est un ecart du portage), salles 4 a 5, retour par la droite de la salle 5 vers la salle 0 haute pour la cle (48,48), salle 5 haut -> salle 8, porte.
- Stage 11 : verifier avec le saut haut ; salles 1 a 5, cle, porte.
- Rejouer sans `Enemies_god` (un `tap A` de plus apres le Konami le coupe, comme `zzm_enter10.inc`) pour tester les degats.
