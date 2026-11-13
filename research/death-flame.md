# Flamme de mort (objet 1E, `9A3E`) : création, place, dessin, durée, interaction

Code : `sgdk/src/enemies_death.inc` (création `deathFlame`, mise à jour `deathStep`), `enemies.c` (appels aux sites de mort, exclusions, `Enemies_sprites`),
`actors.c` (cellules de tuiles `DEATH_FLAME_TILE_INDEX`), `tools/build_death_flame_md.py` (motifs `F4` et `F8`). Tests : `tests/test_death_flame.c` (natif),
scénario `tests/md_scenarios/zzh_death_flame.txt`. Boss exclus (leur mort a son propre script, `stage3-boss.md`, `stage6-boss.md`, `stage9-boss.md`).

## Méthode

Traces openMSX (`tools/` du dépôt : moteur `harness.tcl` de `scratchpad/stage4/tool`, script de trace `scratchpad/dflame/dflame.tcl`, non versionnés) : à chaque passe
(point d'arrêt `98EC`, début de la boucle des objets C800) les 48 premiers octets des 7 blocs et `+7E`, `+7F` ; à chaque image (`4028`) la SAT, les couleurs de
ligne (CC) et, à chaque nouvel ensemble de sprites avec un objet 1E, un instantané de la VRAM et de la palette ; point d'arrêt `9A3E` (IX, pile) ; surveillance des
écritures du type des 7 blocs (PC, pile). Les ennemis sont tués par le fouet (Simon à pied d'œuvre), par les armes lancées (`C416` = 2, 3, 4) et par le contact avec
`C434` = FF (Simon amené sur l'objet à chaque passe : même chemin `9A3E`). **64 flammes** relevées (stages 1, 2, 4, 6, 7, 8, 9 ; types 01, 02, 04, 05, 06, 07, 08, 0A, 0B,
0D et un bloc 1F : 7 par le fouet, 18 par les armes, 39 par le contact), 63 vues sur leurs 16 passes (la 64e coupée par la fin du run).

## Prouvé (code et trace)

**Qui appelle `9A3E`** (recherche des octets `CD/C3 3E 9A` dans toute la ROM, 4 appels) : le fouet (`7DF9` `81AB` puis `7DFC`, PV à 0), les armes lancées (`8072`-`8075`),
le contact avec `C434` (`7EB6`-`7EB9`), et `7E9C`/`7EA1`/`7EAD` pour les objets 24, 26 et 22 (objets lâchés, ramassés par contact). Traces : pile `7D83` (fouet),
`8078` via `7D91` (armes), `7EBC` via `7D70` (contact). Les objets D700 (boule, os) passent par `822A` et `99F6`, sans flamme.

**Création** (`9A4A`-`9A69`, trace) : l'ennemi est effacé (`99F6` : `+0` et `+E` à 0, sprites hors écran), `9ABE` décide du tirage (voir plus bas), puis `5F1D` crée un objet
de type 1E dans le **premier bloc libre** (trace : ennemi tué en bloc 3 avec 0, 3, 4, 5 occupés -> flamme en bloc 1 ; ennemi en bloc 2 avec 1 et 2 occupés -> bloc 0 ; ennemi
en bloc 0 -> bloc 0), à la position **(X, Y - D)** de l'ennemi, D lu dans la table `9B33` indexée par le type (`9B22`) :

| Type ROM | 01 | 02 | 03 | 06 | 09 | 0A | 0B | 0F | 10 | 11 | 12-16 | 21 | autres |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| D | 10 | 10 | 10 | 10 | 10 | 10 | 10 | 18 | 10 | 20 | 18 | 10 | 0 |

**Table lue et exécutée sur la ROM** (`tools/verify_death_flame_z80.py`, `tests/run_death_flame_z80.sh` : `9A3E` dans l'émulateur Z80, un objet de chaque type tué en (120, 176), flamme relevée) : D = octet `9B33 + type` (type 1 en `9B34`), 22 types de la table, 20 flammes créées en (X, Y - D) ; le type 09 devient dormant (`9AA9`), 11h n'est pas effacé par ce chemin. **Types 03 et 21h (homme-poisson du stage 10) : D = 10h**, comme le type 02 ; 0F (aigle) 18h ; 12h-16h 18h ; 11h 20h. Le port avait 0 pour 03 et 21h (`deathOffset` : flamme dessinée 16 px trop bas) : corrigé, `tests/test_death_flame.c` (`kTypes`, FISH3 et EAGLE). Vérifié en trace openMSX : 01 (Y A0 -> 90), 02 (BD -> AD, 4A -> 3A), 04 (B2 -> B2), 05 (80 -> 80), 06 (C0 -> B0), 07 (78 -> 78), 08 (C4 -> C4), 0A (50 -> 40), 0B (90 -> 80), 0D (BB -> BB) ;
X identique. Octets de la flamme à la création : `+0` 1E, `+1` 0, `+2/+3` Y, `+4/+5` X, `+6` 0 (immobile : `99B9` ne la déplace pas), `+9/+A` 0220 (sans effet), `+B` 85, `+C` 10h,
`+D` 1, `+E` 0, `+1F` 0 ou 1 (le `B` de `9A58` : tirage du drop), `+7E` 0, `+7F` 1. Le nombre d'entrées SAT de l'objet (table `6057`, type 1E) est 2 : la seconde est
hors écran (Y 225, couleur 0).

**Routine `9B71`** (une fois par passe, comme tout objet C800) : `+B` = 85 si le bit 2 de `+C` est nul, 86 sinon ; `+C` - 1 ; à 0, `99F6` (objet effacé, sans dessin) ;
si `+1F` est non nul, un objet 24 est créé à la place (`9B90`, le drop). La flamme est donc mise à jour **16 fois** (passes P + 1 à P + 16, P = passe de la mort, qui commence
après la boucle des objets : la flamme n'est pas mise à jour dans sa passe de création). Séquence relevée (état au début de chaque passe, `+B`, `+C`, identique pour les 63 flammes) :

| passe | P+1 | +2 | +3 | +4 | +5 | +6 | +7 | +8 | +9 | +10 | +11 | +12 | +13 | +14 | +15 | +16 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `+B` | 85 | 85 | 86 | 86 | 86 | 86 | 85 | 85 | 85 | 85 | 86 | 86 | 86 | 86 | 85 | 85 |
| `+C` | 10 | 0F | 0E | 0D | 0C | 0B | 0A | 09 | 08 | 07 | 06 | 05 | 04 | 03 | 02 | 01 |

**Dessin** (`6445`, une fois par passe après la mise à jour) : la flamme est dessinée **dès la passe de sa création** (pose 85), puis à chaque passe jusqu'à la 15e mise à jour ;
la 16e l'efface sans dessin. 16 passes visibles = **32 images** à l'écran (SAT du run A : images 1403 à 1434, zombie effacé et flamme visibles à la même image, P + 3 : retard
d'affichage de `frame-timing.md`). Une entrée SAT, **(X - 8, Y - 16)** en pose 85 (motif `F4`) et **(X - 8, Y - 15)** en pose 86 (motif `F8`), couleur de ligne **8** sur les 16 lignes (CC =
08 : pas de bit OR, une seule couche), couleur MSX 8 = (R 7, G 0, B 1) dans les 8 salles relevées. Les motifs `F4` et `F8` sont identiques octet pour octet dans les 8 stages
(empreintes md5 de 32 octets sur 8 instantanés), à la base `F800` des motifs de sprite.

**Interaction** : `+E` = 0 : ni contact (`7E67` exige `+E` bit 0), ni fouet (`7DAD`), ni arme (`7F49`). `+7E` = 0 : `992F` ne la fige pas quand `D010` est non nul (arrêt du temps :
`+7E` = 0 -> `RET Z`, lu dans le code). Elle **occupe un bloc C800 et 2 entrées SAT** pendant 16 passes : un canal d'apparition ne peut pas créer d'ennemi dans ce bloc ni, si 14 entrées
SAT sont prises, dans la salle (`5F1D`). Aucune sortie d'écran (`99E5`) : l'objet est immobile. Dans le port, un changement de salle l'efface avec tous les ennemis (`Enemies_setRoom`, testé) ; la ROM efface de même les objets de la salle (non tracé pour la flamme).

## Drop (`9ABE`-`9B21`, `9B71`/`9B90`, `9BD9`, `9C1E`) : tracé et porté

Traces openMSX (session, `scratchpad/drop` : `dr.tcl` = points d'arrêt `9AD1`, `9AE7`, `9AF0`, `9B13`, `9A61`, `9B90`, `9A97`, `9A92` et `98EC` avec les objets 1E/24/26 et `C500` à chaque
passe ; mêmes salles que ci-dessus, poursuite par `C434` = FF puis Simon écarté pour laisser vivre l'objet ; `FORCE=heart|big|bonus` écrit A = 4, A = 40h ou `CF40` = 1F au point d'arrêt pour
avoir beaucoup d'exemples) :
- **Ordre** (code, confirmé en trace) : boss (`CE00`) : rien ; type 0E : 4 flammes ; ennemi porteur (`+1F` non nul) : flamme 26 de cet objet ; sinon `CF40` + 1 ; `CF40` & 1F = 0 : flamme 26 portant **13h** (5000 points) ;
  sinon `A` = R : `A` & 3F = 0 : flamme 26 portant **02** (grand cœur) ; sinon `A` & 3 = 0 : la flamme de mort porte `+1F` = 1 ; sinon flamme de mort seule. R n'est donc pas lu à la 32e mort.
- **Chiffres** : 160 lectures de `LD A,R` en `9AE7` : A < 80h toujours ; A & 3F = 0 : 3 fois (attendu 2,5) ; A & 3 = 0 hors A & 3F = 0 : 36 fois = 22,5 % (attendu 23,4 %). L'agent hasard : 250 appels stages 1-14, 25,8 % du reste.
  `CF40` : 0 au premier mort après le chargement du jeu (seul effacement : `44C6`), +1 à chaque passage en `9AD9` (pas pour le boss ni le type 0E), jamais remis ensuite : la 32e mort laisse 13h (vu 3 fois, `CF40` = 20h) ;
  A = 0 ou 40h : objet 02 (vu 5 fois en tirage naturel, 48 en forcé).
- **Porteur** : 135 morts d'ennemis, `+1F` toujours 0. Code : `5F1D` met `+1F` à B (`CFFB`) qu'il force à 0 ; seule l'entrée `5F1F` le prend, appelée avec B = 0 par le chargement des objets de salle (`61F6`) et par `89D7` ;
  les seules écritures de `+1F` sont `9A2F` (0), `9A61` (B), `9B1D` (C). Aucun ennemi n'est porteur : le cas n'est pas porté.
- **Flamme 1E avec `+1F` = 1** : même flamme (16 mises à jour). À la 16e (`9B90`) l'objet 24 naît dans le même bloc, en (X, Y) de la flamme ; `99B9` le déplace dans la même passe (X + 2 px, Y + ½ px) : au 1er
  échantillon suivant X est déjà + 2. Aucune différence de forme pour la flamme.
- **Objet 24** (`9BD9` puis `99B9` et `99E5`), 39 cœurs utilisables en trace : à chaque passe test d'atterrissage `7B98` (tuile en (X, Y), puis en (X - 10, Y), limite par stage de `7C5E`, tableau `7C78`),
  `+C` + 1, vx + (`+10`/`+11`) = - 20h (oscillation : à `+C` = 20h, `+C` = 0 et le signe change), X + vx, Y + ½ px (+ 1 à chaque `+C` pair) ; sortie `99E5` : Y >= E4, X >= F1 ou X < 7. Départ vx = 200h, `+C` = 1.
  Une simulation Python puis C reproduit **chaque passe (X, Y, vx, `+C`) des 51 trajectoires** (jusqu'à 207 passes, plusieurs retournements). Atterrissage (`9C03`) : objet C500 83 en (X & F8, (Y - 16) & F8) (36 sur 36 ;
  l'ancien arrondi du port `(y + 4) & F8` donnait jusqu'à 8 px d'écart), repos 8 x 32 images comme les autres objets (122-128 passes vues). Ramassage au contact : `7E9C` -> `9A3E` -> `9A97` -> `8D2C` avec A = 1.
- **Flamme 26** (`9B13`, à la place de la flamme de mort, même X, Y, aucun objet 1E) : 20 passes immobile (`+C` 13h à 1), puis chute de 8 px toutes les 2 images (test `7B98` à chaque pas), atterrissage ou sortie (`99E5`, Y >= E4 :
  23 flammes sur 47 sont tombées hors de la salle sans objet) : objet 83 en ((X - 4) & F8, (Y - 16) & F8). Identique aux flammes de bougie (`candle-flames.md`), mais Y d'un ennemi n'est pas multiple de 8 : l'arrondi
  en Y est `(Y - 16) & F8`, pas `(y + 4) & F8`. Le contact avec la flamme en l'air donne l'objet à l'instant (`7E9F`, vu avec `C434` = FF ; non porté).

## Vitesse du cœur : correction de candles.c

Le cœur de bougie (même objet 24, même routine) tombait deux fois trop vite dans le port : `step_drop` appliquait à chaque image les valeurs d'une passe (vx 200h, ax 20h, Y + 1 toutes les 2 mises à jour). Mesuré : 16 px en 31 passes = 62 images
(le port : 32). Les cœurs font maintenant une mise à jour par passe, dans la phase de leur création, avec les règles ci-dessus (test d'atterrissage à deux points et limite de stage, sortie `99E5`, rangement `9C03`).
Les flammes 26 utilisent le même test (limite de stage au lieu de `< 9`, plus de limite `DB`) : les 4 flammes de bougie de traces restent justes.

## Non porté ou non vérifié

- Le bloc C800 qu'occupe la flamme 26 (~45 passes) et le cœur (~30 passes) : les canaux d'apparition voient un bloc libre de plus.
- Stage 6 salle 0 : 11 cœurs atterrissent à Y = A0 (A4, A7) sur une plateforme mobile dont les tuiles sont écrites dans la carte vivante `D100` ; la carte statique du port les laisse tomber jusqu'à C0. Une flamme 26 du stage 9 salle 4 (E9, 7C)
  traverse ce que la carte statique du port bloque. Écartés des tests.
- Position du sprite du cœur en l'air : SAT en (X - 4, Y - 8) pour le motif E8 (mesuré), le port dessine la cellule de la feuille en (x, y) = (X - 8, Y - 16) (inchangé, provisoire).
- Contact avec le cœur ou la flamme 26 en l'air : ramassage du port inchangé (`touches_drop`, boîte C500), pas la boîte `8290`.
- Mort du boss et type 0E non portés ; le tirage utilise `s7Random` (= `Enemies_randomR`, même flux que la puce et l'os), forçable par `Enemies_s7R` (tests natifs et scénarios : 1 = aucun drop, 4 = cœur, 0 = grand cœur).

## Porté

- Type d'objet `ENEMY_TYPE_DEATH` (16), champs existants : `x`, `y` = X, Y de l'objet, `anim` = `+C`, `pose` = `+B`, `age` (parité de mise à jour). Création par `deathFlame` à tous les sites de mort
  des objets C800 : fouet (types 1, 2, 4, 5, objets 06 à 0D, 1C), armes lancées, contact avec C434 ; pas pour les projectiles D700 ni les gouttes.
- Mise à jour : une fois toutes les deux images, la première deux images après la création (`age` part de 1) ; `s->frozen` ne la fige pas ; 16 mises à jour, effacement à la 16e. 32 images
  d'affichage (scénario : `age` 1 à 32, flamme effacée à l'âge 33).
- Exclusions : contact, fouet de type 1/4/5/2/objets 06-0D, armes lancées, chute (`fall`), `advance`.
- Dessin : cellules de 4 tuiles `F4` / `F8` (`vk_death_flame_tiles`, couleur 8 de PAL2) en VRAM 752 à 759, après les tuiles de la porte (`item_view.c`), avant le bloc ENEMY5 (`_Static_assert` dans `actors.c`).
- Harnais `tools/md_harness.py` : `enemies.enemy[i].active` vaut 0 pour une flamme (champ `.flame` = 1), `enemies.alive` ne la compte pas, pour que les scénarios qui attendent la mort d'un ennemi
  restent valables.
- Tests existants adaptés : `Enemy_alive(e)` (`enemies.h`) dans `test_enemies.c`, `test_stage7.c`, `test_stage8.c`, `test_ghost.c`, `test_weapons.c`, `enemy_integration.c` (le bloc de l'ennemi tué tient la flamme) ;
  `w_simon_blink_phase.txt` retire les ennemis (une flamme rouge sous Simon fausserait le comptage de pixels rouges).

## Non porté

- Type 0E (quatre flammes, aucun ennemi du port) ; les objets 09, 11, 18, 22 de `9A6B` ; compteur `CE08` (+1 par mort, non lu dans le port) ;
  flammes de mort des boss (`CE00` non nul : aucune différence de forme relevée, mort à script).
- Comparaison à l'image près avec le MSX : la mise à jour du port et celle de la passe MSX sont alignées par la règle des deux images de `frame-timing.md` ; le calendrier d'affichage du MSX (P + 3)
  n'est pas mesuré sur le port (la flamme suit le retard de tous les sprites d'ennemis).
