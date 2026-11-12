# Budget du dessin de salle pendant le gel (etat 9)

Le gel de changement de salle dure N images (hall : 20) et lance le dessin ROOM_DRAW_FRAMES = 7 images
avant la fin (main.c, room_freeze_step). Le dessin tourne dans un seul tour de boucle ; si il depasse
7 VBlanks, le gel s'allonge (hall : 21 images, test n_room_transition_freeze).

## Mesure (avant)

Instrumentation temporaire : compteur de lignes (vtimer * 262 + compteur V du VDP, 0xC00008) lu entre
chaque etape, lecture par le harnais (tableau volatil, retire ensuite). Hall salle 0 -> 1, Genesis Plus GX :

| etape                                              | lignes | octets      |
|----------------------------------------------------|-------:|-------------|
| flush DMA, reset sprites, clearPlane               |     59 |             |
| tuiles de la salle (192 tuiles, afficheur coupe)   |    113 | 6144 (54 o/ligne, boucle CPU) |
| palettes, carte 32x22 (CPU), textes                |    140 |             |
| afficheur rallume                                  |     13 |             |
| tuiles Simon (144 tuiles, afficheur ALLUME)        |    225 | 4608 (20 o/ligne) |
| palette + fouet/attaque/flammes/accroupi/butin...  |    497 | ~7 Ko (afficheur allume) |
| HUD, overlay                                       |    190 |             |
| objets, bougies, cellules ennemies (CPU)           |    565 | dont 373 pour vk_enemy_room_cells |
| total                                              | ~1824  | = 6,96 images, budget 1834 |

Cause : le budget etait consomme a 99 %, donc tout octet de code ou de donnees ajoute dans un chemin
du dessin (ou un decalage d'alignement) faisait passer a 8 VBlanks. Le cout dominant est l'ecriture
CPU des tuiles dans la VRAM : avec l'afficheur allume le VDP n'accepte qu'environ 16 octets par ligne
(FIFO), 3 fois moins qu'afficheur coupe, et la boucle CPU seule plafonne a 54 octets par ligne.
`vk_show_room` rallumait l'afficheur apres la carte, donc plus de 5 images d'ecritures lentes.

## Correction

Pendant un gel seulement (`vk_fast_room_draw`, main.c), le dessin garde l'afficheur coupe de bout en
bout (`vk_room_display_on` en fin de `show_hall_room` / `vk_show_courtyard`) et les chargements de
tuiles (`VK_TM`, room_view.h) passent par DMA bloquant (~170 octets par ligne, la boucle CPU l'etait
a 54). Le contenu de la VRAM final est identique. Les autres dessins (reapparition, debut de stage,
saut de stage) gardent l'ancien chemin : leurs scenarios sont calibres sur sa duree (le tour de boucle
qui dessine compte pour un pas de jeu, une duree differente decale les pas de jeu de quelques images :
verifie sur zzj_ghost5 et zzb_stage8).

## Mesure (apres)

`room_draw_vblanks` (main.c) = VBlanks du dernier dessin de gel :

| dessin            | avant | apres |
|-------------------|------:|------:|
| hall salle 1      |     7 |     2 |
| cour salle 1      |     5 |     2 |

Marge : 5 images (~650 000 cycles 68000) au lieu de 0, pour un gel de 7 images de budget.
Test : tests/md_scenarios/n_room_draw_margin.txt exige room_draw_vblanks <= 4 (marge >= 3 images).

## Rejeu 12-18 (tests/run_playthrough_12_18_md.sh)

Le rejeu image exacte ne passe plus (checkpoint stage 12 salle 10 -> 7 manque), et c'est attendu :
l'ancien dessin depassait le budget d'une image selon la phase du VBlank (le tour de dessin finissait
a la 8e image au lieu de la 7e : room_freeze restait a 7 pendant le dessin puis tombait a 0, le gel
durait 21 images au lieu de 20 dans certaines salles). Le rejeu a ete enregistre avec ces 21 images ;
la nouvelle duree est toujours exactement celle de la table (20). Mesure : a l'entree de la salle 10,
le jeu reprend 1 image plus tot, donc Simon avance de 2 pixels en avance, et la trajectoire diverge.
Le rejeu doit etre re-enregistre (RECORD=1) ; la recherche de route s'est bloquee a l'etape 13 avec
la nouvelle duree (aucun candidat pour la salle 1 -> 2), a reprendre par l'auteur du rejeu.
Avec `vk_fast_room_draw = FALSE` (ancien chemin) le rejeu repasse a l'identique.
