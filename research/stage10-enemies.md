# Stage 10 : ennemis (homme-poisson 03 et 21h, routine `A310`), chauve-souris de la salle 0, objet 0Eh

Code : `sgdk/src/enemies_fish3.inc` (inclus par `enemies.c` : état, création `A2E0` et `A2C7`, passes, tir, cellules), `enemies.c` (type `ENEMY_TYPE_FISH3`, canal `9D52`
dans `type2Channel`/`type2Frame`, liste fixe, boule de l'homme-poisson dans `fireballStep` et `Enemies_sprites`), `actors.c` (tuiles), `tools/build_stage10_enemies_md.py`
(tuiles tirées d'instantanés VRAM d'openMSX, non versionnés : `scratchpad/stage10/out/R2/snaps`), `tools/gen_fish3_vectors.py` + `tests/fish3_vectors.h`, test natif
`tests/test_fish3.c`, scénario `zzm_stage10.txt`. Salles, plateformes, bougies, coffres, clé et porte : `stage10-level.md`.

## Ce que le stage 10 contient (trace R : 9 salles forcées, 900 images chacune, `D002` = 3)

Drapeaux de salle `85A6` (`vk_enemy_room_flags[10]` = 08, 04, 04, 04, 04, 04, 0, 0, 0) : salle 0 bit 3 = canal `9D97` (objet 04, chauve-souris), salles 1 à 5 bit 2 = canal **`9D52`**
(objet 03 : `9D52` est `9D4B` avec `C` = 3, même minuterie `CF02`/`CF03`, mêmes tables `9D87` et `9D8F`). Liste fixe (`6154`, `DB00` + salle x 16) : salle 6 deux objets 21h en
(48, 176) et (192, 176), salle 7 un en (128, 176), salle 8 deux en (48, 176) et (208, 176). Types vus dans les blocs C800 de la trace : 03, 04, 20 (goutte), 21h ; D700 : 01 (boule).
Table de routines `9942` : les types 02, 03 et 21h ont la même routine `A310` (l'homme-poisson du stage 2 est le type 02, déjà porté par `fishStep`). Aucun objet 0Eh.

## Prouvé

- **Routine `A310` du type 03 et 21h** (états 0 à 2, init `A2E0` ou `A2C7`, 99B9, 99E5) : exécutée passe par passe dans l'émulateur Z80 sur les cartes des salles 1 à 8 (45 scénarios, Simon en
  triangle) : **9 958 passes**, le port donne le même état, X, Y, pose, vx, vy, `+10`, `+11`, `+12` à chaque passe ; 26 tirs (même passe de création, la boule déjà avancée de 3 px), 19 plongeons, 33 chutes
  de rebord, 85 retours de l'arrêt à la marche. Écart signalé par ce test : un plongeon libère le bloc et les deux gouttes (type 20h) le reprennent (même chose dans l'émulateur).
- **Trace R, salle 6, objet 21h (48, 176)** : type 21h pendant une image puis 03, à terre, pose 12h, `+6` = 0 tant que Simon est trop haut (C425 = 168 : pas de déclenchement) ; Simon à 174 : première passe
  X 46 + 80h, vx FE80h, pose 10h, `+10` 1 ; deuxième passe : état 2, `+10` 0, `+11` 18h ; puis `+6` = 0, `+11` 17h, 16h, 15h, pose 0Fh. Le test natif rejoue ces 5 passes ; le scénario le rejoue sur le MD.
- **Création par le canal `9D52`** (salle 1, trace R) : objets 03 en X 208, 80 (table `9D87`, décalage de 32 px vers le centre si Simon est à moins de 24), 144, 160, 96, 96 ; premier 71 images après
  le début de la salle (MD : 37, la trace compte le rechargement), **le suivant 16 images plus tard** (87 ; MD 53), puis tous les 200 images ; Y C8h, vy F480h. Scénario : 208 puis +16.
