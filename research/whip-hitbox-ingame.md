# Boîte du fouet et cible d'entraînement : vérification en jeu (MD)

Objectif : vérifier, dans la ROM MD exécutée par Genesis Plus GX, que `vk_whip_bounds` (`sgdk/src/whip.c`) couvre ce que le joueur voit, et que la cible d'entraînement n'est touchée que dans cette boîte. Aucun fichier de `validation/` n'a été modifié.

## État actuel (mise à jour 2026-10-27)

- Boîte debout du code, bornes inclusives : face droite x+8..x+32, face gauche x-32..x-8, y-22..y-15 (`sgdk/src/whip.c`). Elle couvre exactement la lanière mesurée en jeu (voir (b)).
- Cible de la cour : posée au reset (A+B) en FLAME_A, (64,144), 16x16 (`sgdk/src/main.c`, `reset_training_target`). Elle est donc atteignable debout.
- Extinction après coup : la flamme A s'éteint après un coup, jusqu'au reset (commit `35851ac`).
- Fenêtre d'activation du fouet : timer 3..10 (inchangée, voir « Décisions à prendre »).
- Accroupi : boîte non mesurée (inchangée).

## Méthode reproductible

- Script : `tools/check_whip_hitbox.py`, hors code du jeu. Lancement :
  `python3 tools/check_whip_hitbox.py /usr/lib/libretro/genesis_plus_gx_libretro.so "Vampire Killer (Japan, Europe) (En).zip" sgdk/out/release/rom.bin sgdk/out/release/symbol.txt <dossier_sortie>`
