# Harnais d'émulation MD automatisé

Outil : `tools/md_harness.py`. Il charge `sgdk/out/rom.bin` dans le core libretro Genesis Plus GX (`/usr/lib/libretro/genesis_plus_gx_libretro.so`, via ctypes, sans fenêtre ni son), rejoue un script d'entrées, lit la RAM 68000 par les symboles et écrit des captures PNG. Le mode d'emploi complet est dans la docstring du script.

- Symboles : `m68k-elf-nm -S` sur `sgdk/out/release/rom.out` (adresse et taille), avec repli sur `sgdk/out/release/symbol.txt`. `--symbols-only` les liste.
- Structures décodées : `game` (VKGame, avec `stage`, `entrance`, `entrance_step`, `entrance_timer`), `player`, `whip`, `enemy_state` (7 ennemis de 18 octets, énergie à l'offset 126, invulnérabilité, recul, compteurs du type 2 ; 144 octets), `test_target`, `courtyard_flame[2]` et `front_mode`. La taille de chaque symbole est vérifiée : si une structure change dans `sgdk/src`, l'outil s'arrête avec un message explicite au lieu de lire de mauvaises valeurs.
- RAM GPGX : les octets sont permutés dans chaque mot (l'octet à l'adresse `a` se lit à l'index `a ^ 1`).
- Boutons (libretro → MD) : B=0 → B, Y=1 → A, START=3, haut, bas, gauche et droite = 4 à 7, A=8 → C.
- Sorties dans `--out` : `dumps.json` et `dumps.csv` (une ligne par `dump`), `trace.csv` (avec `--trace N`), les PNG et `result.json` (les assertions).
- Codes de sortie : 0 = tout passe, 1 = au moins un `expect` échoue, 2 = erreur de script (délai `until` dépassé, par exemple). En cas d'erreur, la capture `error.png` est écrite.

## Scénarios de non-régression

`tests/run_md_scenarios.sh [OUT_DIR]` lance `tests/md_scenarios/*.txt` (sortie par défaut : `scratchpad/md/`). Les huit scénarios prennent quelques secondes au total.

| Scénario | Contenu | Résultat (2026-10-27) |
|---|---|---|
| `a_courtyard_walk` | logo → titre → château → cour ; départ (16,176) face à droite, énergie 32 ; 60 images à droite → x=76 ; demi-tour | 8/8 |
| `b_hall_whip_kill` | DROITE tenu jusqu'au hall (arrivée x=16, y=192 comme sur le MSX2, puis Simon replacé à x=200) ; ennemi type B (type 1 MSX) à x=10 image 1003 ; Simon se tourne, fouet quand l'ennemi est à x-36 ; ennemi tué image 1175 (whip.timer=10), énergie intacte | 8/8 |
| `d_death_game_over` | contact jusqu'à 0 d'énergie : mort (état 6 à +252 images), vies 2 → 1, reprise salle 0 x 16 ; puis vies forcées à 0 : game over et retour au titre | 8/8 |
| `e_room4_bat` | hall, salle 4 par le changement de salle debug ; le type 4 apparaît, X recule de 30 px en 20 images, Y suit la courbe (179 au pas 10, 158 en haut) | 5/5 |
| `f_key_door` | salle 2 : clé prise au contact (x 140..152) ; salle 7 sur le rebord (panthère retirée) : ouverture à x 228, clé consommée ; stage 2 en jeu (salle 0, x 16, y 96), chauve-souris en Y 74, puis salle 1 par la droite depuis l'étage haut (y 64) | 12/12 |
| `g_room2_panther` | salle 2 : panthère créée en 80,80, assise tournée vers Simon (x 194, y 128) ; réveil sous 64 px, course vers Simon ; tuée au premier coup de fouet accroupi, sans contact | 6/6 |
| `h_castle_entrance` | cour 2 : porte à x 216, marche seule 1 px / 4 images (x 226 après 40 images sans touche), sprite de Simon sans priorité derrière le montant, écran noir à t 67, hall à t 103 (x 16, y 192) | 7/7 |
| `c_enemy_contact` | même chemin sans attaquer ; contact image 1192 : énergie 32 → 30, invulnérabilité et recul ; après 30 images, recul fini, pas de second dégât | 7/7 |
| `k_stage2_room2_zombie` (2026-10-28) | stage 2 forcé (`vk_game+20`), salle 2 par le changement de salle debug, Simon en (145, 160) ; zombie à droite en X `E5`, Y SAT `80` puis posé en `81`, `romBox` ; tué par un fouet debout pendant la phase 2 (`whip.timer` 3 à 7), sans contact ; deuxième tir à gauche (X `0A`) ; contact à moins de 10 px : énergie 32 → 30, recul et invulnérabilité | 11/11 |

Le chemin vers le hall n'utilise aucun mode debug : il suffit de tenir DROITE depuis la cour (cour 0 → 1 → 2, porte du château à x 216, puis entrée automatique de 103 images, `research/castle-entrance.md`). Les scénarios attendent `front_mode == MODES['INTERIOR']` : `game.area` passe à 1 dès l'écran noir. Le changement de salle debug du hall (A + gauche/droite) reste disponible avec `hold A+RIGHT 1`.

Observation (c) : avant correction, le recul poussait Simon jusqu'à x=255 sans tester les murs. Il passe maintenant par `vk_game_push` (`gameplay.c`) : limites de marche (x 8 à 248) et sondes de mur ; le scénario vérifie `8 <= x <= 248`.

## Anciens outils

- `tools/check_whip_hitbox.py` : la cour ne teste plus `test_target` mais `courtyard_flame` (même VKTarget). Le script prend maintenant `courtyard_flame` quand il existe. Résultat : 9/9 contrôles OK (avant la correction : 7 échecs).
- `tools/verify_play_mode.py` : le saut est cadencé à 30 Hz (Simon atterrit 29 images après l'étape 6), donc l'attente passe de 25 à 40 images. La cible de FRONT_TEST, à `player+(24,-24)`, est lue en RAM au lieu de la valeur `+48` codée en dur. Une option `--out` est ajoutée (défaut inchangé : `validation/`). Résultat : tout passe.
- `tools/verify_viewer.py` : passe sans modification.
- `tools/verify_intro.py` : **non réparé**. Il demande le module Python `z80` (`pip install z80==1.2.0`), absent du système. Une fois ce module installé, il échoue sur l'ancien enchaînement : START à l'image 0 sans logo, entrée du hall par HAUT, position d'arrivée (16,192). Il faut le réécrire, et le scénario `b` couvre déjà ce parcours.
- `tools/intro_harness.py` et `tools/verify_player_primitives.py` ne sont pas des harnais MD. Ce sont une reconstruction Z80 des scènes MSX et des tests différentiels C ↔ Z80.
- Core utilisé : SHA-256 `ddefb99f…`. Ce n'est pas le core épinglé dans `validation/toolchain.json` (`.tools/gpgx`, absent).
