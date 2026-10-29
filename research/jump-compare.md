# Saut de Simon : trajectoire MSX2 (openMSX) et simulation MD

> **Mise à jour** : la correction proposée (cadence à 30 Hz) est appliquée dans `sgdk/src/player.c`
> (`jump_frames`, commit `0e7d5a7`). Sur le saut immobile, les 46 valeurs de Y du MD égalent celles du MSX2.
> Le saut en mouvement horizontal et la chute (mode 4) ne sont pas comparés.

Statut : **écart prouvé** entre la cadence du saut MSX2 et celle du portage MD. Le tracé Y du saut (19 valeurs) est identique ; la durée est double côté MSX2. Cause dans le code MD : `vk_player_tick_controls` (`sgdk/src/player.c`) avance le saut une fois par VBLANK, alors que le MSX2 l'avance une fois par tick logique de 30 Hz (un tick toutes les 2 images). Aucun fichier de `sgdk/src`, `tests/`, `validation/` ou `.claude/` n'a été modifié.

ROM : `Vampire Killer (Japan, Europe) (En).zip`, SHA-256 interne `99eabaae...` vérifié (`scratchpad/jump/`). openMSX 21.0, machine `C-BIOS_MSX2`.

## 0. Méthode

- **Trace MSX** : `tools/jump_trace.tcl`. Titre passé par SPACE (point d'arrêt `4194`, comme `tools/player_health_trace.tcl`). Une ligne par VBLANK (point d'arrêt `4028`) : `C003`, `C006`, `C007`, `C420`, `C421`, `C423`, `C425` (Y), `C427` (X), `C428`, `C42E`, `C431`, `C439`, `CFF0`, `C422`. Écritures avec PC pour `C420`, `C421`, `C423`, `C425`, `C427`, `C428`, `C006`, `C003`. Dump CPU 64 Ko à 26,0 s pour le désassemblage (`scratchpad/health/dis.py`, désassembleur Z80 `z80dis`).
- Scénarios (appui à 25,0 s, le personnage est posé à X=16, Y=176) :
  - `still_tap` : UP maintenu 0,05 s ; `still_hold` : UP maintenu 2,0 s ;
  - `right_jump` : DROITE maintenue, UP à 25,5 s (relâché à 25,55 s) ; `right_held` : DROITE et UP maintenus ;
  - `turn_jump` : DROITE jusqu'à 25,8 s, puis GAUCHE à 26,2 s, UP à 26,6 s.
