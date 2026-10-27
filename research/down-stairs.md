# Descente par le haut (BAS) dans les escaliers : entrée, descente, arrivée

Statut : **entrée par le haut prouvée (code + trace) pour la sonde de tuile 3 ; sonde de tuile 4 déduite du code (non testable en salle 7) ; arrêt en cours de volée (contrôle à 8 pas) prouvé ; cause exacte du « blocage » signalé par l'utilisateur non reproduite telle quelle.**

ROM de référence : `Vampire Killer (Japan, Europe) (En).zip`, SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2` (vérifié). Adresses CPU Z80, banque 1 en `6000-7FFF` (même hypothèse que `research/vertical-links.md`, cohérente avec les PC tracés).

Corrections à `research/stairs.md` §2.1 : la routine `7CB3` teste la tuile **3** (et non 4) à la rangée des pieds, en `(x, y)` puis `(x-8, y)` (`CP 3` en `7CC8` et `7CD2`). Seule `7C8B` teste la tuile 4, en `(x, y)` puis `(x+8, y)` (`CP 4` en `7CA0` et `7CA6`-`7CAC`).

## 1. Entrée par le haut (BAS = bit 1 de `C007`)

### 1.1 Chaîne de code (prouvé par désassemblage)

| Adresse | Rôle | Statut |
|---|---|---|
| `6B7D` | `A := C007` ; `RRA` ; `JR C 6BDA` (HAUT, bit 0) ; `RRA` ; `JP C 6C10` (BAS, bit 1) | lu (désassemblé, `z80dis`) |
| `6C10` | `CALL 7C8B` (sonde tuile 4) ; si `Z` : `BC := 2`, `JR Z 6BEE` | lu |
| `6C18` | sinon `CALL 7CB3` (sonde tuile 3) ; si `Z` : `BC := 0x102`, `JR Z 6BEE` | lu |
| `6C20`-`6C2C` | sinon : `C420 := 2`, `C42E := 6`, `JP 765F` (mode 2, non étudié) | lu, observé (x200, x183) |
| `6BEE` | `C420 := 3` ; `C435 := C` (= 2) ; `C421 := B` ; `C427` aligné : `+8` si `B=1` (`RRA`), puis `AND F8` ; `C424 := 0` ; `C426 := 0` ; **`C42B` non remis à zéro** | lu + prouvé (`writes.csv`, PC `6BF0`, `6BF8`) |

Sondes (désassemblage, `E = C425` (Y), `D = C427` (X), `7D2F` lit la tuile dans `D100`, index `((Y-16)>>3)*32 + (X>>3)`) :
- `7C8B` : `CP 6` sur `CE00` (stage 6 : chemin différent, non étudié). Tuile 4 en `(X, Y)` : `RET Z` (X inchangé). Sinon tuile 4 en `(X+8, Y)` : `X := X+8`, `XOR A` (Z). Sinon `NZ`.
- `7CB3` : même schéma avec la tuile 3 et `X-8`.
- Solidité (`7C5E`) : `(tuile-1) < limite` ; stage 1 : limite 4, donc tuiles 1 à 4 solides.

### 1.2 Conditions (tableau)

| Condition | Valeur | Effet | Confiance |
|---|---|---|---|
| Touche BAS (`C007` bit 1) | 1 | entrée à tester | prouvé (trace : `hold D`, `D` = BAS) |
| HAUT non tenu (`C007` bit 0) | 0 | sinon `6BDA` (montée) | prouvé code ; non testé |
| `CE00` | ≠ 6 (stage 1 : 0) | sinon chemin stage 6 | prouvé code |
| Tuile 4 au pied `(X,Y)` ou `(X+8,Y)` | oui | `C421 := 0` (descente vers la droite, `6E14`) | **déduit** (code) ; pas de tuile 4 dans la salle 7 du stage 1 |
| Tuile 3 au pied `(X,Y)` ou `(X-8,Y)` | oui | `C421 := 1` (descente vers la gauche, `6E4B`) | **prouvé** (trace) |
| Ni 3 ni 4 | non | `C420 := 2` (mode 2), pas de volée | **prouvé** (trace x200, x183) |
| Ligne des pieds | `Y` dans `[96, 103]` vue par `7D2F` (ligne 10) | | prouvé code ; `Y` mesuré = 96 |

### 1.3 Position d'entrée requise (salle 7, stage 1)

Table `vk_stage1_collision[7]` (`sgdk/src/generated/stage1_collision.c`), lignes 10 (y 96-103) : colonne 21 (x 168-175) = tuile **3**, colonne 22 (x 176-183) = tuile 1, colonne 23 = tuile 2. Le sommet de la volée est la tuile 12 en `(160,104)`.

| Position de départ (Simon posé, `C420 = 0`, `Y = 96`) | Résultat mesuré | Run |
|---|---|---|
| X = 168 | entrée `6BF0` à `C427 = 169`, aligné à 176, `C421 = 1`, `C420 = 3` | `x168` |
| X = 169 (posé) | idem, entrée à 176 | `x168` (2613) |
| X = 170 (Y forcé, aligné à 96) | entrée, `C421 = 1` | `cy100x170` |
| X = 172 | entrée (`6BF0` à 172), descente complète | `c180` |
| X = 176 | entrée (`6BF0` à 168 avant `+8`), `C421 = 1` | `c176b`, `x176c` |
| X = 180 | entrée (`6BF0` à 172), descente complète | `c180` |
| X = 184 | **pas d'entrée** : `6C22` `C420 := 2` | `x183` (X=183 a été ramené à 184 par le jeu) |
| X = 200 / 201 | **pas d'entrée** : `6C22` `C420 := 2` | `x200` |
| X = 160 | non valide : pas de sol (chute) | `c160` |

Fenêtre mesurée : **X dans [168, 180] (entrée OK), 184 et au-delà KO**. Fenêtre prédite par le code : `[168, 183]` (tuile 3 en `X` ou `X-8`, avec `Y = 96`). X = 160 (pas de sol sous les pieds) : le test est invalide ; la fenêtre basse reste prédite à 168 par le code.

Y : le jeu aligne `Y` sur 96 à la pose (voir §3) ; une entrée à `Y` différent n'a pas été mesurée.

## 2. Descente (prouvée, C421 = 1)

- `6DE0` : avec `C421 != 0`, BAS (bit 1) appelle `6E4B` (`DE = 0x100D`) : `X -= 1`, `Y += 1` par pas. `C42B` est incrémenté à chaque pas (`6E6D`), `C435` sert d'animation.
- Mesuré (`x176` : entrée à l'image 2614, arrivée à 2680) : `(176,96)` → `(175,97)` à 2617, `(170,102)` à 2627, `(169,103)` à 2629, `(145,127)` à 2679, `(144,128)` à 2680. Soit **1 px en X et 1 px en Y toutes les 2 images**, 32 px dans chaque sens, sans décalage latéral.
- Touche relâchée : la descente s'arrête (non re-mesuré ici ; prouvé en `research/stairs.md` §2.2 pour la montée).

### 2.1 Contrôle de sol à 8 pas (prouvé, cause d'un retour en haut)

- `6E6D` : quand `C42B >= 8`, `6E44` appelle `7B88` (sonde de sol `(X+5, Y)` si `Y < 208`, tuiles solides 1 à 4). Si solide : `JP 6EF5`.
- `6EF5` : `C42B := 0` (`6F00`), puis `6D3B` : `C420 := 0`, `C421 := 0`, `Y := Y AND F8`. Simon est reposé en haut de la volée.
- Mesuré (`c176b`) : au 8e pas, Simon est en `(169,103)`, sonde `(174,103)` tombe sur la tuile 3 (solide) ; sortie à `(169,96)` à l'image 2478. Avec BAS toujours tenu, l'entrée est relancée à 2480 (`6BF0`, `C421 = 1`) et la descente va à `(144,128)` à 2545.
- Mesuré (`x176c`) : au 8e pas, `(168,104)`, sonde `(173,104)` tombe sur la tuile 6 (non solide) ; pas de sortie, descente complète.

Donc, avec BAS tenu, la volée peut se « rembobiner » (retour à Y = 96 après 8 pas) puis repartir. La phase du compteur `C42B` (non remis à zéro à l'entrée) décide de la ligne où le contrôle tombe.

### 2.2 Arrivée au pied

- Arrêt mesuré : `(144, 128)`, `C420 = 0`, `C421 = 0`, `Y` multiple de 8 (run `x176` : écriture `6D43` à l'image 2680 ; run `c176b` : 2545).
- Le pied `(144,128)` est sur le plancher du bas (ligne 14 de la table : colonne 18 = tuile 1, solide).
- BAS encore tenu après l'arrivée : `6C10` ne trouve ni tuile 3 ni tuile 4 à `(144,128)` ; `C420 := 2` (mode 2, `6C22`), puis `STUCK` après 45 images. Pas de blocage dur mesuré, mais le mode 2 n'est pas étudié.

## 3. Positions et ordre de la trace (état de départ)

Mise en place commune (`scratchpad/down-stairs/plans/prefix.txt`, reprise du plan `scratchpad/vlinks/plan-e.txt`) : titre, salle 0 puis salle 3, `poke C427=52`, `poke C425=64`, HAUT (`climb3to7`), arrivée salle 7 à `(49,224)` puis `(56,217)` en mode 3 (tenu).

Piège constaté : en mode 3 tenu, `poke C420 := 0` déclenche la routine de sortie de l'état précédent (`6F76`, `7EC3 C420 := 5`, `6FB1 C420 := 3`) qui ramène Simon à `Y = 96`. L'essai « Y=100 » a donc été forcé à 96. Les essais `x160`, `x180`, `y100x170` (sans `C42B` remis à zéro, après `wait 150`) ont été perturbés (sortie de salle, `D001 = 0`) et ne sont pas retenus. Les runs retenus utilisent `c*` (après `wait 120`, `C42B`/`C420`/`C421`/`C435` remis à 0) ou `x*` (sans remise à zéro) selon le tableau du §1.3.

## 4. Inconnues

1. **Appelant de `6B7D`** : pas trouvé par `CALL`/`JP` direct. Une référence `7D 6B` existe en fichier `0x1613C` (banque 11, probablement une table de pointeurs). Non remonté : quel état (`C000`) appelle la routine. Le test de BAS en salle 7 était fait en marche (`C420 = 0`).
2. **Tuile 4 (`7C8B`, `C421 = 0`, descente vers la droite par `6E14`, `DE = 0x0103`)** : non testé, aucune tuile 4 dans la salle 7 du stage 1.
3. **HAUT tenu avec BAS** : `6B81` donne la priorité à HAUT (déduit, non testé).
4. **Mode 2** (`C420 = 2`, `C42E := 6`, `JP 765F`) : non étudié (chute ? saut ?).
5. **Stage 6** (`CE00 = 6`) : chemin différent, non testé.
6. **Phase du compteur `C42B`** : non remis à zéro à l'entrée (`6BEE` ne l'écrit pas). Le contrôle de sol peut rendre Simon en haut de la volée (§2.1). Reste à prouver si la phase dépend seulement des états précédents.
7. **Y d'entrée** : seul `Y = 96` mesuré (le jeu aligne Y à la pose).
8. **Cas du « blocage » de l'utilisateur** (salle 7 et salle 5) : non reproduit tel quel. Les essais en salle 7 montrent une descente complète depuis `X` dans [168, 180], Y = 96, BAS tenu. Hors de cette fenêtre (X = 184, 200), BAS n'entre pas et Simon passe en mode 2. En salle 5 (arrivée HAUT depuis la salle 1, `C420 = 3` tenu), l'entrée n'est pas en cause : c'est `C420 = 3` déjà actif ; voir `research/vertical-links.md` §6.

## 5. Fichiers

- Scripts (versionnés) : `tools/down_stairs_trace_extra.tcl` (watchpoints `C420`, `C421`, `C42B` avec PC ; à ajouter après `tools/vertical_links_trace.tcl`) et `tools/run_down_stairs_trace.py` (lance les deux).
- Commande : `python3 -I tools/run_down_stairs_trace.py ROM.rom OUTDIR PLAN [TIMEOUT]`. Plans : `scratchpad/down-stairs/plans/` (non versionné) : `prefix.txt` + `c176.txt`, `x176.txt`, `x168.txt`, `x200.txt`, `x183.txt`.
- Journaux : `scratchpad/down-stairs/runs/` (non versionné) : `c176b` (mesure de `C42B`, sortie à 8 pas à `(169,103)`), `x176c` (même entrée, sans sortie, `C42B` à 25 résiduel), `x168`, `x200`, `x183`, `c180`, `cy100x170`, `c160`.
- Désassemblage : `tools/disasm_rom_window.py` (dépend de `z80dis`, présent dans `scratchpad/venv`, `PYTHONPATH` sur son `site-packages`).
