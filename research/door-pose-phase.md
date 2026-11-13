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

## Port

Avant : `items.c` (porte) et `gameplay.c` (entrée) tournaient [0, 1, 2, 1] par 8 images depuis le début de la marche, hors phase du compteur (écart de 8 images ou plus, et de 6 images à l'ouverture de la porte de clé).

Maintenant : `main.c` donne `C003` après la passe de l'image (`tick`, +1 à la première image d'une passe, comme `414D` qui précède le code de la porte) à `VKItems.c003` et `VKGame.c003` ; l'ouverture et le déclenchement de l'entrée mémorisent `door_c003` / `entrance_c003`. Porte : pose = [2, 1, 0, 1][((`door_c003` + (timer - 1) / 2) >> 2) & 3] dès timer 89 (+3 regard à gauche), pose d'ouverture gardée avant. Entrée : mêmes formule avec step = ligne - 1, pose 0 pour step < 2.

Tests : `tests/test_items.c` (ouverture à `C003` 117 : 2 jusqu'à la ligne 94, 1 à 95, 0 à 103), `tests/test_gameplay.c` (entrée à 172), `tests/stage_exits.c`, scénarios MD `f_key_door` (porte droite), `zy_g_stage4_key_door` (porte gauche, formule + 3) et `h_castle_entrance` (formule aux images 2 à 63 avec `enemies.tick` relevé à l'ouverture, donc indépendant de l'historique du compteur).

Limites : la porte gauche n'a pas été retracée à nouveau (même `6CA4`, la formule + 3 est testée dans le port) ; le port peut ouvrir à la seconde image d'une passe alors que le MSX ouvre toujours à la première (écart d'une image possible sur l'ensemble de la séquence, déjà présent pour X).
