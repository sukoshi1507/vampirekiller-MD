# Entrée du château : cour salle 2 → hall salle 0

Statut : **prouvé** par trace openMSX image par image (runs A et B) et par lecture du code (`7718`, `778A`, `8580`, `55EC`, `4347`). Porté sur MD (`gameplay.c`, `main.c`), le son excepté.

Constat de départ : sur MD, Simon passait devant le château et changeait de salle dès x ≥ 232. Sur MSX, il franchit une porte : marche automatique ralentie, Simon passe derrière le montant droit de la porte, puis écran noir et hall.

## 1. Méthode

- openMSX 21.0, `C-BIOS_MSX2`, ROM de référence en cartouche (lecture seule).
- Harnais : copie de `scratchpad/type5/tool/harness.tcl` et `run.py`, plus `en_extra.tcl` (journal par image au point d'arrêt VBLANK `4028` dès que Simon passe x 180 en cour 2 : `C000`, `C001`, `D000`/`D001`, X `C427`, Y `C425`, `C420`, `C42C`, `C5AC`..`C5AF`, `C408`, `R1`, `R2`, `R8`, `R23`, `C096`, `C0A7`, `C010`, `C016`, SAT complète ; points de surveillance d'écriture avec PC sur `C000`, `C001`, `C5AC`, `C408`, `C096`, `C0A7`, `C010`, `C016` ; instantané VRAM à chaque image pendant 200 images).
- Plans : **A** = `hold R 1500` ×3 (DROITE tenu jusqu'au hall), puis `wait 300`. **B** = DROITE jusqu'à x 222 (`gotox 222`), puis plus aucune touche.
- Fichiers (hors dépôt, ignorés par git) : `scratchpad/entrance/` du worktree (`tool/`, `plans/`, `runs/A`, `runs/B`). Rendu des instantanés par `render_snap.py`, poses par `tool/poses.py` (empreinte des motifs de sprite 0 à 15).
- Les valeurs du journal sont lues au début de l'image N, avant la logique de cette image : elles décrivent l'état laissé par l'image N-1.

## 2. Chronologie mesurée (run A ; run B identique image pour image)

| Image | Événement | Preuve |
|---|---|---|
| 1449 | X atteint 216 (marche normale, 1 px par image) | `en.csv` |
| 1451 | `C5AC` 4 → 5 (`7755`, `INC (HL)`), effet sonore `C096 = 0x15` (`517D`), `C016 = 9C` | `wr.csv` |
| 1451-1454 | X reste à 216 | `en.csv` |
| 1454 | motifs 0x20..0x2F chargés en VRAM (copie du montant de la porte) | instantanés |
| 1455 | 8 sprites ajoutés en tête de SAT (indices 0 à 7, donc devant Simon) : X 224, Y SAT 143 et 159 (lignes 144 à 175), motifs 20, 24, 28, 2C sur deux rangées | `en.csv` |
| 1455-1515 | X augmente de 1 toutes les 4 images (217 à 1455, 232 à 1515), touches ignorées | `en.csv`, runs A et B |
| 1515 | `C408 := 1` (`7802`), X = 232 | `wr.csv` |
| 1516 | `C000 := 10` (`41B6`) | `wr.csv` |
| 1517 | `D000` 0 → 1 (`435A`), `D001 := 0`, `C408 := 0`, `C000 := 4` ; `C410`/`C411` +1 en BCD (code `4347`, STAGE-01 dans le HUD de l'instantané `f01556`) | `wr.csv`, `research/room-links.md` |
| 1518 | affichage coupé : `R1` 0x62 → 0x22 (bit 6), sprites coupés `R8` 0x08 → 0x0A (bit 1) | `en.csv` |
| 1527 | fin de l'effet 0x15 (`C096 := 0` en `89D7`, `C016 := F1`) | `wr.csv` |
| 1534 | Simon posé en x 16, y 192 | `en.csv` |
| 1536 | `C5AC := 0` (`638E`) | `wr.csv` |
| 1554 | affichage rétabli (`R1` = 0x62, `R8` = 0x08) : hall à l'écran | `en.csv` |
| 1555 | `C000 := 5` (`4251`) : retour en jeu | `wr.csv` |
| 1555-1556 | les 8 sprites du montant restent visibles à x 224 sur le décor du hall (SAT non mise à jour) | instantané `f01556` |
| 1558 | sprite de Simon visible à x 16 | `en.csv` |

Durées, comptées depuis le déclenchement (t = 0 à l'image 1451) : X = 216 + t/4 ; sortie à t 64 ; affichage coupé de t 67 à t 102 (**36 images noires**) ; hall affiché à t 103 ; jeu à t 104. De la porte au hall : 103 images (environ 1,7 s).

Poses (empreinte des motifs) : le cycle de marche continue pendant la marche lente, 0, 1, 2, 1, chaque pose tenue 8 images, comme la marche normale. Simon avance donc en « marche au ralenti » : même animation, quatre fois moins vite.

Run B : DROITE relâché à x 222 (image 1476). Simon continue seul, à la même cadence, et sort à la même image (1515). La marche est automatique et les touches sont ignorées.

## 3. Code (banque 1 en 6000, banque 0 en 4000, banque 2 en 8000)

- `7718` : `LD A,(C5AC)` puis `CALL 406B` (saut indexé). Table : 0 → `7805` (RET), 1 → `772A`, 2 → `7805`, 3 → `778A`, 4 → `772A`, 5 → `778A`. La cour 2 entre avec `C5AC = 4` (`BB5D`), la salle 7 du hall avec 1.
- `772A` (état 4 en cour) : `CALL 8580` (zone de la porte), `RET NC`. En cour (`D000 = 0`), le test de la clé (`7737`) est sauté. Puis `CALL 8EBA`, `C422 := 0`, `CALL 7806`, `CALL 7513`, `INC C5AC` (`7755`), et l'effet sonore : `C5AC = 5` donne `0x15` (`7772`), sinon `0x1A` (`776D`). La table `7777` indexée par `D000` vaut 0 pour la cour : pas de `CALL 509F` avec `FF` (coupure de musique, déduit) avant l'effet.
- `8580` (zone) : X ajusté de +8 si `C42C = 0` (regard à droite), -8 sinon ; carry si `(Y-2) - (C5AD-8) < 0x38` et `X' - C5AE < 8`. En cour 2, `C5AD = 144` et `C5AE = 224` (valeurs du journal) : **regard à droite, X dans [216, 224), Y dans [138, 194)** ; regard à gauche, X dans [232, 240).
- `778A` (état 5) : `CALL 55EC` si `C5AC = 5` ; `RET NZ` si `C420 ≠ 0` (pas de marche en l'air) ; `C42E`/`C42F := 0` ; `CALL 765F` (animation de marche, déduit) ; `BC = 0x0080` (regard à droite) ou `0xFF80`, `CALL 6C94` : un demi-pixel par passage de la logique à 30 Hz, soit 1 px toutes les 4 images (mesuré). Puis `77C4` : sortie si `X - (C5AE+8) < 8`, soit **X dans [232, 240)**, et `7800` : `C408 := 1`.
- `55EC` : copie des motifs du montant (source `A0EA`, destination `F900`), puis 8 entrées de SAT dans l'ombre `D600` : Y = `C5AD - 1` (4 premières) ou `C5AD + 15` (4 suivantes), X = `C5AE`, motif `(C & 0x0B) * 4` avec C de 8 à 15 (20, 24, 28, 2C, deux fois), couleurs par `563B` (12, 13, 14, 2 : quatre couches). Ces sprites sont en tête de table : ils passent devant Simon.
- `4387` (état 10) : `C701 &= FE` puis `4347` : `C410`, `C411` +1 (BCD), `D000 += 1`, `D001 := 0`, `C409`, `C408 := 0`, `C000 := 4`.

## 4. Ce qui est prouvé, déduit, ouvert

- **Prouvé (trace)** : déclenchement à x 216 en marchant à droite (Y 176), marche automatique de 1 px par 4 images avec les poses de marche, touches ignorées, montant de la porte en sprites devant Simon (X 224..239, Y 144..175), sortie à x 232, 2 images de cour encore affichées, 36 images d'écran noir, hall en x 16, y 192, jeu 1 image après le retour de l'affichage, effet sonore 0x15 pendant 76 images (1451 à 1527).
- **Prouvé (code)** : bornes de la zone (X et Y), arrêt de la marche en l'air, sens de marche suivant le regard, zone de sortie [232, 240).
- **Déduit** : rôle de `765F` (animation), source graphique `A0EA` (banque non relevée), identité de l'effet 0x15 (« porte du château »).
- **Ouvert** : zone regard à gauche (X 232..239), non atteinte ; saut pendant la marche (non testé) ; contenu musical de l'effet 0x15 (pas d'équivalent MD).

## 5. Portage MD

- `sgdk/src/gameplay.c` (sans SGDK, testé par `tests/test_gameplay.c`) : `VKGame.entrance` (`VK_ENTRANCE_WALK`, `EXIT`, `DARK`), `entrance_step`, `entrance_timer`. Zone de `8580` (regard à droite, X [216, 224), Y [138, 194)) au début de l'image ; événement `VK_EVENT_ENTRANCE`. Marche : 1 px toutes les 4 images au sol, cycle de poses 0, 1, 2, 1 (8 images), arrêt en l'air. Sortie à X ≥ 232 ; 3 images plus tard `VK_EVENT_ENTRANCE_DARK` (Simon passe en hall 0, x 16, y 192) ; 36 images plus tard `VK_EVENT_ENTER`. `vk_game_entrance_locked` bloque les touches. L'ancienne sortie « DROITE à x ≥ 232 » est retirée.
- `sgdk/src/main.c` : le montant (colonnes 28-29, rangées 18-21 du plan A) passe en priorité haute et le sprite 0 de Simon en priorité basse pendant la marche ; sous le montant, des cases noires opaques en priorité haute sur le plan B (inutilisé ailleurs) jouent le rôle de l'opacité des sprites MSX. Écran noir : les 64 couleurs passent au noir, au lieu de couper l'affichage (affichage coupé, le VDP signale le VBlank en permanence et la boucle de jeu ne serait plus cadencée ; mesuré au harnais : 36 tours en 2 images). Le hall recharge les quatre palettes.
- **Non porté / PROVISOIRE** : effet sonore 0x15 (aucun son MD équivalent) ; compteur STAGE du HUD (absent du HUD MD) ; les 4 images du hall sans sprite de Simon (1555-1558) et les 2 images où le montant reste à l'écran sur le hall (défaut d'origine) ; zone regard à gauche ; saut pendant la marche (les touches sont ignorées, non mesuré).
- Non-régression : `tests/md_scenarios/h_castle_entrance.txt` (déclenchement à x 216, x 226 après 40 images sans touche, sprite de Simon sans priorité, écran noir à t 67, hall à t 103, x 16, y 192). Les scénarios qui passent par l'entrée attendent maintenant `front_mode == INTERIOR` au lieu de `game.area == 1` (le hall est atteint en logique pendant l'écran noir).
