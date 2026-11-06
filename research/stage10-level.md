# Stage 10 : salles, décor, plateformes, bougies, blocs, coffres, clé, porte et sortie vers le stage 11

Code : `sgdk/src/generated/candles_md.c` (regénéré par `tools/build_candles_md.py` : le stage 10 n'est plus exclu), `sgdk/src/platforms.c` (table `906C`, stage 10),
`sgdk/src/generated/stage10_platform_md.c` (`tools/build_stage10_platform_md.py`), `actors.c` (chargement des tuiles). Cartes de collision, liens de salles, clé, porte et entrée
(`stage10_maps.c`, `stage_exits.h`) : déjà présents (`research/stages-1-10/`), recontrôlés ici contre la trace. Tests : `tests/test_platforms.c`, `tests/test_candles.c` (comptes du stage 10),
scénarios `tests/md_scenarios/zzm_stage10.txt` (parcours des 9 salles, plateformes, clé, porte, sortie) et `zzz_chests_stage1_10.txt` (35 coffres, dont les 4 du stage 10).
Traces openMSX (non versionnées, `scratchpad/stage10/`, harnais `stage4/tool` + journal des objets C800 à CB80, D700, C500) : `R` (9 salles forcées par `D000` = 10, `D002` = 3,
`D001`, `C000` = 4, Simon immobile, 900 images chacune), `D`/`D2` (instantané VRAM 12 à 25 images après le chargement), `K3`/`K4` (clé, porte, sortie), `CH` (4 coffres, plan
de `research/chests-stage1-9.md` avec `D002` = 3).

## Prouvé

- **Décor des 9 salles** : instantané VRAM MSX (`snap`) contre la capture MD (couleurs `(v + 18) // 36`, 32 premières lignes masquées) : **aucune tuile de décor ne diffère** (salles 0 à 8,
  salles 3 et 4 prises 25 images après le chargement). Restent les sprites du MSX (Simon, plateformes en un autre point de leur course, flammes à une autre phase) : 289 à 799 pixels par salle.
  Bougies : 2, 3, 2, 3, 2, 3, 4, 6, 1 par salle aux mêmes places que sur le MSX (salle 7 : les six).
- **Bougies, blocs et objets fixes** (5A49 + 5B1B exécutés par `build_candles_md.py`) : 26 bougies ou blocs cachés (une bougie `17` en mode 3 : bloc caché en salle 3 (96, 112)), 6 objets fixes
  (4 coffres `19` : salle 0 (224, 128) contenu 1A, salle 4 (64, 128) 11, salle 5 (96, 64) 1C et (112, 128) 13 ; 2 clés dorées `17` : salle 1 (144, 96), salle 5 (224, 128)), 2 blocs cassables
  (salle 1 (32, 48) objet 12, salle 8 (32, 144) objet 04). Avant : 0 entrée au stage 10.
- **Coffres du stage 10** (trace `CH`, `C700` = 1, `C701` = 2, Simon en (X + 8, Y + 16), énergie remise à 32) : 1A : son 11 à 915, son 12 à 989 (74 images), `C416` 0 -> 1 ; 11 : son 11 à 1599, son 12 à 1673
  (74), `C702` bit 7 (80h), type 05 d'octet `+4` = 11h ; 1C : son 11 à 2282, son 12 à 2358 (76 : Simon déplacé par un ennemi), `C416` = 3 ; 13 : son 11 à 2965, son 10 à 3048, `C406` 00 -> 50 (+5000).
  Mêmes effets, mêmes sons que dans `chests-stage1-9.md` ; le port les rejoue (`zzz_chests_stage1_10` : 71 assertions, délai ouverture -> ramassage 72 à 76).
- **Plateformes mobiles** (table `906C` : `0A`, salle, nombre, puis Y, X, pas, longueur : salle 0 `8F 60 +1 60`, salle 2 `A7 40 +1 80`, salle 3 `8F 20 +1 A0`, salle 4 `A7 80 +1 40`). Trace `R`
  (SAT, motifs D8 couleur 09 et DC couleur 4C, deux cellules en X et X + 16) : premier échantillon après le chargement en X début + 1 (97, 65, 33, 129), + 1 par passe de 2 images, X maximal **191** dans les quatre salles
  (début + longueur - 1), SAT Y 142 ou 166 (Y - 1). Même routine `5C80` que le stage 5 : le port la réutilise (`platforms.c`, une ligne par plateforme), tuiles D8/DC (`stage10_platform_md.c`) tirées
  de l'instantané VRAM de la trace. Test natif (course complète, retournement en 191) et scénario (X + 10 en 20 images).
- **Clé** (salle 0, (48, 48), visible, objet 83/18) : trace `K3`, Simon posé en (56, 50), prise 3 images plus tard, son 14h (C500 `84 30 30 01 18` -> `00`). Le port : prise au contact au même point (scénario).
- **Porte et sortie** (salle 8, (236, 128), côté droit) : trace `K4`, clé prise, Simon posé en (200, 176) et poussé à droite : arrêt contre la porte en X 228 à l'image 1221, **son 1Ah à 1222**, Simon traverse, `D000` 10 -> 11
  et `D001` = 0 à 1390 (X 248, **168 images** après le son), `C000` = 4 (chargement), Simon en (16, 176) à 1410, musique 81 à 1427. Le port rejoue la porte (168 images, `kStageItems[9]` : ouverture, clé consommée) ;
  l'entrée du stage 11 (16, 176) est portée depuis (`stage11-level.md`).

## Non porté ou non vérifié

- **Objet 1B** (D002 = 3, routine `B4EA`) : **inatteignable** au stage 10. Les 9 salles ont 26 bougies ou blocs et 2 blocs d'objet, aucun de type 15 (octets `+5` lus dans 5A49 + 5B1B et dans `DE00`) ; `898F` ne crée l'objet
  que pour une bougie 15. Aucun 1B n'a pu être tracé ; pas de code. Test natif : aucune bougie 15, aucun objet mural 15 au stage 10.
- Ennemis du stage 10 (types 3 et 21h de la routine `A310`, chauve-souris du canal `9D97` en salle 0) et objet 0Eh : voir `stage10-enemies.md`.

## Contrôle complémentaire gameplay / salles (2026-11-06)

`tests/test_stage10_rooms.c`, appelé par `tests/run_stage10_native.sh`, verrouille les neuf entrées de la table des liens de salles extraite du MSX2, les 18 tentatives de sortie gauche/droite (passage ou blocage), le maintien de Y/mode/phase et les gardes de franchissement (seuil X et escaliers). Il contrôle les 26 bougies/blocs, les six objets fixes (coordonnées, type et contenu), les deux objets de blocs, les quatre plateformes (y compris l'extrémité X=191 et le retournement), et les coordonnées d'entrée, de clé et de porte des tables ROM.

Résultat de la suite native ciblée : tous les tests ont réussi. **Aucune correction de gameplay n'a été appliquée** : les valeurs déjà intégrées concordent avec les références et scénarios conservés. Ces tests reprennent les données ROM extraites et protègent contre les régressions, mais ne remplacent pas une exécution du parcours dans Genesis Plus GX ni une nouvelle trace openMSX. La comparaison d'images et le timing réel de la traversée des neuf salles restent à valider dans les émulateurs.