- **Poses et motifs** (SAT, plus de 2 000 images appariées) : 0F et 10 : haut 80/84, bas 88/8C ou 90/94 ; 11 (tir à gauche) : 98/9C et A0/A4 ; 12 et 13 : haut A8/AC, bas B0/B4 ou B8/BC ; 14 (tir à droite) : C0/C4 et
  C8/CC ; deux couches, couleurs 02 et 45 (CC + 5), colonne en SAT (X - 8, Y - 32) et (X - 8, Y - 16). Les cellules composées sont dans `vk_stage10_enemy_tiles` (cases 40 à 68 de la disposition compacte).
- **Boule** (D700 type 1 : `A08E[2]` ; `830D[0]` = 0 : boîte (3, 6) comme la boule de la statue ; `9E77` ne fait rien) : créée en (X, Y - 14h), vx +-300h dans le sens de la marche (`A498`), tout droit ; trace :
  vx 0300h, SAT (X - 5, Y - 12) vers la droite (725 images) et (X - 11, Y - 12) vers la gauche (463). Motif F0 couleur 08 comme le stage 8.
- **Goutte** (objet 20h, créée à la sortie de l'eau et au plongeon) : motif D0 **couleur 0Bh** (4 575 entrées SAT) au lieu du 6 du stage 2 : cellule 90 du bloc du stage 10.
- **Chauve-souris de la salle 0** : première création en SAT (229, 122), comme sur le MSX (trace 24/48:122:229). **Son image n'est pas celle du stage 2** : les motifs 48/4C, 50/54, 58/5C de l'instantané du stage 10
  diffèrent du tableau `vk_enemy4_tiles` (contour couleur 4 et une ligne plus haut) ; le port charge `vk_stage10_bat_tiles` (couches de l'instantané, droite = miroir, vérifié sur le tableau du stage 2 : les cellules
  droites sont les miroirs exacts). Le comportement reste la routine `B0F8` déjà portée.
- **Objet 0Eh (`A9B7`) : pas au stage 10.** Aucune des 9 salles (R, R2, L0 : 3 000 images en salle 0) n'en crée ; les listes fixes du stage 11 (salle 5 : type 0Eh en (208, 112)) et du stage 12 (salles 0, 4, 5,
  7, 8, 10, 11) en ont ; trace `S11` (`D000` = 11, `D002` = 3, salle 5) : écriture du type 14 dans C800. L'appel `AA81` rapporté au « stage 10 salle 0 » par `hasard-R.md` venait d'un plan qui avait dépassé le stage 10.
  Non porté (stages 11 et 12 hors périmètre).

## Déduit (code lu, non tracé en jeu) ou non porté

