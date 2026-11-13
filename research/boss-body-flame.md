# Flamme de mort des corps de boss des stages 3, 6 et 9 (objet 1E, `7806` -> `9A3A`)

Code : `sgdk/src/boss_body_flames.c` (dessin), `boss.c` (`vk_boss_hit` pose `flame[k]` à 16 : inchangé), `actors.c` (appel, réservation des entrées SAT).
Rendu réutilisé : cellules `F4` / `F8` des flammes de mort (`research/death-flame.md`, `DEATH_FLAME_TILE_INDEX`, `tools/build_death_flame_md.py`), palette de ligne 8,
exactement comme les flammes des projectiles du boss (`VK_BOSS_SHARED_FLAME_TILE`). Tests : `tests/md_scenarios/zzs_boss_flame.inc` inclus par
`zr_stage3_boss.txt`, `zz_stage6_boss.txt` et `zzd_stage9_boss.txt` (le dessin n'existe que dans la ROM MD : pas de test natif).

## Méthode

Traces openMSX (moteur `harness.tcl` de la session, script `tools/boss_body_flame_trace.tcl`, analyse `tools/boss_body_flame_poses.py` ; plans et
sorties non versionnés) : à chaque image (`debug set_bp 0x4028`) les 7 blocs C800 (type, Y, X, `+B`, `+C`, `+E`, `+7E`, `+7F`, `+20..+27`), la SAT complète (entrées de motif >= 40h
avec la couleur de la première et de la dernière ligne), la palette ; point d'arrêt `7806` ; instantané VRAM à la première flamme. Stage 3 : plan D (porte salle 4 -> 5,
`C418` = 2 puis coups de fouet), stage 6 : plan D de `stage6boss`, stage 9 : plan C2 (salle 7, `C418` = 8, les deux objets 14h à portée).

## Prouvé

- **Création** : `7E17` -> `7806` appelle `9A3A` pour chaque objet non nul (A = 1 dans `CFFF`), puis `9A4A` (`99F6` efface le corps, `9ABE` ne tire rien car `CE00` != 0,
  `5F1D` crée un objet 1E dans le premier bloc libre) à **(X, Y - 18h)** du corps : table `9B33` = 18h pour les types 12h, 13h, 14h (lue sur la ROM). X identique.
  Trace : stage 3 corps (2Bh, B9h) -> flamme (2Bh, A1h) ; stage 6 (DDh, A1h) -> (DDh, 89h) ; stage 9 deux corps (78h, C5h) et (7Ah, C5h) -> (78h, ADh) et (7Ah, ADh), une
  flamme par corps. Octets : `+0` 1E, `+B` 85, `+C` 10h, `+E` 0, `+7E` 0, `+7F` 1, `+1F` 0 (pas de drop : boss).
- **Dessin** (`6445`) : une entrée SAT visible **(X - 8, Y - 16)** en pose 85 (motif `F4`) et **(X - 8, Y - 15)** en pose 86 (`F8`), soit à l'écran Y - 15 / Y - 14 comme les
  flammes de mort ordinaires ; couleur de ligne 8 sur les 16 lignes ; la seconde entrée est cachée (Y 225, couleur 0). Mêmes motifs que `death-flame.md`, vérifiés sur
  l'instantané VRAM (`F800`).
- **Durée et animation** : la flamme est à l'écran **34 images** : F4 x 6, F8 x 8, F4 x 8, F8 x 8, F4 x 4 (identique dans les trois stages, y compris les deux flammes du
  stage 9, qui sont synchrones). Début : 2 images après le `7806` (stages 3 et 6) ou 3 (stage 9) ; fin : bloc effacé 33 images après le `7806`, SAT cachée 2 images
  plus tard. La flamme dure 34 images et non 32 (`death-flame.md`) parce que le script de fin `B6AB` (`sub_7806` appelée à `B6C5`, vue en trace 3 images après la mort) rappelle
  `7806` : elle passe sur le bloc 1E, le tue (`99F6`, `9A3A`) et `5F1D` recrée une flamme 1E dans le même bloc, avec D = 0 (la table `9B33` n'a pas d'entrée pour 1E) : même X, Y, `+C` = 10h
  (trace : `+C` 10, 0F, 0F, **10**, 0F, 0E ... stage 3 images 1366-1369 ; stage 6 images 1898-1901). La séquence `+B` / `+C` est ensuite celle de `death-flame.md`.

