# Stage 11 : salles, décor, bougies, blocs, clé, porte et sortie vers le stage 12

Code : `sgdk/src/generated/stage11_maps.c` (`tools/build_stage11_maps_md.py` : cartes par la routine `4F83`, liens par `B99A`, exécutées dans l'émulateur Z80), `candles_md.c`
(`tools/build_candles_md.py`, stage 11 inclus), `stage_exits.h` (`tools/extract_stage_exits.py`), `enemy_rooms.h` (`tools/extract_enemies.py`). Les tableaux de bougies dépassent 255
entrées : `vk_candle_first` est sur 16 bits et les drapeaux `out`/`taken` du stage 11 sont en fin de `VKCandles` (`out11`, `taken11`, macros `VK_CANDLE_OUT/TAKEN`) pour garder les décalages lus par les
scénarios. Tests : `tests/test_candles.c`, `stage_exits.c`, `room_transitions.c`, `enemy_integration.c`, scénarios `zzm_stage10.txt` (la porte du stage 10 mène au stage 11), `zzn_stage11.txt`,
`zzz_chests_stage1_11.txt`. Traces openMSX (non versionnées, `scratchpad/stage11/`, harnais `stage4/tool` + journal des objets) : `R` (6 salles forcées, 900 images), `R5`, `K_*` (clé), `KD` (clé, porte, sortie), `G`, `G5` (bougies d'objet 15).

## Prouvé

- **Salles** : 6 salles en ligne (liens 0-1-2-3-4-5, aucun lien vertical), 0 écart entre la carte du port et `4F83` (`extract_collision_maps.py` vérifie aussi le stage 11). Décor : instantané VRAM MSX contre la capture MD, 6 salles :
  seuls les pixels de Simon diffèrent (289 pixels). Pas de plateforme mobile (table `906C` : stages 5 et 10 seulement).
- **Entrée** : depuis la porte du stage 10 (trace K4 du stage 10) Simon arrive en (16, 176), tourné à droite (`6402`) : le scénario `zzm_stage10` le vérifie.
- **Bougies** : 25 bougies ou blocs (salles 0 à 5 : 3, 7, 3, 7, 3, 2), 1 objet fixe (clé dorée 17 en (128, 64), salle 1), pas de coffre (le scénario des coffres garde ses 35 coffres), 4 bougies d'objet 15 (salles 0 et 5).
- **Clé blanche** : cachée dans le bloc (96, 80) de la salle 1 (bougie `01 50 60 00 03 18`). Trace K : Simon en (92, 112), coup à 873 (sons 0C et 0E), clé `84 50 60 00 18` en (96, 80), prise à 884 (son 14h) ; son 05 (début du coup) à 870, donc 14 images ; le port : 13 (scénario).
- **Porte** (salle 5, (236, 128)) : trace KD, clé prise, Simon posé en (200, 176) et poussé à droite : arrêt en X 228, **son 1Ah à 1153**, `D000` 11 -> 12 à 1322 (169 images après le son ; 168 au stage 10, échantillonnage), Simon en (16, 176) au stage 12 à 1344, `C000` = 5 à 1361, aucune commande sonore pendant le chargement. Le port rejoue la porte (`kStageItems[10]`).

## Non porté

- **Stage 12** : la sortie du stage 11 renvoie au titre (`main.c`), le stage 12 (boss) n'est pas porté. Entrée tracée : (16, 176), tournée à droite.
- Ennemis du stage 11 : `stage11-enemies.md`.

## Contrôle supplémentaire du 6 novembre 2026 (livraison stage 11)

- `tests/test_stage11_rooms.c` : liens horizontaux complets des 6 salles, 12 tentatives de franchissement et seuils, absence de liens verticaux, absence de plateformes mobiles, 25 bougies/blocs et 4 objets 15, clé dorée fixe, bloc à clé blanche, tables d'ennemis fixes, drapeaux `85A6` et porte/entrée.
- `tests/run_stage11_native.sh` : exécute indépendamment le test des salles, les 42 vecteurs de l'aigle/puce, les 10 vecteurs et 432 visées du dragon, les bougies, les tests de clé/porte/sortie. Ce script ne requiert pas la ROM MSX2 fournie séparément.
- Les scripts natifs globaux `tests/run_native.sh` et `tests/audit_run_native.sh` incluent désormais ce test des salles. La suite globale et la suite ciblée passent dans l'environnement d'audit.
- Ce contrôle est une **régression contre les extractions et les traces préexistantes**, pas une nouvelle preuve openMSX. Aucun nouveau comportement de gameplay n'a été inventé ni ajouté à cette étape.
- **Limites** : pas de compilation SGDK ni de test sur une ROM Mega Drive dans cet environnement ; la transition vers le stage 12 revient au titre car le stage 12 n'est pas porté (état déjà présent) ; durée précise et rendu final à vérifier sur émulateur MD.