- Dégâts de contact 4 (`81CE` : paire 02 02 doublée), 200 points, 2 points de vie (`60E1`), boîte 5 x 18h (classe 1 de `82CB`) : lus dans la ROM, identiques au type 02 ; non mesurés sur le stage 10
  (les traces de coffres montrent des coups à Simon mais leur énergie n'a pas été relevée). Fouet à deux coups, armes lancées, temps arrêté : mêmes chemins que les objets du stage 4 (`hasHp`), non rejoués.
- Sons : la version initiale ne jouait pas 08 (saut), 07 (atterrissage), 09 (plongeon) de `A4E8` ; les trois points d’appel Z80 ont ensuite été raccordés au PSG dans la reprise du stage 10, avec tests natifs. La cadence sonore en émulation Mega Drive et en trace openMSX reste à comparer. Aucun son au tir dans la routine documentée.
- Octet `+12` hérité de l'occupant précédent du bloc (pas effacé par `5F1D`) : le port garde ses octets d'une salle à l'autre (`s8x`, `f3`), comme le MSX, pas leur valeur exacte.
- Cadence : le type 03 est avancé dans la passe de sa création (comme sur le MSX), l'objet 21h une passe après (trace : 21h une image) ; décalage d'une image non comparé.
- Chauve-souris : positions suivantes et courbe non comparées au stage 10 (seule la création l'est).
- Échantillons VRAM : cases du bloc d'ennemis chargées à l'entrée de la salle ; les images de la trace montrent un pixel de jitter en X (-6 à -9) non reproduit.

## Reprise du 06/11/2026 — verrouillage de la première chauve-souris

- Ajout de `test_stage10_bat_first_sat` dans `tests/test_enemies.c` : salle 0, canal `9D97`, objet `04`, apparition SAT `(229,122)` et motifs `48/4C` (observations de la trace R décrites ci-dessus). Vérification négative du canal `9D97` en salle 1 (drapeaux `85A6`).
- Exécution ciblée du test natif des ennemis : `Enemies tests: PASS`.
- Aucun changement à la trajectoire du type 04, aux graphismes ni au son à cette étape. Les mesures de trajectoire existantes concernent le hall/stage 2 ; elles ne sont **pas** une trace du stage 10.
- La suite d'audit native complète a été interrompue par la limite de temps de l'environnement après le contrôle du stage 10 ; elle ne peut pas être considérée comme entièrement validée sur cette reprise. Une capture openMSX SAT image par image de la salle 0 du stage 10 est nécessaire pour conclure sur la trajectoire et sa cadence.

## Reprise complémentaire du 06/11/2026 — contrôle de périmètre et audit strict

- Le test `test_stage10_bat_first_sat` couvre maintenant les **salles 1 à 8** pour l'absence du canal `9D97` : table `85A6` documentée plus haut, salle 0 seule avec bit 3. La position initiale et les motifs de la salle 0 restent testés séparément. Aucun changement au mouvement de la chauve-souris, faute de trace SAT complète du stage 10.
- `tests/run_stage10_native.sh` rassemble les contrôles natifs ciblés (plateformes, ennemis, bougies, poissons ROM et sorties) et s'arrête immédiatement en cas d'échec. Exécution locale : PASS pour les cinq ensembles de tests ; poissons : 45 vecteurs, 9 958 passes, 26 tirs. Ce contrôle ne remplace pas une ROM MD lancée dans l'émulateur.
- `tests/audit_run_native.sh` utilisait `set -u` et pouvait poursuivre jusqu'au message « PASS » final malgré le retour en erreur d'un test. Il utilise maintenant `set -eu` ; l'exécution s'arrête avec statut 1 sur `test_health.c` lignes 131-132 (deuxième contact à l'image 181). Il s'agit d'une anomalie transversale non attribuée au stage 10, à confronter avec une trace ROM avant toute correction comportementale.

## Reprise interactions — 06/11/2026

- Tests ajoutés à `tests/test_fish3.c` (`test_stage10_interactions`) : contact du type 03/21h (classe 1, 4 points d'énergie, recul et invulnérabilité), fouet cuir (2 coups), fouet à chaîne (1 coup), armes par les chemins `8048`/`8066` (dague 1 et hache 4), et arrêt du temps sur un 21h fixe pendant 12 images. Vérification des scores de 200 points et de la restitution du bloc lors de la flamme de mort.
- **Provenance des attentes : lecture des tables et routines Z80**, non traces openMSX des actions en salle 10. Ce sont des tests de conformité des chemins communs, pas des mesures image-par-image du stage 10. Ne pas qualifier ces scénarios de validation ROM exécutée tant qu'une trace d'action MSX2 n'est pas disponible.
- Exécution de `tests/run_stage10_native.sh` : 5 ensembles PASS ; `test_fish3.c` conserve les 45 vecteurs / 9 958 passes de déplacement et tir confrontés à l'émulateur Z80.
- Aucune constante comportementale ni nouvelle mécanique ajoutée au moteur. La trajectoire SAT complète de la chauve-souris, les cadences audio et la fidélité des interactions en conditions réelles openMSX restent ouvertes.