## Porté

- `sgdk/src/boss_body_flames.c` (`vk_boss_body_flames`, appelée par `actors.c` juste après `vk_boss_sprites`) : dès que `flame[k]` (posé à 16 par `vk_boss_hit`, inchangé) est vu non nul, la flamme du
  corps k est dessinée **34 VBlanks** (`vtimer`), F4 / F8 aux positions ci-dessus, X et Y - 18h du corps (les coordonnées du corps ne bougent plus après la mort), cellules `F4` / `F8` et palette
  des flammes de projectile (`VK_BOSS_SHARED_FLAME_TILE`). Le décompte est en VBlanks et non en passes : le coup tombe sur une image quelconque alors que le boss ne passe que toutes les deux images
  (35 images au lieu de 34 sur les stages 6 et 9 avec un décompte par passes), et l'image de la mort est lourde (le port perd parfois un VBlank). L'état tient dans `shot_state[1..2]`, inutilisé par ces
  boss (aucun champ ajouté). Première entrée SAT une image après le premier calcul, la VDP en ajoutant une : 2 images après le `7806`, comme la ROM.
- Les entrées SAT tenues (`actors.c`, `flame[k]`, 16 passes) sont inchangées : elles ne suivent pas la relance de `B6AB` (3 images de moins que l'image) ; sans effet sur le dessin.
- Stages 12 et 15 : la flamme du corps (`boss12_flame`, marcheur `alive` = 2, Y - 24, relance à `script` = 0, poses 85/86) était déjà dessinée par `boss12_sprites`/`boss15_sprites`
  et comparée par passe à la ROM (`tests/run_boss12_trace.sh`, `run_boss15_trace.sh` : octets IX+00..1F des flammes et sprites `6445`). Non retracé ici sur openMSX image par image.

## Rejeu 12-18 (`tests/run_playthrough_12_18_md.sh`)

Le rejeu image par image ne survit à aucune modification du binaire qui change les décisions d'inlining LTO ou les cycles du dessin : mesuré sur ab58629 (qui passe : 55 points de contrôle) avec
(a) une fonction morte ajoutée : passe ; (b) 4 octets de remplissage dans `VKBoss` : décrochage stage 13 salle 1 (image 5405) ; (c) le dessin des flammes dans `shared_flames` : `vk_show_player`
perd 3,6 Ko, premier écart image 9987 (stage 14 salle 7, X de Simon 20 au lieu de 14 : une image d'avance, aucun boss), décrochage au point de contrôle stage 15 salle 2 (image 10916) ; (d) le même
appel gardé par `kind < 3` : décrochage stage 13 (image 5405) ; (e) l'appel sans garde, dans `actors.c` (version livrée) : tout passe jusqu'à la salle 9 du stage 15 (image 14187), décrochage
au dernier point de contrôle du combat de boss (image 25383, stage 16 salle 0 attendu, Simon en salle 7). La cause n'est pas la logique du jeu (stages 12-15 sans corps de boss 3/6/9) : la marge CPU des
images est mince et tout changement du code compilé déplace des images perdues. Le rejeu n'a PAS été réenregistré (consigne) ; `vk_boss_body_flames` n'ajoute que ~40 cycles par image dans les salles de boss.

## Test

`zzs_boss_flame.inc` : après le coup mortel (`until ram8('vk_boss+192') != 0 max N B` : `flame[0]` posé par `7806`), position (X - 8, Y - 24 - 15) de F4, puis exactement les
passages F8 / F4 / F8 / F4 aux images 6, 14, 22, 30 de la première apparition, disparition à 34 images, `flame` revenu à 0 ; stage 9 : les deux flammes aux deux positions.
Le comptage en images du scénario détecte aussi une perte de VBlank non compensée.