- **Simulation MD** : `tools/jump_sim.c`, appelle `vk_player_tick_map` une fois par image, sur un sol plat (lignes 20 à 23 de la carte solides, pieds à Y=176 comme au MSX). Compilée depuis `sgdk/src/player.c` (version HEAD et version de l'arbre de travail : sorties identiques sur le saut, car la modification en cours ajoute seulement `vk_stair_resume`). Sol plat et pas de mur : la trajectoire verticale ne dépend que du sol.
- Comparaison : `tools/jump_compare.py`. Images repérées par `k` = images depuis l'appui (échantillon `1257` pour MSX, image 0 pour MD).

Les sorties brutes sont dans `scratchpad/jump/` (non versionnées) : `run-*`, `v2-*` (avec `C003`), `sim/`.

## 1. Séquence Y(t) MSX2 (prouvé, trace `v2-still_tap`)

| Élément | Valeur | Preuve |
|---|---|---|
| Appui UP (front) | image 1257, t = 25,031 s, `C007` = 33, `C006` = 33 | trace images 1257-1258 |
| Front `C006` effacé | image 1259 (`C006` = 0), par `PC 4BB9` à chaque tick | `writes.csv` (`4BB9 C006 0` toutes les 33 ms) |
| Départ | `C420` := 1 à PC `6B9D`, t = 25,0228 s ; `C421` := 0 à `6BA1` | `writes.csv` |
| Compteur logique | `C003` += 1 à PC `4150` toutes les 33,3 ms (images 1257, 1259, 1261...) | `writes.csv` (`4150 C003`) |
| Pas de saut | `C428` += 1 à PC `6D4D` ; `C425` := valeur à PC `6D5D` | `writes.csv` (19 écritures `6D5D`) |
| Premier pas | image 1261 (k = 4) | trace |
| Sommet | Y = 144 (176 - 32 px), images 1277-1282 (k = 20 à 25) | trace |
| Atterrissage | Y = 176 à PC `6D5D` t = 25,6894 s ; `6D41` (`AND F8`), `6D43` (`C420` := 0), `6D46` (`C421` := 0), `6D49` (`C428` := 0) | `writes.csv` |
| Fin | `C420` = 0 à l'image 1297 (k = 40) | trace |

Nombre d'images : 40 (appui -> fin de `C420`), dont 38 images de pas (19 pas à 2 images). Hauteur : 32 px. Retour au sol : Y = 176, exact.

Table `C425` (Y) : valeurs écrites à `6D5D`, dans l'ordre :
`170, 164, 159, 154, 154, 150, 147, 145, 144, 144, 144, 145, 147, 150, 154, 159, 164, 170, 176`
soit les 19 valeurs de `6D95` (`-6,-6,-5,-5,0,-4,-3,-2,-1,0,0,1,2,3,4,5,5,6,6`) ajoutées à 176 (prouvé : les 19 écritures coïncident avec la table de `sgdk/src/generated/player_assets.c`).

`C428` (index du saut) : 1 à 18 sur les pas, remis à 0 au 19e pas, qui est aussi l'atterrissage. Le code clampe l'index à 19 (`6D1F`, `CP 13`), donc le delta reste +6 si le sol n'est pas trouvé (déduit du code, non tracé).

## 2. Séquences côte à côte

### 2.1 Saut sur place (Y, images depuis l'appui)

Colonne « MSX » : `v2-still_tap` (identique à `v2-still_hold`, voir §4). Colonne « MD actuel » : code de `sgdk/src/player.c` tel quel. Colonne « MD 30 Hz » : variante de test, §5.

| k | MSX Y | MD Y actuel | MD Y 30 Hz (test) | MSX - MD actuel |
|---:|---:|---:|---:|---:|
| 0 | 176 | 176 | 176 | 0 |
| 1 | 176 | 170 | 176 | 6 |
| 2 | 176 | 164 | 170 | 12 |
| 3 | 176 | 159 | 170 | 17 |
| 4 | 170 | 154 | 164 | 16 |
| 5 | 170 | 154 | 164 | 16 |
| 6 | 164 | 150 | 159 | 14 |
| 7 | 164 | 147 | 159 | 17 |
| 8 | 159 | 145 | 154 | 14 |
| 9 | 159 | 144 | 154 | 15 |
| 10 | 154 | 144 | 154 | 10 |
| 11 | 154 | 144 | 154 | 10 |
| 12 | 154 | 145 | 150 | 9 |
| 13 | 154 | 147 | 150 | 7 |
| 14 | 150 | 150 | 147 | 0 |
| 15 | 150 | 154 | 147 | -4 |
| 16 | 147 | 159 | 145 | -12 |
| 17 | 147 | 164 | 145 | -17 |
| 18 | 145 | 170 | 144 | -25 |
| 19 | 145 | 176 | 144 | -31 |
| 20 | 144 | 176 | 144 | -32 |
| 21 | 144 | 176 | 144 | -32 |
| 22 | 144 | 176 | 144 | -32 |
| 23 | 144 | 176 | 144 | -32 |
| 24 | 144 | 176 | 145 | -32 |
| 25 | 144 | 176 | 145 | -32 |
| 26 | 145 | 176 | 147 | -31 |
| 27 | 145 | 176 | 147 | -31 |
| 28 | 147 | 176 | 150 | -29 |
| 29 | 147 | 176 | 150 | -29 |
| 30 | 150 | 176 | 154 | -26 |
| 31 | 150 | 176 | 154 | -26 |
| 32 | 154 | 176 | 159 | -22 |
| 33 | 154 | 176 | 159 | -22 |
| 34 | 159 | 176 | 164 | -17 |
| 35 | 159 | 176 | 164 | -17 |
| 36 | 164 | 176 | 170 | -12 |
| 37 | 164 | 176 | 170 | -12 |
| 38 | 170 | 176 | 176 | -6 |
| 39 | 170 | 176 | 176 | -6 |
| 40 | 176 | 176 | 176 | 0 |
| 41-44 | 176 | 176 | 176 | 0 |

Lecture : le MD actuel termine son saut à k = 19, le MSX à k = 40. La forme est la même (mêmes 19 valeurs de delta), la cadence est double côté MSX. La colonne « MD 30 Hz » suit le MSX avec un décalage exact de 2 images : MSX(k) = MD30Hz(k-2) pour tout k entre 2 et 41 (vérifié sur les 40 images).

### 2.2 Saut en marche à droite (X et Y)

Colonne MSX : `run-right_jump` (appui DROITE à 25,0 s, UP à 25,5 s). MD : `sim/right.csv`, appui à l'image 32 (X = 48 à l'appui, comme le MSX à 25,5 s).

