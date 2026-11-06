# Boss du stage 9 (deux objets 14h, salle 7), clé, porte, orbe et passage au stage 10

Code : `sgdk/src/boss.c` (`kind` = 2 : `walker_pass`, `walker_fire`, `room9_pass`, boule D700 type 7 dans `shots_pass` ; le dégât, la barre, le script
de fin `B6AB` et l'orbe sont ceux des stages 3 et 6), `actors.c` (tuiles, cellules clignotantes, contacts, armes lancées par cible), `main.c` (salle du
boss par `vk_boss_room_of`). Tuiles : `tools/build_boss9_md.py` sur des instantanés VRAM d'openMSX non versionnés (`scratchpad/stage9/out/{R,C2,C}`) ;
l'orbe est celui du stage 6 (`vk_orb6_tiles` : 0 mot de différence sur 384 avec l'orbe construit depuis la trace C2). Tests : `tests/test_stage9.c` (natif, avec
`tests/stage9_boss_trace.inc` généré par `tools/gen_stage9_trace.py`), scénario `tests/md_scenarios/zzd_stage9_boss.txt`.

Traces (stage 9 forcé : `D000` 9, `D002` 2, `D001` 7, `C000` 4) : R = 500 images par salle, Simon immobile ; B = Simon posé en X 60, 140, 220, 100, 200, 30
toutes les 120 images (30 déplacements), sans toucher au boss ; C et C2 = `C418` posé à 8, objets placés par `poke` devant le fouet (C2 : 8 tentatives,
mort, orbe laissé au sol, sortie). Script : `scratchpad/stage9/plans/{R,B,C,C2}.txt` (non versionnés), lancés par `scratchpad/stage4/tool/run.py`.

## Prouvé (ROM et trace, chiffres)

**Existence.** `7777`[9] = 1 (boss), `636F`[9] = `73` : salle 7, `CE00` = 3, script de salle `B857` (table `B6B9`[2]). Pas de bust à effacer ni d'attente :
à la première passe `CE01` = 0 crée deux objets 14h par `5F1D` en (X 30h, Y C5h) et (X D0h, Y C5h) (`B863`, `B86B`), puis `CE01` = 1 attend `CE15` (la mort,
`B874` -> `BE47`). Trace R : salle chargée image 4464, `CE00` 3 à 4488, création 4506-4508 (premier échantillon : `+10` = 1Eh). Les 12 entrées SAT des deux
objets (`6057`[14h] = 6) ne laissent de place qu'à 2 boules (14 entrées au total) : `VK_BOSS_SHOTS` = 2.

**Porte.** `kStageItems[8]` : clé visible (208, 96) en salle 8, porte (12, 128) à gauche ; `77D1` : lien gauche de la salle 8 = salle 7, X 246, Y de la salle de
la porte puis chute. Scénario : clé prise en (216, 98), porte ouverte, salle 7 en (246, 176).

**Objet 14h** (init `B87C`, routine `B893`, états `B8A1`, `B8AD`, `B8D0`, `B960`, animation `B9CF`, hasard `B9B6`). Pose 36, `+10` = 20h, `+14` = 2, `+15` = 1. État 0 :
`+10` passes puis mobile ; état 1 : vx -230h si Simon est à gauche (`+12` = 1), sinon +230h (`+12` = 0), `+13` = 7, `+10` = table `B9C7`[R & 7] (`40 08 30 10 20 18 38 28`) ;
état 2 : animation (poses 33 34 35 vers la gauche, 36 37 38 vers la droite, `+14` = 1, 0, 1, 2, 1... toutes les 8 passes), `+10` - 1 ; hors de X 20h à E0h
demi-tour et X ramené à 20h ou E0h (`B930`) ; à 0 : nouveau `+10`, `+17` + 1, sens pris selon le bit 0 de `+17` (inversion) ou selon la place de Simon, et
arrêt (état 3, `+11` = 30h) quand l'objet regarde Simon ; état 3 : 30h passes (la seconde couche des cellules clignote, couleur 08, quand `+11` est pair), puis la
boule et retour à l'état 2. Vitesse de marche 2,19 px par passe. Le déplacement `99B9` suit la routine.
- **Passe par passe** : R 488 transitions, B 658 : toutes reproduites octet pour octet (état, Y, X avec fractions, vx, vy, pose, `+10` à `+17`, 0 écart, 24 avec un
  nouveau `+10` : le hasard `R` n'est pas reconstruit, les 8 valeurs de la table sont essayées) ; C2 : 567 transitions, 558 reproduites, 9 écarts dus aux `poke`
  du plan (objet déplacé entre deux échantillons).
- **Création** : l'octet `+9/+A` (vx) garde la valeur de l'occupant précédent du bloc (FDC0, FD80, FFE0 vus) ; le port part de 0, d'où au plus 2 px d'écart pendant la
  passe où l'objet devient mobile.

**Boule** (D700 type 7, `A08E`[14h] = 7, init `A04D`, routine `9E90`). En (X, Y - 18h) sans fractions, visée `A0ED` depuis (X, Y - 38h) vers Simon avec la vitesse 40h + 8 D012,
la composante X forcée dans le sens du regard (`B993`-`B998`) ; `+10/+11` = (0, Y), pente `+12/+13` = vy - 100h. Chaque passe : `+10/+11` += pente, vy += 30h si Y < `+11`
sinon -30h (`A549`, borné à 7FFh), pose 39 ou 3A selon le bit 2 de `+C` avant l'incrément, déplacement, suppression `99E5`. Première boule de la trace R (Simon (16, 128), objet en
(9Ah, C5h)) : Y ACB8h, X 9802h, vy FFB8h, vx FE02h (le test natif et le scénario les reproduisent). Passe par passe : R 169 transitions (168 identiques), B 258 (254) ; les
5 autres sont des échantillons en cours de passe. Son : effet 0A (`A4E8`).

