# SAT : propriétaires boss, couleurs écrites, espèces anciennes

Référence : `probe.load_rom(probe.reference_rom())`, instructions intactes. Outils : `tools/sat_snapshot_extra.tcl` et
`tools/sat_watch_extra.tcl` (ajoutés à `tools/run_species_trace.py` par `SP_EXTRA=`), `tools/verify_sat_colour_writers.py`,
`tools/verify_boss_bodies_sat.py`, `tools/build_c800_sat.py` + `tools/verify_c800_sat.py` (lignes ancienne espèce).

## CE03 (restriction D700 17..16)

9FC2 lit CE03 ; aucun code ne l'écrit : un seul `3A 03 CE` dans toute la ROM, aucune charge `CE00`/`CE03` de base avec
décalage (vérifié par `verify_sat_colour_writers.py`). Instantanés openMSX (`sat_snapshot_extra.tcl`, salles forcées des stages 3, 6, 9, 10, 14-18 ;
`sat_watch_extra.tcl`, points d'arrêt d'écriture stages 1-13) : CE03 vaut 0 dans tous les états relevés, aucune écriture non nulle.
D700 balaye donc toujours 17..10 ; `Enemies_sat_boss` n'est jamais appelé en jeu (API de test).

## Couleurs (+25+5k), lues par 651B