| k | MSX X | MD X actuel | MSX Y | MD Y actuel | MD Y 30 Hz |
|---:|---:|---:|---:|---:|---:|
| 0 | 48 | 49 | 176 | 176 | 176 |
| 2 | 48 | 51 | 176 | 164 | 170 |
| 4 | 50 | 53 | 170 | 154 | 164 |
| 6 | 52 | 55 | 164 | 150 | 159 |
| 8 | 54 | 57 | 159 | 145 | 154 |
| 10 | 56 | 59 | 154 | 144 | 154 |
| 12 | 58 | 61 | 154 | 145 | 150 |
| 14 | 60 | 63 | 150 | 150 | 147 |
| 16 | 62 | 65 | 147 | 159 | 145 |
| 18 | 64 | 67 | 145 | 170 | 144 |
| 20 | 66 | 69 | 144 | 176 | 144 |
| 22 | 68 | 71 | 144 | 176 | 144 |
| 24 | 70 | 73 | 144 | 176 | 145 |
| 26 | 72 | 75 | 145 | 176 | 147 |
| 40 | 86 | - | 176 | - | - |

X : le MSX avance de 2 px tous les 2 images pendant le saut, comme en marche : 1 px par image, la même vitesse horizontale que le MD (décalage constant de 1 px dû à l'alignement). Après l'atterrissage (k = 40), X continue de +1 px/image sans pause.

Autres observations : `right_held` (UP maintenu jusqu'à 27,0 s) donne exactement les mêmes Y et X que `right_jump` (k = 0 à 40). Le saut de `turn_jump` (GAUCHE en l'air, départ à X = 38) suit le même tracé Y (176 à 144 à 176) ; X recule de 1 px/image jusqu'à X = 14, où il s'arrête alors que GAUCHE est encore maintenu (cause non analysée : probable sonde de mur ou bord de salle).

## 3. Réponses

### (1) Séquence MSX
Voir §1. Démarrage à l'image 1257 (25,031 s), premier pas à 1261, sommet Y = 144 (32 px au-dessus de 176), retour à Y = 176 à l'image 1297 : **40 images** entre l'appui et la fin du saut.

### (2) Séquence MD simulée
Voir §2.1 (colonne « MD actuel ») : premier pas à k = 1, sommet Y = 144 aux images 9 à 11, retour à Y = 176 à k = 19. **20 images** entre l'appui et l'atterrissage, soit la moitié de la durée MSX.

### (3) Différence image par image et cause

Écart principal (prouvé) : **le MD avance la table de saut à chaque image (60 Hz) ; le MSX2 l'avance une fois toutes les deux images (tick logique 30 Hz)**. Le tracé est le même, la durée est divisée par deux.

Cause dans le code MD (lecture de `sgdk/src/player.c`, `vk_player_tick_controls`) : la branche `else if (p->mode == 1)` exécute `vertical = vk_jump_deltas[next_phase - 1]` à chaque appel. Le gameplay appelle `vk_game_tick_controls` (`sgdk/src/gameplay.c`, ligne 62) depuis la boucle de `sgdk/src/main.c` (ligne 572), qui se termine par `SYS_doVBlankProcess()` (ligne 670) : un appel par image. Rien ne limite la cadence du saut à un tick sur deux. Preuve par test : en ajoutant `&& (p->clock & 1)` aux branches `mode == 1` et `mode == 4` d'une copie de travail de `player.c` (`scratchpad/jump/sim/player_gate.c`), la trajectoire devient celle du MSX à un décalage de 2 images près (tableau §2.1, colonne « 30 Hz »). Pour un test complet, il reste à mesurer la chute (voir §6).

Écarts secondaires (lus dans le code MSX, non traités par le MD) :

| Point | MSX2 | MD | Statut |
|---|---|---|---|
| Déclenchement | front de UP (`C006` bit 5, `6B90`), touche UP | front de BUTTON_C (`input.c`), touche C | prouvé (MSX) ; le choix de touche est une décision de portage |
| Blocage au départ | `6B97` appelle `7B78` (tuile à X, Y-44) ; si solide, `RET C` : pas de saut | porté (`vk_jump_blocked`, 2026-10-29) | lu dans le code (sens de la retenue déduit de `7C5E`) ; non tracé |
| Plafond en l'air | aucun : `6CE7` ajoute la table à `C425` sans sonde (trace K4 du stage 2, salle 5 : 176 → 144 sous le rebord y 112, `door-room7.md` §18) | le plafond inventé à `Y - 30` est retiré (2026-10-29) | prouvé (trace) |
| Décalage de départ | pas au tick suivant : `C420` posé au tick 177, premier pas au tick 179 (4 images après l'appui) | premier pas à la première image (k = 1) | prouvé (trace) ; cause précise inconnue |
| Atterrissage | sonde de sol seulement si `C428 >= 9` (`6D13`, `CP 9`) ; sinon le saut continue | atterrissage testé seulement en descente | lu dans le code ; équivalent sur sol plat |
| Table de saut | `C431` bit 4 = 0 : table `6D95` (19 valeurs) ; bit 4 = 1 : table `6D94` (21 valeurs) | les deux tables (`vk_jump_deltas`, `vk_jump_deltas_high`, `vk_player_tick_jump`) | prouvé pour le cas testé (`C431` = 0) ; la seconde table est lue dans le code, non tracée |
| Horizontal en l'air | 2 px par tick, dans la même routine que le pas vertical (`6D61`/`6D67` appellent la marche puis `6CE7`) | 1 px par image | prouvé (trace) ; même vitesse moyenne |

### (4) Dépendance à la touche tenue
**Prouvé : le saut MSX ne dépend pas de la durée de l'appui.** `still_tap` (UP relâché après 3 images) et `still_hold` (UP maintenu 2 s) donnent les mêmes Y (k = 0 à 40, vérifié). Un UP maintenu n'en relance pas un deuxième après l'atterrissage (`still_hold`, images 1297 à 1377 : `C006` = 0, `C420` = 0). Le départ est déclenché par le **front** de UP (`C006` bit 5 vaut 33 pendant 2 images, puis `4BB9` le remet à 0 à chaque tick). Relâcher UP en l'air n'interrompt pas le saut. La direction (DROITE ou GAUCHE) n'influe que sur X.
Côté MD, `jump_pressed` est aussi un front (`input.c`, `pressed & BUTTON_C`), donc la même logique. Non testé : appui UP en l'air (`C420` = 1).

## 4. Preuves et adresses (résumé)

| Adresse / PC | Rôle | Statut |
|---|---|---|
| `C420` = 1 à `6B9D` | départ du saut (test `C006` bit 5 à `6B90`) | prouvé (trace, `writes.csv`) |
| `C421` à `6BA1` (0), `6CE5` (valeur de direction : 1 à l'arrêt, 2 ou 3 en mouvement) | direction du saut, `B` calculé par `6CD0` | prouvé (valeurs) ; sens exact : déduit |
| `C006` bit 5 (`20`) | front de UP, effacé par `4BB9` à chaque tick | prouvé (trace) |
| `C003` += 1 à `4150` | tick logique toutes les 33,3 ms (2 images) | prouvé (trace, `writes.csv`) |
| `C428` += 1 à `6D4D` | index du saut, 1 à 19 | prouvé (trace) |
| `C425` := ... à `6D5D` | Y du saut, 19 écritures | prouvé (trace) |
| table `6D95`, 19 entrées | deltas `-6..+6` | prouvé (valeurs identiques à la trace) |
| `6D00`-`6D0A` : `C42E` := 6 | pose du saut (6 à droite) | prouvé (trace : `C42E` = 6 de 1261 à 1297) |
| `6D41`-`6D49` | atterrissage : `C425 &= F8`, `C420`, `C421`, `C428` := 0 | prouvé (trace, PC) |
| `7B78`, `7B88` (sondes) | plafond au départ / sol à l'atterrissage | lu dans le code |
| `6CE7` : `C431` bit 4 | choix de table (19 ou 21 entrées) | lu dans le code ; `C431` = 0 en trace |
| `6D4D` : clamp de l'index à 19 | delta +6 au-delà | lu dans le code |

Fichiers : `scratchpad/jump/run-still_tap/`, `v2-still_tap/`, `v2-still_hold/`, `run-right_jump/`, `run-right_held/`, `run-turn_jump/` (`frames.csv`, `writes.csv`, `ram-c400.bin`, `mem-cpu.bin`) ; `scratchpad/jump/sim/` (`sim`, `sim_gate`, `still.csv`, `gate_still.csv`, `right.csv`, `gate_right.csv`).

## 5. Correction proposée (non appliquée)

Cadencer la trajectoire de saut sur un tick de 30 Hz, comme le MSX2, sans toucher aux tables ni aux sondes :

- Dans `vk_player_tick_controls`, ne faire avancer `p->phase` et `vertical` (branche `mode == 1`) qu'une image sur deux. Le test utilise `(p->clock & 1)` ; le bon choix est une phase de tick explicite (compteur 30 Hz séparé) pour ne pas dépendre de la parité de `clock` au moment de l'appui. Le test prouve la cadence, pas la phase : la variante donne le premier pas à k = 2 et le MSX à k = 4 (décalage de 2 images à régler pour une égalité stricte).
- Appliquer la même cadence à la chute (`mode == 4`) seulement après avoir mesuré la chute MSX (non tracée ici).
- Optionnel, pour la fidélité : aligner le déplacement horizontal sur le même tick (2 px tous les 2 images) ; sans effet sur la vitesse moyenne.
- Vérifier avant de généraliser : le blocage de départ par plafond (`7B78`) et la règle `C428 >= 9` pour l'atterrissage sont des écarts réels, à traiter à part.

## 6. Inconnu

- Cadence et trajectoire de la **chute** au MSX2 (table `6F81`, 2,4,6,6) : non tracées. Le portage `mode == 4` suit le même schéma que le saut en MD.
- Cause de la latence de 2 ticks entre `C420` posé au tick 177 et le premier pas au tick 179 (ordre d'appel de la routine d'état) : non identifiée.
- Table à 21 entrées (`6D94`, `C431` bit 4) : non tracée. **Lu dans le code et porté** (2026-10-29) : `6CE7` prend `BC = 6D94`, `D = 15h` ; valeurs `-6,-6,-6,-5,-5,0,-4,-3,-2,-1,0,0,1,2,3,4,5,5,6,6,6` (somme nulle), soit un sommet à 38 px au lieu de 32 et un retour au sol de départ au pas 21. Le test de `6D1C` (`C428 - 2 >= 13h`) ne dépend pas de la table : avec `6D94`, un saut qui retombe à sa hauteur de départ arrive au pas 21, donc `CFF0 = 1` et l'effet 07 joue (le saut normal arrive au pas 19, sans 07). Porté : `gameplay.c` lève `VK_EVENT_LAND` pour un atterrissage en mode 1 au pas >= 21 (`VK_JUMP_THUD_STEP`). Non porté : `6D6D` décale aussi X de `(C428 - 13h) * 8 / 256` px quand `C420` vaut 2 ou 3 (ce cas n'est pas identifié ; en saut `C420 = 1` et `6D73` sort tout de suite).
- Appui UP en l'air (`C420` = 1) : non testé.
- Arrêt de la marche à X = 14 en `turn_jump` : cause non identifiée.
- Blocage au départ par plafond (`7B78`) : lu dans le code, porté, non tracé. Le plafond en l'air du port (`Y - 30`) n'existait pas sur MSX : retiré (trace K4 du stage 2).
- Le sol plat de la simulation (Y = 176) ne reproduit pas la salle réelle du portage (sol à 192 dans `vk_player_room`) : la trajectoire verticale est indépendante du sol, mais pas les atterrissages sur une marche.

## 7. Reproduction

```
# ROM : scratchpad/jump/game.rom (copie de l'archive, SHA-256 99eabaae... vérifié)
cd /home/sukoshi/Documents/githubsuko/vampirekiller-MD/scratchpad/jump
VK_OUT=$PWD/v2-still_tap VK_SCENARIO=still_tap SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  /usr/bin/openmsx -control stdio -machine C-BIOS_MSX2 -cart $PWD/game.rom \
  -script /home/sukoshi/Documents/githubsuko/vampirekiller-MD/tools/jump_trace.tcl < /dev/null
cd /home/sukoshi/Documents/githubsuko/vampirekiller-MD
gcc -std=gnu99 -O0 -I sgdk/src -o scratchpad/jump/sim/sim tools/jump_sim.c sgdk/src/player.c sgdk/src/generated/player_assets.c
scratchpad/jump/sim/sim still 45 0 > scratchpad/jump/sim/still.csv
python3 tools/jump_compare.py scratchpad/jump/v2-still_tap scratchpad/jump/sim/still.csv 1257
```

Scénarios de `tools/jump_trace.tcl` : `still_tap`, `still_hold`, `right_jump`, `right_held`, `turn_jump` (variable `VK_SCENARIO`). Sorties dans `VK_OUT`.
