# Déblocage du contrôle de Simon en openMSX et mesure de la marche

Statut : **le contrôle fonctionne**. Le blocage venait d'une erreur d'interprétation de la commande openMSX `keymatrixdown`, pas de la ROM. Les hypothèses sur `C00B` sont fausses. La vitesse de marche est mesurée.

## Prouvé

### Commande clavier : `keymatrixdown` / `keymatrixup` prennent un masque de bits

- `keymatrixdown 8 N` applique le masque `N`, pas l'index de colonne `N`. Mesure via `debug read keymatrix 8` (openMSX 21.0, C-BIOS_MSX2) :

| argument | ligne 8 lue |
|---|---|
| 0 | 255 (rien) |
| 1 | 254 (bit 0) |
| 2 | 253 (bit 1) |
| 3 | 252 (bits 0 et 1) |
| 4 | 251 (bit 2) |
| 8 | 247 (bit 3) |
| 16 | 239 (bit 4) |
| 32 | 223 (bit 5) |
| 64 | 191 (bit 6) |
| 128 | 127 (bit 7) |
| 255 | 0 (tous) |

- Ligne 8, bits selon la matrice MSX standard : bit 7 = RIGHT, bit 6 = DOWN, bit 5 = UP, bit 4 = LEFT, bit 0 = SPACE. Le comportement observé est cohérent avec ce mapping (voir ci-dessous). Le mapping n'a pas été vérifié par trace de `IN A,(A9)` dans la ROM.

### Explication de l'anomalie des colonnes impaires

- `keymatrixdown 8 7` = masque 7 = bits 0, 1, 2 = SPACE + HOME + INS, et non « RIGHT ». `keymatrixdown 8 1` = SPACE.
- Tous les masques impairs incluent bit 0 (SPACE), donc déclenchent le fouet. Les masques pairs ne l'incluent pas. L'anomalie vient de là.
- Le rapport précédent (« RIGHT = (8,7) met C007 à 0x10 ») était donc faux : c'était SPACE+HOME+INS.

### Contrôle réel de Simon (masques corrects)

Matrice d'essai, `gate/gate.tcl` (chaque touche maintenue 2,5 s, puis 1 s de repos) :

| touche | masque | effet observé |
|---|---|---|
| RIGHT | 128 | `C427` augmente (16 à 166), `C007` = 8 |
| LEFT | 16 | `C427` diminue (158 à 38), `C007` = 4 |
| DOWN | 64 | `C42E`/`C42F` passent à 16/15, `C007` = 2 (accroupi) |
| UP | 32 | `C425` baisse 176 à 159 puis revient (saut), `C007` = 33 |
| SPACE | 1 | fouet (`C42F` 6, 7, 8), comme avant |

### La marche n'exige pas `C00B`

- `C00B` et `C00C` sont à 0 pendant toute la marche, y compris à droite. L'hypothèse « contrôle verrouillé tant que `C00B` bit 0 = 0 » est **fausse**.
- Écritures de `C00B` (watchpoint `write_mem`, `gate/watch.txt`) :
  - `4BB9` écrit `C00B` = 0 et `4BB5` écrit `C00C` = 0, environ 1025 fois en gameplay (de 10,9 s à 11,0 s, puis à 19 s et au-delà). Ce sont des remises à zéro répétées, pas une porte d'entrée.
  - `8105`, `80E0`, `80F6`, `809E`, `8320` écrivent pendant le boot et le titre (`C00B` = 3, `C00C` = 39 à 85). Hors gameplay.
- `C00B` ne bloque donc rien dans ce scénario. Le hook `4BB5`/`4BB9` est la seule écriture en gameplay observée.

### Pas de titre : aucune attente de 60 s nécessaire

- Entrée RIGHT à 25 s (`C000` = 5, partie en cours) : `C427` passe de 16 à 136 en 2 s. Pas besoin d'attendre 60 s.
- Le déclencheur de titre reste le point d'arrêt `0x4194` avec SPACE pulsé à 0,2 s / 0,4 s.

### Vitesse de marche mesurée (MSX, RIGHT maintenu)

Données : `gate/ram-walk.txt` (dump RAM `C000-C5FF` à chaque VBLANK, 120 images, 60,0 s à 62,0 s) et `gate/gate-walk.csv`.

