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

## Images perdues en jeu dans les salles de boss (2026-11-13)

Le test de fumee (`tools/smoke_rooms_md.py`, 153 salles) comptait 4 a 8 salles avec 1 ou 2 images perdues (tour de boucle qui ne finit pas dans l'image : la passe ne
bascule pas `itemPhase`). **Le dessin de salle n'est pas en cause** : les tours perdus sont des tours de jeu normaux, mesures par une instrumentation temporaire (lignes
`vtimer * 262 + compteur V` lues entre les etapes du tour, par le harnais) ; un tour normal fait ~130 lignes sur ~237 disponibles. Les trois couts mesures :

| salle | tour lent | cause | correction |
|---|---|---|---|
| stage 9 salle 7 (boss 9) | 340 lignes | `boss9_flash_cells` : tuiles du clignotement des walkers recalculees quartet par quartet (96 mots x 8) et ecrites par CPU, afficheur allume : ~200 lignes | calcul par masques (nibbles 12 -> 8, 14 -> 2, verifie sur 200 000 mots), DMA en file (`actors.c`) |
| stage 18 salle 9 (Dracula) | 130 a 185 lignes dans `vk_dracula_view_pass` | recomposition du buste (4 couches x 128 mots) a chaque changement de couches : 50 a 120 lignes | les 65 listes de couches possibles (sous-ensembles ordonnes de 1..4, `dracula.c`) sont composees par `tools/build_dracula_background.py` ; le jeu met un DMA en file depuis la ROM (`dracula_view.c`) |
| stage 9 salle 8 (avant le boss), toute salle ou l'on marque | +100 lignes quand le score change | `vk_msx_text` : composition de 12 caracteres a chaque changement, 6 divisions 32 bits, ecriture CPU des tuiles | une division 32 bits + chiffres 16 bits, seuls les caracteres changes sont composes et mis en file DMA (`hud.c`, `msx_text.c`) |

Faux positif corrige : le tour qui ferme le marchand (stage 7 salle 8) laisse `vk_walls.shop` a 0 alors que le monde est encore fige : le test ignore l'image suivant un tour de marchand.
Apres correction : 153 salles, 0 image perdue ; le test de fumee echoue desormais s'il en trouve une (`run_smoke_rooms_md.sh`). Les dessins de reapparition et de debut de stage
gardent l'ancien chemin lent (non en cause, scenarios `zzj_ghost5`, `zzb_stage8` inchanges).

## Images perdues pendant le rejeu 12-18 : mesure, correctifs, test (2026-11-13)

Constat : le rejeu image par image (`tests/run_playthrough_12_18_md.sh`) se desynchronisait des qu'on changeait la disposition du code (4 octets
dans VKBoss, une fonction de plus : autre inlining LTO). Cause : des tours de boucle de jeu normal (stages 13-15, ennemis actifs) finissent a quelques
lignes de la fin de l'image. Si le tour finit apres le debut du VBlank, l'attente de SGDK (`VDP_waitVBlank(forceNext)`) sort du VBlank et attend le
suivant : une image entiere perdue pour quelques lignes, et selon l'alignement du code. Le test de fumee ne la voyait pas (aucun ennemi actif, pas d'actions).

### Instrumentation (toujours presente, `sgdk/src/profile.h`, `vk_turn_sync` de main.c)