Un octet de couleur de quartet bas nul force Y=E1 (entrée jamais libérée au Y=E0). Seuls écrivains hors constructeurs (6029 table 6084,
A01A table A0BD) : les 22 stores directs `LD (IX+25|2A|2F|34|39|3E|43|48)` de la ROM entière, tous listés dans le vérificateur :
objet 26h (9C2A : 08 01 0E 01, 9C75 : 0B), statue 0A (A28A..A295 : 02/4C), objet 14h boss 9 (B96F..B981 : 02 puis 08 ou 4C), objet 09h
(AF55/AF58, B053/B056 : zéro ; AF85/AF88 : retour 02 / 45), objet 11h boss 18 (ACB2..ACBB : zéro). Les cinq `SET 5,L` de la ROM sont 6029, 6445,
6501, 9A00 (libère les Y d'un mort) et A01A. Seuls 09h et 11h peuvent donc annuler une couche ; les lignes 09h de `c800_sat.h` sont par état
(constructeur = AF4A : couches 2/3 nulles, poses 0/25/26 ; AF7E : quatre couches, poses 21..24), déjà dans `build_c800_sat.py`. Les écritures
d'openMSX stages 1-13 (`sat_writers.txt`) ne montrent que 6040 (constructeur) et A022 (constructeur D700) hors artefacts de salle forcée
(9:2, 13:3/13:4 plantent le Z80 en salle forcée : exclues, stage 9 relevé hors salle 2).

## SAT tenues par les boss (C800[0] puis C800[1], références croissantes)

Instantanés openMSX : 3:5 corps 12h, 8 entrées 0..7 ; 6:5 corps 13h, 8 entrées 0..7 ; 9:7 deux 14h, 6 entrées chacun (0..5, 6..11) ;
12:6 cavalier 15h (6, 0..5) + 18h (2, 6..7) ; 15:9 corps 16h (12, déjà LOT14G) ; 18:9 corps 11h (8, couleurs réécrites 00 -> 02/48, ys 80).
Portés : 12h, 13h, 14h x2 (`actors.c`, `Enemies_boss15_reserve`, emplacement 9 = C800[1]) ; comparés au vrai 5F1D/99F6 par
`tests/run_boss_bodies_sat.sh` (masque 18 entrées, références de deux flammes 1E, libération puis réutilisation).
Ajoutés ensuite (`actors.c`, `tools/verify_boss_bodies_sat.py`, 5F1D/9F6D/99F6 réels) : cavalier 15h (C800[0], 6) + 18h (C800[1], 2) du stage 12, corps 11h
du stage 18 (8), tirs D700 de 2 entrées des boss 6 (type 6), 9 (type 7), 12 (0Ah) et 18 (05h), références 17 16 / 15 14 / 13 12 pour D700[0..2].
Le maintien du cavalier du stage 12 et de ses tirs 0Ah est actif (`BOSS12_SAT_HOLD 1` dans `actors.c`, rejeu `tests/run_playthrough_12_18_md.sh` réenregistré sur le gel de 20 images).
Flamme de mort des corps (stages 12 et 15, `walker.alive == 2`) : le vrai `7806` (`7E17` -> `81AB`, `JP 7806` : chaque bloc C800 passe par `9A3A`, `99F6` libère le corps puis, `CE00` non nul, `5F1D` construit un 1E de 2 entrées sur les deux premières références libres, dans l'ordre des blocs) donne : stage 12 `CE0B` non nul, flammes en C800[0] (0,1) et C800[1] (2,3) ; `CE0B` nul, seule la flamme 0 (0,1), le cavalier 18h garde 6,7 (délai `9AB3`) ; stage 15 flamme (0,1). Elles tiennent leurs entrées 16 passes (`9B71`) puis le bloc est libéré. Porté : `actors.c` tient 2 entrées pour `alive == 2`, un corps devenu flamme est d'abord libéré même si le compte reste 2 (rider 6,7 -> 2,3) ; `tools/verify_boss_bodies_sat.py` `flames()` compare masque, références, fin des flammes et réutilisation. Stages 3, 6, 9 (CE00 = 1, 2, 3 relevé sur openMSX en 3:5, 6:5, 9:7 ; 12h/13h/14h n'ont pas le délai 9AB3) : le vrai `7806` donne une flamme 1E (0,1) pour 12h/13h et (0,1) + (2,3) pour les deux 14h, tenue 16 passes. Porté : `VKBoss.flame[2]` (boss.c, `vk_boss_hit`), tenue dans `actors.c` `boss_bodies_sync` (image de la flamme non modélisée) ; `verify_boss_bodies_sat.py` `flames()`.

Type 01 (zombies A/B) : 5F1D + 6445 réels, objet A0 -> octet SAT 80 : objet = octet SAT + 32 (`ENEMY_OBJ_Y` 32, la chute provisoire teste les pieds en Y + 32) ; l'écran affiche l'octet SAT + 1 : `Enemies_sprites` renvoie maintenant octet + 1 pour A/B et 04, comme les autres espèces.

Combat 18 : l'allocateur tient maintenant sept emplacements C800 (0, 9, 10..14 = C800[0..6]). Objets 17h (aucune entrée, table 6057), 1Eh, 2Ch, 2Dh, 2Eh (2 entrées chacun, `5F1D` réel) tenus de la création au kill (`dracula_sat_sync`). Preuve de la tenue sans libération par Y=E0 : `6445` réel sur chaque Y d'objet 0..227 et chaque pose (2Ch : 02, 2Dh : 57/59, 2Eh : 5A, 1Eh : 85/86) ne met jamais une entrée à E0 (Y SAT = Y objet - 15, flamme - 16/15) et `99E5` supprime l'objet à Y >= E4 (`dracula.c` `move`). `tools/verify_dracula_sat.py` (ordre de `5F1D`, `99F6`, réutilisation des références, flamme refaite dans le bloc). Un objet qui change de type dans son bloc (2Ch tué, flamme 1E) est libéré avant la nouvelle tenue.

## Espèces des stages 2-11

`research/legacy-species-msx.json` (poses vues par `run_species_trace.py` stages 1-11, stage 9 sans la salle 2). Lignes ajoutées à
`c800_sat.h` pour les types ROM 02, 03, 05, 06, 07, 08, 0A, 0C, 20, 21 (et 23 : mêmes lignes que 0D) : 5F1D + 6445 réellement exécutés
(les enfants construits par 5F1D, ex. gouttes du type 02/03, sont retirés pour ce montage parent seul), comparés (masque SAT, références
du constructeur suivant, octets IX+00..1F) par `tests/run_c800_sat.sh` à plusieurs Y, IA gelée (le rendu et l'IA de ces espèces sont prouvés
ailleurs). Types ROM 01 (A/B) et 04 (le port garde leur Y en coordonnée SAT : `Enemy.y` = octet SAT de la première entrée, Y objet - 32 pour 01, - 15 pour 04) : lignes propres (`RAW_Y_SPECIES` de `build_c800_sat.py`, relatives à la première entrée : 0,0,16,16 et 0,0), la même mise en page à toutes les poses (400 images de `98E5`/`9E31`, plusieurs places de Simon ; la première image du 04, 7 plus bas, est la pose provisoire que le port ne reproduit pas), comparées au vrai `6445` autour de E0 par `tools/verify_c800_sat.py` (`raw_y_case`). Poses atteintes par la ROM sans figurer dans les traces (panthère 43h..46h, chauve-souris 16h/17h, poisson 13h/14h) : `LEGACY_EXTRA` de `build_c800_sat.py`, prouvées par `tools/verify_legacy_poses.py` (5F1D + 98E5/9E31 réels).

Coût : avec 73 lignes parcourues à chaque image pour chaque objet C800, `tests/run_candle_loot_md.sh` (repli C500) et
`tests/run_mixed_pools_md.sh` échouaient (assertions sur 40 images ; même avec des lignes factices jamais appariées), car ces scénarios sont au
budget d'une image. `build_c800_sat.py` groupe donc les lignes par type et émet `c800_sat_ranges` (première ligne, nombre) : la passe SAT ne
parcourt que les lignes du type de l'objet. Les deux scénarios repassent.