**Dégâts** : fouet -4 (`7E59`[3], `C416` = 0 ou 2 ; 6 sinon, `7E60`) : trace C et C2 `C418` 8 -> 4 ; barre de 80h : 32 coups. Corps -4 (paire 19 de `81CE` : `20 02`, doublée), 2000
points ; boule -1 (trace R et B : 3 pertes de 1) ; corps -4 (trace C2 : 32 -> 28). Boîtes de classe 6 (`82CB`[14h] = 6) : corps `8420`, fouet `8428`, armes `8430`,
(H, L) = (28h, 5) ; boule de classe 1 (`830D`[6] = 1) : (6, 6), `8338`. Mort (`7E17`, C2 image 1344) : effets 0C et 1C, `C445` = 20, `CE15` = 1, les deux objets et la boule
disparaissent (`7806`).

**Fin** : script `B6AB` commun. C2 : `CE10` 0 à 1345, 1 à 1350, 2 à 1469 (60 passes), 3 à 1472 (orbe 22h en (78h, 40h), 125 images après la mort), au sol en Y **C0h** à l'image
1900 (436 images après sa création : 90 passes d'attente + 128 de chute ; scénario 430 à 445), sans prise `CE10` = 6 à 2265 et musique 8C, sortie 288 images plus tard ; stage 10, salle 0, Simon
en (32, 144) (`kStageEntries[9]`, trace : 2598). Palette : entrées 5 et 7 de (6,3,5)/(6,3,3) à (3,0,0)/(5,0,0) à la création de l'orbe, comme au stage 6.

**Rendu** : cellules 16 x 16 en (X - 8, Y - 46 + 16 k), k = 0, 1, 2 (SAT Y - 47, Y - 31, Y - 15, +1) ; boule en (X - 6, Y - 14). Les cellules sont composées (couleur 2, puis couleur 12 avec CC) ;
les cellules clignotantes (couleur 08 sans CC) sont recalculées à l'exécution depuis celles de la pose (`boss9_flash_cells`).

## Déduit (code lu, pas mesuré en jeu)

- Armes lancées sur le boss : même chemin que les stages 3 et 6 (une touche par aller et retour, `8083`/`999F`), dégât de `C416` ; non jouées au stage 9.
- Orbe pris et soin : script commun, rejoué en scénario (`CE11`), non tracé au stage 9 (la trace C2 laisse l'orbe au sol).
- Limites exactes des boîtes (corps, fouet, boule) : les mesures de dégâts prouvent le contact, pas la limite en pixels.
- Score 2000 : `81CE`, non lu sur le HUD MSX. Temps arrêté : boss et boules figés par `98E5`, non rejoué.

## Anomalie du stage 6, corrigée : boîte de la boule

`830D`[type - 1] donne la classe de la boîte d'une boule D700 : la boule du boss 6 est de classe 1 -> `8338`, boîte (6, 6) ; `boss.c` utilisait (6, 3).
Corrigé (`kind` 1 et 2 : (6, 6) ; seul le stage 3 garde (6, 3)). Traces : voir `research/boss-boxes.md`.
