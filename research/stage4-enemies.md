# Stage 4 : objets 06 et 08, réarmement des coups, objets de salle

Code : `sgdk/src/enemies.c` (`type6Step`, `type8Step`, `type8Channel`, `Enemies_whipHitStage4`, `Enemies_rearm`),
`actors.c`, `weapons.c`. Tuiles : `tools/build_stage4_enemies_md.py`, sur une trace openMSX non versionnée.
Scénario : `tests/md_scenarios/zu_stage4_enemies.txt`. Limité au stage 4 : les autres stages ne chargent ni ces
tuiles ni ces objets.

Forcer un stage dans openMSX demande aussi `D002` (table `5E6A`[D000], 1 pour les stages 4 à 6) : `6154` recopie
les listes d'objets de ce groupe à chaque chargement de salle. Sans lui, aucun objet fixe n'apparaît.

## Prouvé (ROM et trace)

- Table de routines par type `9942` : 06 → `A592`, 08 → `A51B` ; initialisation `A573` et `A4FB`. Classes `82CB` :
  06 classe 1, 08 classe 2. Points et dégâts `81CE` : 200 points, dégât 2 doublé (4).
- Objet 06 : 4 points de vie, 2 cellules. Demi-tour sans sol (`7B98`), contre un mur sur 4 hauteurs (`7BBE`,
  `7C1A`), aux bords, ou quand `+C` arrive à 0 (96 passes). À la hauteur de Simon, il repart vers lui une fois.
- Positions X des deux objets 06 égales à la trace aux 8 relevés de 60 images en salle 0 (Simon immobile).
- Objet 08 : canal du bit 20 (`9DD5`, période `9DDF`), à Y de Simon - 8, vitesse X 280h, vitesse Y ondulante (± 40h).
- Fouet : -1, -2 avec la chaîne (`7DE6`). Armes : dague 1, hache 4, croix 2, eau 2 (`8048`).
- **Réarmement** : un coup efface `+E` bit 0 (`8083`) ; `999F` le remet après le fouet (`7257`), au lancer (`7286`),
  au demi-tour d'une hache ou d'une croix (`7356`, `7370`) et pour l'eau (`7422`). Le boss suit la même règle.

## Déduit

- Poses 73 et 74 de l'objet 08 (vers la droite) : motifs 50 et 58. Jamais vus au stage 4, vus depuis au stage 5
  (`stage5-enemies.md`) ; ce sont les miroirs exacts de 40 et 48.
- Boss : 12 par coup de hache, 2 coups par lancer, soit 6 lancers pour 80h, cohérent avec la soluce (5-6). Pas de
  mesure en jeu.

## Comparaison à la trace (salles 1 à 5)

- Objets 06 : trajectoires identiques passe par passe, 241 passes en salle 2, 82 en salle 1 (fin de la trace : Simon
  repart en salle 0 après un coup), 214 en salle 4. Objets 08 (salle 4) : mêmes passes d'apparition (période 52), même
  trajectoire. Salles 3 et 5 : ni 06 ni 08, comme la trace.
- À l'image près, le MSX prend du retard : 28 passes en 61 images en salle 1, 30 passes sur 60 en salle 2. Ce retard
  dépend de la charge du Z80 et la MD ne le reproduit pas ; comparer à image égale donne donc de faux écarts.
- Décor : les clés dorées et coffres fixes (10 objets 0x17 et 0x19) manquaient, `build_candles_md.py` s'arrêtait au
  stage 3. Corrigé ; les écarts restants sont les sprites, la phase des bougies et les deux dessins parasites ci-dessous.

## Provisoire, ouvert

- (Réglé, voir Vérifications) L'eau bénite ne réarmait pas comme `7422`.
- Objet 1E (flamme de mort) : porté depuis (`death-flame.md`). Bit 7 du drapeau de salle, objet 1F (bloc) : non portés ici.
- Pas de comparaison pixel des sprites des deux objets.
- Une partie lancée sur un stage par le debug (C au titre, sélecteur de pause) garde deux dessins parasites en
  (64,144) et (192,144), les torches de la cour. Absents en passant par la sortie du stage 3 (`zr-stage4`). Cause
  non trouvée : ni le mode de `vk_show_castle_room`, ni la file DMA, ni les sprites 7 et 8 ne l'expliquent.
- (Réglé, voir Vérifications) Clé, porte et sortie du stage 4 rejouées.

## Vérifications (agent `zy`, scénarios `tests/md_scenarios/zy_*`)

Légende : **prouvé** = ROM désassemblée et/ou trace openMSX ; **déduit** = lecture du code sans trace ; **non vérifié**.
Les scénarios posent les objets par `poke` (position, vitesse X nulle ou drapeau `+11` pour les immobiliser) : la
trajectoire propre aux objets n'y est pas testée, seulement le contact, le fouet et les armes.

