# Armes lancées : dague, hache, croix, eau bénite (C416 ≥ 2, objet 1E)

Statut : **prouvé** (code de la banque 1 lu, trace openMSX image par image) et **porté**
(`sgdk/src/weapons.c`, `tests/test_weapons.c`, scénario MD `zb_secondary_weapons`).

Correction des notes précédentes : les objets 1B, 1C et 1D ne sont **pas** des fouets de
niveau 2 à 4. `whip-variants.md` voyait dans `C416` = 2 et 3 des fouets dont la lanière
serait les compositions 12-14 / 27-29 : ce sont les poses de lancer de Simon, et le
« verrou » de 94 images (droite) ou 6 images (gauche) au niveau 3 est la vie de la hache
lancée (boomerang rattrapé à droite, bord gauche atteint aussitôt à gauche depuis X 16).

## 1. Objets et HUD

| Objet | Effet (`8D70`, `8D8D`) | Icône |
| --- | --- | --- |
| 1A | `C416` = 1, fouet chaîne | — |
| 1B | `C416` = 2, **dague** | case du HUD en (80h, 0Ch), `8E9A` |
| 1C | `C416` = 3, **hache** | idem |
| 1D | `C416` = 4, **croix** (boomerang) | idem |
| 1E | `C701` bit 3, **eau bénite**, son 12 | case des objets (`8EE6`) |

`8D70` : `C416` = objet - 19h, `C440` (minuterie) et icône `8E9A`. Un objet 1A-1D égal à
l'arme tenue sort comme petit cœur (`8A29`, déjà noté dans `castle-candles.md` 9.6).
Aucun coût en cœurs pour 1B à 1D ; l'eau bénite coûte 5 cœurs. Planche des icônes :
`scratchpad/secondary-weapons/item_sheet.png` (hors dépôt).

## 2. Touche (banque 1, `7116`)

- ESPACE (`C006` bit 4), `C422` = 0 : `C436` = `C416`. Si `C436` ≥ 2, `71BE` cherche un
  emplacement : `B` = 2 (`C450`, `C460`) pour la dague, 1 (`C450`) sinon ; le premier dont
  `+1` est nul reçoit `+0` = 1. Aucun libre : **l'appui est ignoré** (ni pose ni son).
- Coup `C416` ≥ 2 : 4 passes (`C422` 1 : `71FA`, `C42F` = 12 et chargement des
  graphismes `5593` ; 2 : `7240`, 13 ; 3 : `7250`, 14 ; 4 : `7262` → `7286` : `999F`,
  `C422` = 0, `C42F` = 0 et lancement : l'emplacement `+0` = 1 reçoit `+6` = 18h,
  `+8` = `C42C`, `+1` = `C436`). Le son 05 n'existe que pour les fouets (`7213`).
  Trace : appui image 2038, lancement 2044.
- `7D7E` : avec `C416` ≥ 2, les trois tests du fouet (`7DAD`, `7F49`, `7FB7`) sont
  remplacés par une boucle d'attente (`7DA0`) : **le fouet ne touche plus rien**.
- Eau bénite (`714D`, avant l'arrêt du temps `715F`) : `C701` bit 3, en saut
  (`C420` = 1), `C417` ≥ 5, GAUCHE (prioritaire, `C468` = 1) ou DROITE nouvellement
  pressée, `C461` = 0 → `C461` = 5, `999F`, cœurs - 5. Elle n'utilise que `C460` et
  n'empêche pas l'arme principale.

**Touche MD** : B lance l'arme (comme le fouet) ; GAUCHE ou DROITE pressée en saut lance
l'eau bénite (touche MSX gardée, comme BAS en saut pour l'objet 0A).

## 3. Emplacements `C450` / `C460` et passe `72B2`

Octets : `+0` état, `+1` type, `+2` dy, `+3` dx, `+4` Y, `+5` X, `+6` motif, `+7`
compteur, `+8` sens. Une passe toutes les deux images :

1. gestionnaire par type (table `72D6`) : 2 → `7436`, 3 → `7473`, 4 → `72DE`, 5 → `73A4` ;
2. `74BB` : Y += dy, X += dx ;
3. `74CE` : `Y - D8 < 16` → libéré ; `X - FB < 10` (bords) → libéré, et si `C416` vaut
   3 ou 4, `8E93` : **l'arme est perdue** (`C416` = 0, case du HUD vidée) — quel que soit
   l'emplacement, l'eau bénite comprise (trace image 3642) ;
4. `7535` : sprites (SAT `D618`/`D620`, Y - 16, X - 8, motifs N et N + 4 ; couleurs
   `D460`/`D480` : croix 0F/0E sans CC, les autres 2/4Ch avec CC, flammes 8/0).

`7513` (Y = E0, vidage) au chargement de salle (`639E`) et à la porte (`774F`) ; l'état 4
(`63B7`) efface `C420`-`C46F`.

## 4. Trajectoires (trace `scratchpad/sw/A`, hall salle 1)

