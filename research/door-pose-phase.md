# Phase des poses de marche aux portes (C003)

Point ouvert de `door-room7.md` : « la phase du cycle de poses reste calée sur le début de la marche (sur MSX elle dépend de `C003` à l'ouverture) ». Tracé le 2026-11-14.

## Code (ROM)

`6CA4` (octets lus dans la ROM : `3A 03 C0 1F 1F 1F 11 01 01 38 09 1F 11 00 00 38 03 11 02 02 2A 2E C4 19 22 2E C4 C9`) : `A = C003`, trois `RRA`, `DE = 0101`, si la retenue (bit 2 de `C003`) est mise on saute à l'addition ; sinon un `RRA` de plus (bit 3), `DE = 0000` si mise, `0202` sinon ; puis `C42E/C42F += E/D`. Pose = [2, 1, 0, 1][(`C003` >> 2) & 3] : le cycle suit le compteur global, 8 images (4 passes) par pose, il ne repart pas au début de la marche. `778A` (marche seule de la porte) et l'entrée du château l'appellent ; à gauche `765F` ajoute 10.

## Trace openMSX

Outil : `tools/vertical_links_trace.tcl` (harnais) avec un relevé par image au crochet `4028` de `C003`, `C005`, `C42E`, `C427`, `C425`, `C5AC`, plus points d'arrêt `7743`, `7752`, `778A`, `6CA4`, écritures de `C42E` (pc). Hall du stage 1, salle 7, `C701` = 1 posé, Simon de X 200 à la porte (X 228), plans à 5 déphasages ; cour 2 (entrée du château) dans le même parcours.

Lignes n = images après l'image de l'ouverture (point d'arrêt `7752`), `C003` lu au `4028` de la ligne n :

| Fait | Mesure |
|---|---|
| Compteur | `C003`(n) = `C003` de la passe d'ouverture + (n - 1) / 2, +1 toutes les 2 images, sans saut (portes, levée comprise) |
| Pose pendant la levée | gardée (pose de l'ouverture) jusqu'à la ligne 88 ; la première `778A` est à l'image 88, sa pose se voit à la ligne 89 |
| Ouverture à `C003` 117, 122, 123, 124, 117 (5 essais) | à partir de la ligne 89, pose = [2, 1, 0, 1][(`C003` >> 2) & 3] sur toutes les lignes, 0 écart ; première pose 2 (`C003` 161) pour 6 images seulement (89..94), puis 8 images par pose ; avec 124 la première pose est 0 (`C003` 168) |
| Entrée du château (`C003` 172 à la porte) | lignes 1 et 2 : pose 0 ; à partir de la ligne 3, même formule sur 63 lignes, 0 écart (1 de 3 à 8, 2 de 9 à 16, 1, 0, 1, 2...) ; X + 1 aux lignes 5, 9, 13... |

## Porte gauche (stage 4 salle 3), 16 ouvertures (2026-11-14)

Même outil (script openMSX hors dépôt : salle forcée `D000 = 4`, `D002 = [1E6A + 4]`, `D001 = 3`, `C000 = 4`, `C701` bit 0 posé, Simon en X 34 / Y 96, GAUCHE tenu après 0 à 15 images d'attente pour varier `C003`), relevé `C003`, `C427`, `C42C`, `C42E`, `C5AC` au crochet `4028` plus points d'arrêt `7743`, `7752`, `778A`, `8580`, `7802`, écritures `C003`, `C427`, `C42E`, `C000`, `D000`. Ouverture à X 26, `C42C` = 1, pose 10..12 (regard à gauche).

| Fait | Mesure |
|---|---|
| Ouvertures | 16, `C003` à `7752` = 94, 183, 16, 106, 196, 31, 122, 214, 50, 143, 236, 74, 168, 7, 102, 198 |
| Compteur | `C003`(n) = `C003` d'ouverture + (n - 1) / 2 sur les 16 (0 écart) |
| Pose | gardée (pose d'ouverture) jusqu'à la ligne 88, puis `C42E` = [2, 1, 0, 1][(`C003` >> 2) & 3] + 10 sur toutes les lignes 89 à 162 des 16 ouvertures, 0 écart (la formule du port, + 3 dans l'indexation du port) |
| X | 26 jusqu'à la ligne 88, 25 à la ligne 89, puis -1 toutes les 4 lignes, 7 à la ligne 161 : 0 écart sur les 16 |
| Fin | `77BA`/`7802` et `C408` := 1 dans l'image f = 160 (celle de l'écriture de X = 7), `C000` := 10 à f = 161, `D000` + 1 à f = 162 |

Les poses et X de la porte gauche suivent donc exactement la formule de la porte droite (poses gauches = + 10 sur MSX).

## Image de l'ouverture dans la passe (2026-11-14)

Une passe = 2 images, `C003` est incrémenté (`W003`, pc 4150) à la première. Sur 16 ouvertures de la porte du stage 4, `7752` est toujours exécuté dans une image où `C003` vient d'être écrit (première image de la passe) ; sur 209 appels de `8580` (test de contact de la porte, déclenché à chaque passe tant que Simon est dans la zone) aucun n'a lieu hors d'une image où `C003` est écrit : `8580` appartient au code de la passe, il ne tourne jamais à la seconde image. Un contact établi à la seconde image n'ouvre donc la porte qu'à la première image de la passe suivante.

Toute la marche suit la même grille : les écritures de `C427` (`6C98`) et `C42E` (`779B`, `7677`, `6CBC`) ont lieu aux images f = 88, 90, 92... (f = nombre d'images après l'image de `7752`, toujours paires, donc toujours des premières images), jamais aux images impaires. Les lignes n du relevé lisent l'état avant l'image f = n (ligne n = après f = n - 1) : la ligne 89 montre l'écriture de f = 88.

## Port

Avant bf17112 : `items.c` (porte) et `gameplay.c` (entrée) tournaient [0, 1, 2, 1] par 8 images depuis le début de la marche, hors phase du compteur.

bf17112 : `main.c` donne `C003` après la passe de l'image (`tick`, +1 à la première image d'une passe, comme `414D`) à `VKItems.c003` et `VKGame.c003` ; l'ouverture et le déclenchement de l'entrée mémorisent `door_c003` / `entrance_c003`.

Cette passe (porte, `items.c`) :
- **Image d'ouverture** : `VKItems.second_frame` (1 quand `Enemies.itemPhase` est 0, c'est-à-dire seconde image de la passe) interdit l'ouverture ; le port ouvrait avant à n'importe quelle image, donc à la seconde dans la moitié des cas (la porte s'ouvrait alors une image plus tôt que sur MSX, avec une pose et un X décalés d'une image sur toute la marche).
- **Grille de la marche** : l'état du port après l'image f suit désormais l'état MSX après l'image f (l'ouverture est dans la même image des deux côtés). L'ancien port comptait la ligne n du relevé comme l'état après f = n, soit une image de retard : X bougeait aux secondes images de la passe, jamais sur MSX. Maintenant 778A est appelé dès f = 88 (`DOOR_WALK_FIRST`) : pose = [2, 1, 0, 1][((`door_c003` + f / 2) >> 2) & 3] (+3 regard à gauche) ; à gauche X - 1 à f = 88 puis toutes les 4 images, à droite X + 1 à f = 90 puis toutes les 4 images.
- **Sortie** : `D000` + 1 reste à f = 162 (gauche) / 164 (droite), c'est-à-dire 2 images après X final (nouveau champ `end_wait`) : le calendrier de sortie n'a pas changé, seul X/pose ont avancé d'une image. `VKItems` passe de 22 à 24 octets (`second_frame`, `end_wait` dans le trou d'alignement après `door_c003`) : `EXPECTED_SIZES` du harnais mis à jour.

Entrée du château (`gameplay.c`) : inchangée. Même question de grille (ligne n du relevé contre état après l'image) non retracée : voir limites.

Tests : `tests/test_items.c` (contact à la seconde image sans ouverture, porte droite à `C003` 117 : pose 2 jusqu'à f = 93, 1 à f = 94, 0 à f = 102, X 229 à f = 90, sortie à f = 168 ; porte gauche image par image), `tests/test_gameplay.c` (entrée à 172), `tests/stage_exits.c`, scénarios MD `f_key_door` (porte droite), `zy_g_stage4_key_door` (porte gauche, formule + 3), `zy_g_door_parity_a` et `_b` (porte gauche, contact à la première puis à la seconde image de la passe, `C003` différents ; chaque image de la marche contrôle X, pose et que X ne bouge qu'aux premières images) et `h_castle_entrance`. Avec la garde de la seconde image désactivée, `_b` échoue (ouverture à l'image 1 au lieu de 2 après le contact).

## Rejeu 12-18

`tests/run_playthrough_12_18_md.sh` (non réenregistré) échoue à la frame 1821, stage 16 salle 5 (porte gauche, X 7, Y 112) : checkpoint « stage 17 salle 0 » manqué d'une image. Cause : l'enregistrement a été fait avec l'ancien port, qui ouvrait cette porte à une seconde image de la passe ; le port ouvre maintenant à la première image suivante, comme le MSX, donc toute la sortie est une image plus tard. Contre-épreuve : avec la seule garde `second_frame` neutralisée (marche décalée et sortie `end_wait` gardées), le rejeu passe en entier (55 checkpoints, 3847 images, 0 image perdue) : le décalage de X/pose et `end_wait` ne changent pas le calendrier de sortie. Le rejeu est à réenregistrer.

## Limites

- Les 16 ouvertures de la trace sont toutes de la porte gauche du stage 4 ; la porte droite (stage 1, 5 + 1 ouvertures) a été tracée avant mais pas relue avec la grille f ci-dessus (même code `778A`/`6CA4`, mêmes lignes) ; stage 5 salle 5 non relancé dans cette passe.
- La grille de l'entrée du château (poses et X, ligne n contre état après l'image) n'a pas été revérifiée comme la porte : la même éventuelle image de retard reste à tracer.
- `8580` exclu des secondes images : seule l'ouverture de la porte en dépend dans le port ; le ramassage de la clé (`7FE2`) et les autres tests de contact n'ont pas été retracés pour la parité.