1. **Contact 06 et 08** (`zy_a_contact`). Dégâts : table `81CE` (octet 2 : 06 = 2, 08 = 2), doublés (`816C`) soit 4 ;
   avec l'armure (`C701` bit 4) et Simon tourné vers l'objet : 2 et `C441` - 1, dos tourné : 4 sans décompte. Prouvé par le code
   (`816C`-`81A8`, `8179`) ; trace sans armure : 32 -> 28 sur 3 contacts de 06 (`stage4/out/R` image 1482, 1687, `T` 919,
   `stage5/out/R` pour 08). Invulnérabilité `C42D` = 5A, 87 cinq images plus tard, 0 après environ 200 images (trace R) : identique
   (`ENEMY_INVULN_START`). Recul : **écart corrigé** (`2751e3e`). `7ECD`-`7ED6` : `C43C` = 1 si le bit 7 de `+A` (vitesse X de l'objet)
   est nul, donc Simon est repoussé dans le sens de **marche de l'objet**, pas selon sa position relative. L'objet 06 gardait sa
   vitesse dans `vy` avec `dx` = 0 : le port utilisait la position. Les 5 contacts des traces ne distinguent pas les deux règles
   (objet venu de face à chaque fois) : correctif prouvé par le code (même routine `7EC1` que le type 1, dont la trace est dans
   `player-health.md`), pas par une trace d'un 06 qui s'éloigne. Les drapeaux `+E` valent 7 en trace pour 06 et 08 (bit 1 =
   contact, bit 0 = fouet, bit 2 = réarmement par `999F`).
2. **Fouet** (`zy_b_whip`). PV (`+D`) : 06 = 4, 08 = 1 (trace). `7DE6` : -1, -2 avec `C416` != 0, mort si le résultat est 0 ou négatif,
   donc 4 coups au cuir, 2 à la chaîne ; 08 meurt au premier coup (1 - 2 = FF, bit 7). Points : `81AB` = 2 centaines, seulement à la
   mort. Le port est conforme sur les 4 cas. Déduit : un seul test par coup (`C422` = 3), donc un seul coup de PV par fouet ;
   `7257` réarme au début du coup, le port réarme après le test (même effet tant qu'aucune arme n'est en vol : non vérifié).
3. **Armes secondaires** (`zy_c_weapons`). Dégâts `8048` : dague 1 (détruite au contact, `74EC`), hache 4, croix 2, eau 2. 06 (4 PV) :
   dague 4 -> 3, hache tue en une touche (200 points), croix touche à l'aller (2), pas de seconde touche avant le demi-tour
   (`999F` en `7356`/`7370`), tue au retour. 08 : la dague le tue (200 points) ; la hache contre 08 : test non concluant
   (l'objet sort de la trajectoire de la hache), **non vérifié**. **Eau bénite : écart confirmé puis corrigé** (`521808c`) :
   `7419`-`7422` appellent `999F` les passes où `C003` bit 2 est mis (4 passes sur 8, soit 12 des 24 passes de la flamme) ; le port
   n'appelait jamais `Enemies_rearm` depuis l'eau, donc un 06 touché une fois par une flamme ne l'était plus jamais (2 PV restants au
   lieu de la mort pendant la flamme). Le scénario échoue sans le correctif. L'instant exact des touches dans la flamme dépend de
   l'ordre des routines MSX (non tracé) : seul le résultat (mort avant la fin de la flamme) est testé. La même règle s'applique au
   boss (`vk_boss.armed`), déduit de `999F`.
5. **Clés dorées et coffres fixes** (`zy_e_keys_chests`). Décodage ROM (`5A49`/`5B1B`, comme `tools/build_candles_md.py`) :
   stage 4 = **10** objets (le « 11 » plus haut était inexact) : salle 0 clé (208,80) ; salle 1 clé (192,112), coffre (144,176) 0F ;
   salle 2 clé (208,112) ; salle 3 coffre (16,176) 11, clé (112,144) ; salle 4 clé (176,176), coffres (32,112) 0A et (160,112) 1C ;
   salle 5 coffre (16,144) 03. Les tables du port sont égales au décodage. Jeu rejoué en salle 4 : coffre sans clé fermé, clé prise
   (`C700` = 1, icône 17), coffre ouvert au contact (clé consommée, contenu 1C en attente 32 images, puis pris : hache), sortie et retour
   de salle : seul le coffre non ouvert est revenu. Pas de trace openMSX de la prise ni de l'ouverture (déjà noté pour le hall) :
   déduit du code `8F68`/`8C14`. Contenus des stages 4 et 5 : 03, 05, 09, 0A, 0F, 11, 1C, 1E sont portés ; **13 (5000 points, `8E42`,
   `44EC`) ne l'était pas : corrigé** (`a0f4c79`), voir `stage5-enemies.md` ; 14 (1000 points) aussi.
6. **Clé blanche, porte, sortie** (`zy_g_stage4_key_door`). Bloc caché (32,64) en salle 0 brisé au fouet depuis (52,96), clé prise ;
   porte de la salle 3 (côté gauche) ouverte en marchant vers la gauche (Simon en 27, clé consommée), sortie vers le stage 5 : arrivée
   en salle 0 (232,96) porte fermée. Données issues de la ROM (`MSX2_EXITS_VERIFICATION.md`) ; déroulement identique à
   `f_key_door` (hall) : déduit, pas de trace MSX du stage 4.
7. Voir `stage5-enemies.md` (trace de chute).
