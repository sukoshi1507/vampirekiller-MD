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
Le maintien du cavalier du stage 12 et de ses tirs 0Ah est écrit mais désactivé (`BOSS12_SAT_HOLD 0` dans `actors.c`) : activé, le rejeu image exacte `tests/run_playthrough_12_18_md.sh` manque le point de contrôle du combat (image 4870) ; à réactiver après un nouvel enregistrement.
Restent hors allocateur (marqueur limité) : la flamme qui remplace un corps du stage 12 à sa mort, les autres objets C800 du combat 18.

## Espèces des stages 2-11

`research/legacy-species-msx.json` (poses vues par `run_species_trace.py` stages 1-11, stage 9 sans la salle 2). Lignes ajoutées à
`c800_sat.h` pour les types ROM 02, 03, 05, 06, 07, 08, 0A, 0C, 20, 21 (et 23 : mêmes lignes que 0D) : 5F1D + 6445 réellement exécutés
(les enfants construits par 5F1D, ex. gouttes du type 02/03, sont retirés pour ce montage parent seul), comparés (masque SAT, références
du constructeur suivant, octets IX+00..1F) par `tests/run_c800_sat.sh` à plusieurs Y, IA gelée (le rendu et l'IA de ces espèces sont prouvés
ailleurs). Exclus : types ROM 01 et 04 (le port garde leur Y en coordonnée SAT, `ENEMY_TYPE4_OBJ_Y`, alors que les lignes sont relatives au Y
de l'objet). Poses atteintes par la ROM sans figurer dans les traces (panthère 43h..46h, chauve-souris 16h/17h, poisson 13h/14h) : `LEGACY_EXTRA` de `build_c800_sat.py`, prouvées par `tools/verify_legacy_poses.py` (5F1D + 98E5/9E31 réels).

Coût : avec 73 lignes parcourues à chaque image pour chaque objet C800, `tests/run_candle_loot_md.sh` (repli C500) et
`tests/run_mixed_pools_md.sh` échouaient (assertions sur 40 images ; même avec des lignes factices jamais appariées), car ces scénarios sont au
budget d'une image. `build_c800_sat.py` groupe donc les lignes par type et émet `c800_sat_ranges` (première ligne, nombre) : la passe SAT ne
parcourt que les lignes du type de l'objet. Les deux scénarios repassent.