| Arme | Code | Départ | Mouvement | Fin | Trace |
| --- | --- | --- | --- | --- | --- |
| Dague | `7436` | Y = `C425` - 16 (- 10 accroupi), dx ±5, motif 20h droite / 18h gauche, son 04 | droite | bord de l'écran | X 65, 70 … 250, libérée image 2121 (76 images) |
| Hache | `7473` | vitesse 3, motif 18 + 8·((`C003` >> 1) & 3), son 06 si `C003` & 7 = 0 | état 2 : 24 passes ; état 3 : dx - 1 aux comptes impairs, 23 passes ; état 4 : retour | rattrapée par Simon (`8240` → `847A`, HL = 1008) en états 3 et 4 | X 61 … 133 (état 3), 139, … 67, rattrapée 2543 (94 images) |
| Croix | `72DE` | vitesse 5, motif 18 + 8·((`C003` >> 2) & 3), son 03 | idem | idem | X 21 … 161 … 23, rattrapée 3180 (126 images) |
| Eau bénite | `73A4` | états 0 et 1 : départ (deux fois), Y = `C425` - 16, dx ±2, motif 38h | état 2 : Y += 2·`707D`[compteur] (FD FD FE FE FE FF FF FF FF 00 00 00 00 01 01 01 01 02 02 02 03 03, dernier tenu) ; sol `7B98` en (X, Y) et (X - 10, Y) → état 3, son 18 | flammes F4/F8 (`C003` & 4) 24 passes | lancée 3478, sol 3540, fin 3588 |

Bord pendant le vol (`7359`) : à `X - 10 ≥ ECh`, état 4 direct, dx ∓5 (hache lancée à
gauche depuis X 56 : 53 … 8, puis 13 … 48, rattrapée, image 2865).
Hache et bloc : si `C433 - 2 < 2` (un projectile a touché un bloc de mode 2 ou 3), la hache
tombe comme objet 1C (`8992` en (X, Y - 16), sans `8A29`), est libérée, et `8E93`.

## 5. Collisions

- `8500` : |X - D| < L + 6 et (Y - E + H) & FF < H + 6 (dague) ou H + 12 (autres).
- Ennemis `801E` (si `C450` | `C460` non nul) : premier ennemi de `C800` touché par
  l'emplacement 0 puis 1, un seul par passe. Dégâts `8048` : dague 1, hache 4, croix 2,
  eau 2 (les ennemis des stages 0-2 ont 1 PV) ; points `81AB`, son 0C ; la dague disparaît.
  Classes : types 1 et 2 H = 18h, L = 5 ; type 4 H = 10h, L = 8 ; type 5 H = 0Ah, L = 0Ch.
- Bougies `80A6` (`8460` : mode < 2 D = X + 8, E = Y + 16, H = 10h, L = 8 ; blocs D = X + 16,
  E = Y + 32, H = 20h, L = 10h) : une par passe, `C433` = mode, dague libérée, son 0C.
  `C433` est remis à 0 par tout ramassage (`7FFD`) et par l'état 4.

## 6. Graphismes

`5593` décompresse (RLE `46F1`, banques 9/10) : dague `A24E` (1 image), hache `A4C9`,
croix `A272` (2 images), miroirs `477F` (18 → 30, 20 → 28 ; dague 18 → 20) ; bouteille
`A147` → F9C0 (motif 38) ; flammes `A185` → FF00 (F4 à l'offset A0h). Vérifié contre la VRAM
des instantanés. Extraction MD : `tools/build_weapons_md.py` →
`sgdk/src/generated/weapons_md.c` (13 images : dague 0-1, hache 2-5, croix 6-9, bouteille
10, flammes 11-12, PAL2).

## 7. Objet 04 (C701 bit 5)

`8610` : au contact, les projectiles `D700` sont détruits, son 0B, `C441` - 1 ; à 0,
`C701` bit 5 effacé et `8EE6`. Dans les stages 0-2, `D700` ne contient que les flammes de
mort (type 0C exclu) et peut-être la goutte : **non porté** (aucun projectile ennemi
porté). 04 et 1E ne sont dans aucune bougie des stages 0-2 (marchand seulement, table
`9428`, non porté) ; on les obtient par la RAM ou les scénarios.

## 8. Port MD

- `sgdk/src/weapons.c/.h` : emplacements, passe `72B2`, gestionnaires, bords et perte,
  collisions `8500`/`8460`, eau bénite ; sans SGDK, testé par `tests/test_weapons.c`
  (trajectoires de la trace ci-dessus).
- `whip.c` : niveaux 2-4 = lancer (4 passes, 6 images de pose 12-14, aucune boîte de fouet).
- `enemies.c` : `Enemies_shotHit` ; `candles.c` : `vk_candles_weapon_lost`,
  `vk_candles_drop`, icône `hud_weapon` ; `candle_view.c` : icône en (80h, 0Ch).
- `actors.c` : sprites 1 et 2 (PAL2, Y - 15, X - 8, deux images de retard comme Simon),
  sons, perte, chute de la hache, points.
- `main.c` : B (réserve `71BE`), GAUCHE/DROITE en saut (eau bénite), remises à zéro à la
  salle, à la porte, à la mort et au changement de stage.
- Scénario `tests/md_scenarios/zb_secondary_weapons.txt` : dague (2 emplacements, 3e appui
  ignoré), hache et croix rattrapées, eau bénite (5 cœurs, flammes).

Captures (hors dépôt) : `scratchpad/secondary-weapons/` (MSX `msx_f02466.png` hache,
`msx_f03072.png` croix, `msx_f03500.png` et `msx_f03525.png` eau bénite ; MD `daggers.png`,
`axe.png`, `cross.png`, `holy_water.png`, `flames.png` ; planche `weapons_preview.png`).

## 9. Ouvert

- Lancer accroupi : la pose accroupie (composition 6 / 12-14) n'est pas dessinée à part.
- Cadence des passes : le port fait une passe toutes les deux images avec sa propre phase,
  non calée sur `C003` de la ROM (à ±1 image).
- Objets muraux (`80DC`) touchés par les projectiles : non portés.
- Objet 04 et projectiles `D700` (section 7), marchand.
- Dégâts > 1 (hache 4, croix 2) sans effet tant qu'aucun ennemi porté n'a plus d'1 PV.