- `C427` (x) : +2 px toutes les 2 images (0, 2, 0, 2, ...). Soit **1 px par VBLANK, 60 px/s**. Sur 119 images, x passe de 16 à 134 (118 px, soit 59,5 px/s).
- Compteur de tick logique : `C003` s'incrémente toutes les 2 images (c9, c9, ca, ca, ...). La logique tourne donc à **30 Hz** et chaque tick déplace de **2 px**. Cela confirme le « 2 px/tick » déduit de `6C2F/6C53`.
- Cadence d'animation : `C42E` (et `C42F`, identique) suit la séquence 0, 1, 2, 1 avec **8 images par pose** : une pose toutes les 133 ms, cycle complet de 32 images (≈ 533 ms). Les 4 valeurs du cycle sont 0, 1, 2, 1 (la dernière 1 est la même pose que la deuxième). Ces valeurs apparaissent dans `C42E` et `C42F` ; `C420`, `C422`, `C423`, `C42B`, `C436` restent à 0 pendant la marche.

## Déduit (non mesuré côté MD)

- Port MD (`sgdk/src/player.c`, `vk_player_tick_controls`) : boucle `n < 1`, **1 px par appel**. Un appel par image (`vk_player_tick` dans la boucle de `sgdk/src/main.c`, suivi de `SYS_doVBlankProcess`), donc 60 px/s, identique au MSX. Historique : le port était à `n < 2` (2 px par appel, 120 px/s) ; corrigé par le commit `726922d` (« Match Simon walk speed and cycle to the MSX2 trace »).
- Animation MD (même commit) : `walk_frames = {0, 1, 2, 1}` indexé par `(clock >> 3) & 3`, soit 8 images par pose et cycle de 32 images, comme le MSX (`C42E`, 0, 1, 2, 1 avec 8 images par pose). Cette correspondance est **déduite du code**, pas mesurée sur le port.
- L'ancienne comparaison de `research/input-mapping-openmsx.md` (« 2 px/tick MSX et 2 px/frame MD coïncident à 60 Hz ») reposait sur une erreur : le MSX fait 2 px par tick de 30 Hz, soit 1 px par image. Le port MD fait lui aussi 1 px par image depuis `726922d`.

## Inconnu

- Routine exacte qui lit la ligne 8 (`IN A,(A9)` après `OUT (AA)`) : non tracée. Le mapping vient de la matrice MSX standard et du comportement observé.
- Comportement de marche au-delà de 2 s, près des murs ou dans d'autres salles : non mesuré.
- Pourquoi `4BB5`/`4BB9` écrivent 0 à `C00B`/`C00C` en boucle : routine non identifiée, sans effet mesuré sur la marche.
- Comportement de la ligne 8 pour les touches sans lien avec la marche (HOME, INS, DEL) : non testé.
- SHA-256 : le fichier `Vampire Killer (Japan, Europe) (En).zip` de la racine a pour SHA-256 `afb01a5d9ebc11c16a09...`, pas `99eabaae...`. Le `game.rom` de 128 Ko utilisé ici a le SHA `99eabaae...`. Le hash annoncé correspond donc au ROM extrait, pas au zip. À vérifier avant de publier cette référence.

## Reproduction

Dossier de travail : `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/gate/`

```
export OPENMSX_SYSTEM_DATA=/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/omsx-share
VK_OUT=/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/gate \
timeout 200 /tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/openmsx/openmsx-21.0/derived/x86_64-linux-opt/bin/openmsx \
  -machine C-BIOS_MSX2 -cart .../scratchpad/gate/game.rom -script .../scratchpad/gate/<script>.tcl -control stdio
```

Scripts (dans `scratchpad/gate/`) :
- `probe_mask.tcl` : vérifie `keymatrixdown` comme masque (`mask.txt`).
- `probe_matrix.tcl` : ligne 8 par bit (`matrix.txt`).
- `gate.tcl` : parcours RIGHT/LEFT/DOWN/UP et SPACE, sortie `gate-run.csv`.
- `gate2.tcl` : marche RIGHT avec adresses d'animation, sortie `gate-walk.csv`.
- `dump.tcl` : dump RAM `C000-C5FF` à chaque VBLANK pendant la marche, sortie `ram-walk.txt`.
- `watch.tcl` : watchpoints `C00B`/`C00C` et test RIGHT à 25 s, sortie `watch.txt`.

## Correction à apporter

- `research/input-mapping-openmsx.md` affirme que « aucune combinaison ne déplace Simon » et que `keymatrixdown 8 7` est RIGHT. Les deux sont faux ; voir ce document.