Compteurs en RAM mis a jour a la fin de chaque tour (avant `SYS_doVBlankProcess`, via `vk_turn_sync`) : `vk_turns`, `vk_last_slack` (lignes restantes avant le
VBlank, negatif = lignes dans le VBlank), `vk_late_turns` (tours finis apres le debut d'un VBlank), `vk_absorbed` (ceux finis dans ce VBlank, voir plus bas),
`vk_late_plain` (images perdues hors dessin de salle), `vk_draws` (dessins : `show_hall_room`, `show_courtyard`, fenetre du marchand). Le rejeu les lit apres
chaque image (`LateTurns`, tools/playthrough_12_18_md.py) : il **echoue** sur une image perdue hors dessin et imprime la marge (`slack`) des tours ordinaires.
Le mode `--record` les relit aussi pendant la recherche (resume par salle). Les dessins de salle (gel, porte du boss, debut de stage) ont le droit de prendre leurs images.

Mesure par etape (ROM de mesure : `make GDK=... EXTRA_FLAGS="-std=c99 -fno-late-combine-instructions -DVK_PROFILE"`) : `VK_MARK(i)` range `vtimer * 262 + compteur V`
dans `vk_prof[i]` entre les etapes du tour (main.c, actors.c, enemies.c, boss.c) ; au tour en retard les marques sont copiees dans `vk_prof_late`, l'outil les liste
(`input`, `tick+whip`, `enemies` ... `show.*`, `ef.*`, `boss.*`) et, sur les tours a l'heure, le cout moyen et maximal de chaque etape. Les marques ne coutent rien
dans la ROM normale. Pour sonder une salle sans rejeu : entrer par RAM (`Game.entry`) et jouer quelques centaines d'images avec `LateTurns` (quelques secondes par salle).
Piege rencontre : le tour *suivant* un tour marginal est celui que SGDK signale (delta de `vtimer` entre deux entrees, ou le V counter etait 222/223) ; le cout est a lire
sur le tour qui finit pres du VBlank, d'ou l'etat `crossed` (VBlanks commences pendant le tour) de `vk_turn_sync`.

### Chemins chauds trouves et corriges (lignes de 489 cycles ; un tour dispose de ~250 lignes depuis la fin du travail du VBlank)

| chemin | avant | apres | correctif (comportement visible inchange) |
|---|---:|---:|---|
| barre d'energie du HUD (`vk_hud_update`, a chaque coup sur le boss) | 115-140 | ~2 | tuiles en DMA en file ; `prepare_bar` au dessin du HUD |
| premier tour d'une salle : champs du HUD changes (stage...) | 112 | ~2 | `hud_status` fait pendant le dessin de salle |
| premier tour de la salle de boss : tuiles du boss (CPU, afficheur allume) | ~200 | 0 | chargees avec les cellules de la salle, pendant le dessin (`room_cells`) |
| cellules graphiques tardives (stages 12-18), recherche lineaire par sprite (jusqu'a 32 cellules) | 40-110 | ~2 | table (type, motif / 4) refaite au changement de salle (`late_lut`) |
| ouverture de la porte (`draw_door` toutes les 2 images pendant 80 images) | ~45 | ~2 | DMA en file |
| tir des types 0B et 10h : recherche du numero D700 libre (8 parcours de 15 objets) | 16-23 | ~4 | un parcours, masque de bits |
| `sat_sync` (appele par chaque creation d'objet, boss 15 : 16 fois par tour) | 5 | 2.5 | test d'abord si un proprietaire a change |
| boss 15 : `boss15_pool_sync` x 2 par tour (8 liberations de tirs inactifs) | 110-160 | ~10 | pas d'appel sur un emplacement qui ne tient rien (`Enemies_boss15_held`), une seule synchro |
| ramassage du fouet : tuiles des 6 images (CPU) | 330 | ~10 par tour | 3 etapes de 2 images en DMA (images 0+3, 1+4, 2+5), puis les images accroupies en 2 moities ; les dessins gardent le chargement immediat |
| orbe des boss 3, 6, 9 (tuiles CPU) | ~100 | ~2 | DMA en file |
| fenetre du marchand | 340 | (dessin, autorise) | comptee comme dessin (`vk_draws`) |

Mesures restantes (tour ordinaire, 3 ennemis + tirs) : `tick+whip` 18, `enemies` 40 en moyenne (pointes 60-100 a la creation d'objets : `newObject` ~9),
`show_player` 45-75 (`Enemies_sat_render` 10-17, `Enemies_sprites` 20-30), contact 11. Tour moyen ~125 lignes, tours les plus charges ~200.

### Les tours qui depassent de quelques lignes ne perdent plus l'image

`vk_turn_sync` : si le tour finit **dans le VBlank commence pendant lui** (un seul VBlank, hors dessin), le travail du VBlank (file DMA) est fait a l'instant
(`SYS_doVBlankProcessEx(IMMEDIATELY)`) et le tour suivant commence : plus d'image perdue pour quelques lignes (le VBlank dure 38 lignes), et la sequence des tours ne depend
plus de l'alignement du code tant que le depassement reste dans le VBlank. Un tour ordinaire n'est pas change (meme attente). Un tour qui finit apres le VBlank, ou qui en
traverse plusieurs, reste une image perdue (`vk_late_plain`) et fait echouer le rejeu.

### Resultat

Sonde de salles (stages 12-17, 60 salles, 3 passes d'inputs, ROM de mesure) : 0 image perdue, marge minimale 6 lignes, 1 % des tours a moins de 17-30 lignes selon la salle.
Rejeu re-enregistre sur la ROM normale apres fusion du fouet accroupi par niveau (91e0265) (`tools/playthrough_12_18_md.py --record`, segments 12-15 puis 16-17, `--width 64`, 55 points de controle) :
0 image perdue, 0 tour absorbe sur 29 158 tours ; marge (`slack`) : mediane 112 lignes, 1 % des tours a 59 lignes ou plus, 0,1 % a -6 (quelques tours isoles de debut de segment, ni perdus ni absorbes), maximum de charge 198 lignes de 250.
Segment 16-17 seul : marge minimale 50 lignes. `tests/run_playthrough_layout_md.sh` rejoue ce meme fichier sur deux
copies de sgdk/ dont seule la disposition change (fonction morte ajoutee a main.c ; 4 octets de remplissage dans VKBoss) : meme resultat, sans desynchronisation.
`tools/smoke_rooms_md.py` lit aussi `vk_late_plain` (153 salles, 0 perte).