- Core : `/usr/lib/libretro/genesis_plus_gx_libretro.so`, SHA-256 `ddefb99f1d8e8140871de9b69731e590eac5f40c3e8966be1478bc2b97e68fc0`. Ce n'est pas le core épinglé dans `validation/toolchain.json` (`.tools/gpgx/...`, absent du dépôt, empreinte `0f4b6a29...`). Résultats à reproduire avec le core épinglé.
- ROM MD : `sgdk/out/release/rom.bin`, reconstruite avant la mesure (SGDK, sans avertissement).
- Adresses RAM (`sgdk/out/release/symbol.txt`, offsets dans la RAM 64 Ko) : `player`, `whip`, `test_target` (voir `symbol.txt`, résolues par le script). Structures lues : VKPlayer (x, y, facing, crouched), VKWhip (timer, facing, x, y), VKTarget (x, y en int 32 bits, width, height, active, hit_latched). La taille 16x16 de la cible est vérifiée à l'exécution.
- Chemin de jeu : titre (START), château (START), cour (`FRONT_COURTYARD`). Dans cette cour, B est le fouet et A+B réinitialise la cible en FLAME_A (64,144). Le script vérifie cette position, puis pose la cible par écriture RAM à une position explicite avant chaque attaque (parcours et balayages).
- Sorties (hors dépôt) : `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/whip-ingame/` : `whip-hitbox-ingame.json` (toutes les mesures), `shot-{right,left}-timer09.png` (capture brute), `box-{right,left}-timer09.png` (zoom x4 avec boîte en rouge), `mask-{right,left}-timer09.png` (pixels modifiés par l'attaque en magenta).

## Résultats

Tous les contrôles automatiques passent (`all_passed: true`, 9 contrôles).

### (a) Cible touchée dans la boîte seulement (prouvé)

Hit (`vk_target_tick`) : chevauchement entre la boîte du fouet et la cible `[t.x, t.x+16) x [t.y, t.y+16)`, bornes de la boîte inclusives. Mesuré à la RAM, dx = t.x - x, dy = t.y - y.

| Balayage | Résultat mesuré | Attendu par le code |
|---|---|---|
| Droite, dx (dy = -24) | touché pour dx de -7 à 32 ; raté à -8 et 33 | -8 < dx <= 32 |
| Droite, dy (dx = 20) | touché pour dy de -37 à -15 ; raté à -38 et -14 | -38 < dy <= -15 |
| Gauche, dx (dy = -24) | touché pour dx de -47 à -8 ; raté à -48 et -7 | -48 < dx <= -8 |
| Fenêtre : cible dans la boîte au poke à timer t | touché si t >= 4 ; hit au timer min(t-1, 10) | timer 3..10 |
| Parcours réel, Simon avance de 0, 7, 1 px (cible posée par RAM à dx=40) | dx = 40, 33, 32 : raté, raté, touché (timer 10) | -8 < dx <= 32 |

Conséquence : la cible de la cour (FLAME_A, 64,144) est touchable debout. Le spawn à `player.x+48` ne l'était pas (bord de la boîte à x+32, cible à x+48).

### (b) Lanière visible par rapport à la boîte (prouvé pour les pixels mesurés)

Mesure : pixels de la capture différents de l'image de référence (Simon au repos, même position, cible et flammes masquées). Les pixels du corps d'attaque sont exclus (x+8 ou plus à droite, x-9 ou moins à gauche). Coordonnées relatives à Simon (x, y) = position `whip.x/y`.

| Face | Timers où la lanière est visible | Lanière (relatif à Simon) | Boîte du code | Pixels lanière hors boîte |
|---|---|---|---|---|
| Droite | 9 à 3 (aucun pixel de lanière aux timers 10 à 14) | x+8..x+32, y-22..y-15 (bbox : x 32..56, y 154..161 pour x=24, y=176) | x+8..x+32, y-22..y-15 | 0 |
| Gauche | 9 à 3 | x-32..x-9, y-22..y-15 (bbox : x 31..54, y 154..161 pour x=63, y=176) | x-32..x-8, y-22..y-15 | 0 |

- La lanière tient dans la boîte, bord à bord à droite (x+32). À gauche, la lanière s'arrête à x-9 et la boîte va à x-8 : écart de 1 px, accepté.
- Les pixels « hors boîte » de l'image (environ 150 à 350 px selon le timer) sont le corps d'attaque, à gauche de x+8 (droite) ou à droite de x-8 (gauche). Le corps n'inflige pas de dégât : seule la lanière compte pour `vk_target_tick`.

### Écarts (état précédent, corrigés)

1. **Portée horizontale** : la boîte allait à x+40 ; la lanière s'arrête à x+32. Corrigé : boîte x+8..x+32.
2. **Hauteur** : la boîte allait de y-31 à y-17, la lanière occupe y-22..y-15. Corrigé : boîte y-22..y-15.
3. **Cible de la cour** : posée à x+48 au reset, jamais touchée debout. Corrigé : FLAME_A (64,144) dans `main.c`.

### Écart restant

4. **Fenêtre temporelle** : la boîte est active de timer 10 à 3 (`vk_whip_active`), la lanière n'est visible que de timer 9 à 3. Une image d'écart, à l'avantage de la cible (touchée au timer 10 sans lanière visible). Non modifié : décision de l'utilisateur. La détection par différence peut aussi manquer des pixels de couleur proche du fond.

## Décisions à prendre

- Fenêtre `vk_whip_active` : garder 3..10 (actuel) ou passer à 3..9 pour coller à la lanière visible.
- Accroupi : boîte non mesurée, reste à l'estimation précédente dans `vk_whip_bounds`.

## Limites

- Une seule attaque par cas (cour, Simon au sol), 3 timers de lanière identiques (9..3) : la forme n'a pas bougé entre les images de cette phase.
- La détection par différence ne voit pas les pixels de lanière de couleur identique au fond : les bornes de la lanière sont des minorants.
- Le core utilisé n'est pas le core épinglé (voir Méthode).
- Le recouvrement visuel cible/lanière n'est pas mesuré (la cible est masquée pendant les mesures de lanière).

## Statut

- **Prouvé** : boîte effective du code (balayages aux bornes inclusives), fenêtre timer effective (3..10, début au timer 10), lanière visible aux timers 9..3 et contenue dans la boîte debout, cible de la cour atteignable.
- **Déduit** : l'avance d'une image de la cible sur la lanière vient de la fenêtre 3..10 et de la détection par différence.
- **Inconnu** : accroupi, lanière à d'autres frames d'animation (phases 0 à 2 non mesurées, car la lanière n'est pas visible à 10..14), couleurs de lanière identiques au fond.
