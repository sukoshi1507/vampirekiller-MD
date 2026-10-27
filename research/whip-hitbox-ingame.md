# Boîte du fouet et cible d'entraînement : vérification en jeu (MD)

Objectif : vérifier, dans la ROM MD exécutée par Genesis Plus GX, que `vk_whip_bounds` (`sgdk/src/whip.c`) couvre ce que le joueur voit, et que la cible d'entraînement n'est touchée que dans cette boîte. Aucun fichier de `sgdk/src/` ni de `validation/` n'a été modifié. Aucun correctif appliqué.

## Méthode reproductible

- Script : `tools/check_whip_hitbox.py` (nouveau, hors code du jeu). Lancement :
  `python3 tools/check_whip_hitbox.py /usr/lib/libretro/genesis_plus_gx_libretro.so "Vampire Killer (Japan, Europe) (En).zip" sgdk/out/release/rom.bin sgdk/out/release/symbol.txt <dossier_sortie>`
- Core : `/usr/lib/libretro/genesis_plus_gx_libretro.so`, SHA-256 `ddefb99f1d8e8140871de9b69731e590eac5f40c3e8966be1478bc2b97e68fc0`. Ce n'est pas le core épinglé dans `validation/toolchain.json` (`.tools/gpgx/...`, absent du dépôt, empreinte `0f4b6a29...`). Résultats à reproduire avec le core épinglé.
- ROM MD : `sgdk/out/release/rom.bin` (daté 05:39, postérieur à `whip.c` 05:14 et `main.c` 05:24).
- Adresses RAM (`sgdk/out/release/symbol.txt`, offsets dans la RAM 64 Ko) : `player` 0x0180, `whip` 0x018C, `test_target` 0x016E. Structures lues : VKPlayer (x, y, facing, crouched), VKWhip (timer, facing, x, y), VKTarget (x, y en int 32 bits, width, height, active, hit_latched). La taille 16x16 de la cible est vérifiée à l'exécution.
- Chemin de jeu : titre (START), château (START), cour (`FRONT_COURTYARD`). Dans cette cour, B est le fouet et A+B réinitialise la cible à `player.x+48`, `y-24`.
- Pas de modification de la RAM pour le parcours réel, uniquement pour les balayages (cible déplacée par écriture RAM avant chaque attaque).
- Sorties (hors dépôt) : `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/whip-ingame/` : `whip-hitbox-ingame.json` (toutes les mesures), `shot-{right,left}-timer09.png` (capture brute), `box-{right,left}-timer09.png` (zoom x4 avec boîte mesurée en rouge), `mask-{right,left}-timer09.png` (pixels modifiés par l'attaque en magenta).

## Résultats

Tous les contrôles automatiques passent (`all_passed: true`).

### (a) Cible touchée dans la boîte seulement (prouvé)

Hit (`vk_target_tick`) : chevauchement entre la boîte du fouet et la cible `[t.x, t.x+16) x [t.y, t.y+16)`. Mesuré à la RAM, dx = t.x - x, dy = t.y - y.

| Balayage | Résultat mesuré | Attendu par le code |
|---|---|---|
| Droite, dx (dy = -24) | touché pour dx de -7 à 40 ; raté à -8 et 41 | -8 < dx <= 40 |
| Droite, dy (dx = 20) | touché pour dy de -46 à -16 ; raté à -47 et -15 | -47 < dy <= -16 |
| Gauche, dx (dy = -24) | touché pour dx de -55 à -8 ; raté à -56 et -7 | -56 < dx <= -8 |
| Fenêtre : cible dans la boîte au poke à timer t | touché si t >= 4 ; hit au timer min(t-1, 10) | timer 3..10 |
| Parcours réel, Simon avance de 0, 7, 1 px (cible de spawn) | dx = 48, 41, 40 : raté, raté, touché (timer 10) | dx = 48 et 41 : raté ; 40 : touché |

Conséquence : la cible posée à `player.x+48` ne peut jamais être touchée en station debout (8 px d'écart entre le bord de la boîte, x+40, et le bord gauche de la cible, x+48). Cela ne dépend pas du timer.

### (b) Lanière visible par rapport à la boîte (prouvé pour les pixels mesurés)

Mesure : pixels de la capture différents de l'image de référence (Simon au repos, même position, cible et flammes masquées). Les pixels du corps d'attaque sont exclus (x+8 ou plus à droite, x-9 ou moins à gauche). Coordonnées relatives à Simon (x, y) = position `whip.x/y`.

| Face | Timers où la lanière est visible | Lanière (relatif à Simon) | Boîte mesurée | Pixels lanière hors boîte |
|---|---|---|---|---|
| Droite | 9 à 3 (aucun pixel aux timers 10 à 14) | x+8..x+32, y-22..y-15 (bbox : x 32..56, y 154..161 pour x=24, y=176) | x+8..x+39, y-31..y-17 | 2 lignes (y-16, y-15) |
| Gauche | 9 à 3 | x-32..x-9, y-22..y-15 (bbox : x 31..54, y 154..161 pour x=63, y=176) | x-40..x-9, y-31..y-17 | 2 lignes (y-16, y-15) |

- Aucun pixel de lanière au-delà de x+40 (droite) ; la lanière s'arrête à x+32, soit 8 px avant le bord de la boîte. La miroir gauche est cohérent à 1 px près (x-32..x-9 contre x-32..x-8 en miroir).
- Le corps d'attaque (sprites 5/6) est absent de cette mesure ; il déborde à gauche de la boîte, ce qui est attendu.

### Écarts trouvés (prouvés par les mesures ci-dessus)

1. **Portée horizontale** : la boîte va jusqu'à x+40 (dx <= 40 touche), la lanière visible s'arrête à x+32. Sur 8 px, la boîte touche ce qui n'est pas dessiné. Symétrique à gauche (x-40 contre x-32).
2. **Hauteur** : la lanière occupe y-22..y-15 (8 lignes), la boîte couvre y-31..y-17 (15 lignes). Les lignes y-16 et y-15 sont dessinées mais hors boîte : la lanière peut être visible sans toucher. La partie haute (y-31..y-23) de la boîte ne contient aucune lanière.
3. **Fenêtre temporelle** : la cible est touchée dès le timer 10 (première image active), alors que la lanière n'apparaît qu'au timer 9. Une image d'écart. À confirmer : la détection par différence peut manquer des pixels de couleur proche du fond.
4. **Cible de spawn** : à `x+48`, jamais touchée en debout (voir (a)). Ce n'est pas un bug de boîte, mais la cible d'entraînement n'est pas atteignable à sa position initiale.

## Propositions de correctif (non appliquées)

Coordonnées inclusives, conformes au test `vk_target_tick` :

```c
/* whip.c, vk_whip_bounds, debout : boîte mesurée sur la lanière de la ROM (timers 9..3). */
*left   = w->facing ? (int)w->x - 32 : (int)w->x + 8;
*right  = w->facing ? (int)w->x - 8  : (int)w->x + 32;
*top    = (int)w->y - 22;
*bottom = (int)w->y - 15;
```

- Fenêtre : `vk_whip_active` à 3..9 au lieu de 3..10, pour que la cible ne soit touchée qu'avec la lanière visible (à confirmer par une capture du timer 10 sans seuil).
- Cible d'entraînement : la poser à `player.x+24` (ou plus près) au lieu de `+48`, sinon elle n'est jamais touchée debout (`main.c`, `reset_training_target`).
- Accroupi : non mesuré, laisser tel quel.

## Limites

- Une seule attaque par cas (cour, Simon au sol), 3 timers de lanière identiques (9..3) : la forme n'a pas bougé entre les images de cette phase.
- La détection par différence ne voit pas les pixels de lanière de couleur identique au fond : les bornes de la lanière sont des minorants.
- Le core utilisé n'est pas le core épinglé (voir Méthode).
- La cible n'est pas visible à `x+48` pendant les mesures de lanière (masquée), donc le recouvrement visuel cible/lanière n'est pas mesuré.

## Statut

- **Prouvé** : boîte effective du code (balayages), fenêtre timer effective (3..10 avec début au timer 10), position de la lanière visible (frames 9..3), absence de lanière au-delà de x+32 (droite) / x-32 (gauche), spawn non atteignable.
- **Déduit** : l'écart de 8 px en portée tient à la détection par différence ; l'écart d'une image en temps vient de la même limite.
- **Inconnu** : accroupi, lanière à d'autres frames d'animation (phases 0 à 2 non mesurées, car la lanière n'est pas visible à 10..14), couleurs de lanière identiques au fond.
