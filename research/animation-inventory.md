# Inventaire des animations — MSX2 Vampire Killer, portage Mega Drive

ROM de référence : SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`,
chargée via `tools/probe.py` (`reference_rom`, `load_rom`). Aucune ROM n'est incorporée au dépôt.
Aucun fichier de code du jeu n'a été modifié. Les traces brutes sont dans le dossier de travail
`/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/anim-inventory/`.

## 0. Base de temps

| Élément | Valeur | Statut | Preuve |
|---|---|---|---|
| Point d'échantillonnage `4028` | Gestionnaire VBLANK appelé par `H.TIMI`, une fois par image | prouvé | `research/z80-vdp-io-analysis.md`; trace `attack/out/simon-live.csv` |
| Cadence mesurée (temps émulé openMSX) | 59,92 Hz, Δt médian 16,69 ms | prouvé | 3472 échantillons, `attack/out/simon-live.csv` |
| Unité de cadence du rapport | 1 tick = 1 VBLANK | convention | — |

Les cadences ci-dessous sont données en ticks VBLANK. 1 tick ≈ 16,7 ms.

## 1. Tableau par catégorie

Statuts : **prouvé** = mesuré dans une trace openMSX ou relevé octet par octet dans la ROM;
**déduit** = cohérent avec le code ou la documentation existante, non mesuré ici; **inconnu** = non établi.

### 1.1 Simon

| Animation | Frames (motifs inférieur / supérieur) | Cadence | Source | Statut |
|---|---|---|---|---|
| Attaque (fouet), sélecteur haut `C42F` | 6 (2 ticks), 7 (2 ticks), 8 (8 ticks); bas `C42E` = 0 | **12 ticks ≈ 200 ms** | RAM `C42E`/`C42F`, écrites par `6F20`, `6FB6`, `721D`, `722F`, `7280` (banque 1) | **prouvé** (23 déclenchements, tous identiques) |
| Repos | 0/0 | constant | RAM `C42E`=0, `C42F`=0 pendant la gameplay | **prouvé** |
| Marche droite | 0/0, 1/1, 2/2 (poses MD 0..2) | non mesurée | `research/player-format.md`; vitesse 0x0200 = 2 px/tick par `6C2F` | **déduit** (cadence) / **inconnu** (ordre et durée) |
| Marche gauche | 10/15, 11/16, 12/17 | non mesurée | `research/player-format.md` | **déduit** / **inconnu** |
| Saut droite / gauche | 6/0 et 16/15 (pose seule, pas de séquence) | 19 valeurs de trajectoire dans `6D95` | `research/player-format.md`; `C420` écrit à 2 et 3 par `6C20` et `6BF0` | **déduit** (valeurs de trajectoire) / **inconnu** (durée des images) |
| Accroupi | 5 candidats (`simon_crouch_manifest.json`) | inconnue | `sgdk/src/generated/simon_crouch_manifest.json`, `phase_order_verified: false` | **inconnu** |
| Escalier | MD : cycle `{0,1,2,1}` selon `stair_anim>>3` | inconnue dans la ROM | `sgdk/src/player.c`, lignes 129-144 | **inconnu** |
| Attaque, sélecteur bas `C42E` | toujours 0 pendant le fouet | — | trace 23 déclenchements | **prouvé** |

Le code MD courant (`player.c` : `clock & 4` et `clock & 8`) n'est donc pas issu de la cadence ROM.

### 1.2 Décors animés

| Animation | Frames | Cadence | Source | Statut |
|---|---|---|---|---|
| Flamme gauche de la cour | 2 états (A, B) de 16 × 16, coordonnées VRAM x 64–78, y 144–159 | **2 états tenus 16 ticks chacun, période 32 ticks ≈ 0,53 s** ; transitions partielles d'une trame (écriture sur les lignes 144–148 puis 149–159) | VRAM openMSX (80 trames, 2300–2379, cour au repos) ; blocs source en ROM (fichier 0xDF48 = état A, 0xDEC8 = état B) | **prouvé** (cadence) ; forme des états **extraite de la ROM** (`tools/build_flame_fx.py`) et contrôlée contre la VRAM du run harness et la VRAM openMSX ; palette **candidate** (non comparée à la VDP en direct). Voir `research/courtyard-flames.md` |
| Flamme droite de la cour | idem, x 192–206 | même cadence, en phase avec la gauche | idem | **prouvé** (cadence) |
| Coordonnées écran | Y VRAM − 32 = ordonnée du fond ? | — | `research/room-format.md` (fond à (0,32)) | **déduit**, à vérifier |
| Carte de fond `D100–D3FF` | aucune modification en idle | — | RAM, 454 VBLANK | **prouvé** : pas d'animation par la carte en RAM |
| Zone VRAM y 488–495 (page 1 présumée non affichée) | 16 états distincts, changement environ toutes les 2 trames | — | VRAM openMSX | **inconnu** (rôle non établi ; probablement tampon hors écran) |
| Flammes du code MD actuel | 4 frames de flamme + 3 frames de bouffée, `flame_fx_md.h` | — | générées par `tools/build_flame_fx.py` depuis les blocs ROM (commit `19819d0`, remplace le prototype V15). La bouffée réutilise les états ROM : aucune bouffée n'existe dans la ROM de la cour | **issu de la ROM** (provenance dans `sgdk/src/generated/flame_fx_manifest.json`) ; choix d'intégration pour la bouffée |
| Nuage, 2 chauves-souris, Simon devant la grille (scène 63D3/4E93) | 14 sprites distincts | 320 ticks, une position par tick | `research/intro-format.md`; `sgdk/src/generated/intro_assets.h` (`vk_intro_frames[320][8]`) | **déduit** de la doc existante, **non remesuré** ici |

### 1.3 Compteurs RAM (cadence, sans rôle d'animation établi)

| Adresse | Comportement mesuré en gameplay (idle) | Statut |
|---|---|---|
| `C420` | Décrémenté de 1 par tick pendant le boot (60 → 0 entre les ticks 15 et 75), puis 0 constant en gameplay | **prouvé** ; rôle d'animation **inconnu** |
| `C003` | +1 toutes les 2 ticks (240 changements sur 480 ticks) | **prouvé** ; rôle **inconnu** (candidat horloge 30 Hz) |
| `C005` | Garde de réentrance, bascule à chaque tick | **prouvé** |
| `C025`, `C039`, `C04D` | −1 à chaque tick (479 changements sur 480) | **prouvé** ; rôle **inconnu** (candidats minuteurs d'entités) |
| `C00F` | Pas de −0x18 toutes les 2 ticks | **prouvé** ; rôle **inconnu** |
| `C03A`, `C04E`, `C052`, `C053`, `C097` | Modifications irrégulières | **inconnu** |

### 1.4 Ennemis

Aucune mesure d'animation d'ennemis n'a été faite dans ce travail. Le fichier non suivi
`research/enemies-platforms.md` (autre travail en cours) couvre la boucle d'entités et la scène
de l'écran titre ; il n'a pas été repris ici. **Inconnu** pour les ennemis de stage.

## 2. Entrées clavier (conditions de mesure)

| Constat | Statut |
|---|---|
| `keymatrixdown 8 c` met le bit c de la ligne 8 à zéro (testé pour c = 0 à 7) | **prouvé** |
| Routine `4B9D` : lit `SNSMAT` lignes 8 et 6, et le port joystick (PSG R14) ; appelée si `C002` bit 6 est à 1 (vrai en gameplay : `C002` = 0x40) | **prouvé** (lecture statique) |
| Attaque : `(8,1)` déclenche le fouet (9 essais, plus 14 dans la trace existante) ; `(8,3)`, `(8,5)` et `(8,7)` déclenchent aussi exactement le même fouet ; `(8,0)`, `(8,2)`, `(8,4)`, `(8,6)` ne le font pas | **prouvé**, **non expliqué** |
| `(8,7)` (RIGHT supposée) met `C007` à 0x10 et `C006` à 0x10 au front | **prouvé** |
| `(8,4)` (LEFT supposée) ne modifie ni `C006` ni `C007` pendant le test | **prouvé**, **non expliqué** |
| Simon ne se déplace pas en gameplay (`C000=5`, `C001=0`, `C5AC=0`) malgré `C007=0x10`. Les gardes `CE11` et `C5AC` valent 0 | **prouvé** |
| Correspondance des directions | **inconnu** : non établie |

Remarque : la table de `tools/_tas_tools.tcl` d'openMSX attribue SPACE à `(8,0)`, ce qui contredit
la consigne `(8,1)`. La mesure ci-dessus donne `(8,1)` pour le fouet ; la consigne est conservée,
mais le lien avec le fouet est à reconfirmer, car `(8,3)`, `(8,5)` et `(8,7)` le déclenchent aussi.
Le test `vk_attack_keys` de `sgdk/src/whip.c` doit être revu à partir de ces constats.

## 3. Ordre de priorité pour le portage

1. **Fouet (12 ticks, 6/6/7/7/8×8)** : séquence déjà reproduite dans `whip.c` ; elle est prouvée. À garder ; vérifier la correspondance des touches.
2. **Flammes de la cour (2 états, 16 ticks, deux emplacements)** : cadence prouvée, forme extraite de la ROM (fait, `19819d0`). Reste le contrôle de palette contre la VDP openMSX et la vérification de l'ordre de tuiles colonne par colonne sur émulateur MD.
3. **Marche (poses 0..2 / 3..5, cadence)** : bloquant. Il faut une entrée de déplacement qui produise un mouvement, ou une routine de dispatch à tracer (routine d'écriture de x : `6BD0-6BD6` et alignement `6BFC-6C05`, identifiées statiquement).
4. **Saut** : après la marche (cadence des images à mesurer ; trajectoire déjà connue).
5. **Escalier** : séquence non identifiée.
6. **Accroupi** : 5 candidats, ordre non vérifié.
7. **Ennemis** : travail en cours par ailleurs.
8. **Intro** : déjà issue de la ROM ; à conserver.

## 4. Points restant à vérifier

- Garde qui empêche le déplacement en gameplay (`C5AC=0`, `C007=0x10`) : décoder la dispatch depuis `414D`.
- Cause des colonnes impaires de la ligne 8 qui déclenchent le fouet, et absence de LEFT.
- Page VDP affichée (R#2 = 6 ⇒ page 0 présumée) : confirmer avant de dire que la zone y 488–495 est cachée.
- Correspondance des coordonnées VRAM avec le fond MD (décalage 32 supposé).
- Palette exacte des états de flamme (non vérifiée contre la palette du stage).
- Rôle de `C003`, `C025`, `C039`, `C04D` (minuteurs candidats).

## 5. Fichiers de travail (hors dépôt)

Dossier : `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/anim-inventory/`
- Lanceur : `run_trace.py` ; sondes : `probe_matrix.tcl`, `dir_probe.tcl`, `matrix_bits2.tcl`, `ram_watch.tcl`, `ram_right.tcl`, `gate.tcl`, `map_watch.tcl`, `vram_watch.tcl`, `vdpregs.tcl`.
- Données : `probe1/probe.csv`, `dir1/dirprobe.csv`, `ram1/ram_c0.txt`, `ramr/ram_right.txt`, `gate1/gate.csv`, `map1/map.txt`, `vram1/vram.bin` (80 trames × 128 Ko), `vram1/vram_index.txt`.
- Désassemblage : `dis.py` (z80dis 1.0.6 du venv du scratchpad).

## 6. Vérifications effectuées

- `python3 -m unittest discover -s tests -p 'test_*.py'` : 21 tests, OK. Le test `test_flame_fx` régénère `sgdk/src/generated/flame_fx_md.h` ; le fichier est identique à la version suivie.
- `sh tests/run_native.sh` : PASS.
