# Durée du lancer : dague 6 ticks, hache et croix 9 ticks

Statut : **prouvé** (openMSX, `tools/run_weapon_trace.py`, `weapon.csv` / `weapon_ev.csv`, 3 lancers par arme, phases d'appui différentes, ROM de référence) ; **porté** (`sgdk/src/whip.c`, `whip.h`, `tests/test_whip.c`, scénario `zb_throw_timing`).

## Code Z80 (banque 1, table `71F0`)

Quatre passes, une par valeur de `C422` : `71FA` (état 1 : `5593` charge les graphismes, `C42F` = 12), `7240` (état 2 : `C42F` = 13, `5593`), `7250` (état 3 : `C42F` = 14, `5593`), `7262` (état 4 : `7286`, lancement `72A6`). Aucun compteur de délai pour `C436` >= 2 (celui de `C429` ne sert qu'aux fouets). La durée vient donc du **rythme des passes**, pas d'une constante.

## Trace (image = VBLANK `4028`, appui = événement `attack_7133`)

Séquence `C422/C42F` aux images appui + 1, + 2, ... :

| Arme (`C416`) | Séquence | Lancement `72A6` | `C003` |
| --- | --- | --- | --- |
| dague (2), 3 lancers | 2/12 2/12 3/13 3/13 4/14 4/14, puis 0 | appui + 6 (2464 -> 2470, 3337 -> 3343, 3459 -> 3465, 3589 -> 3595) | +1 toutes les 2 images |
| hache (3), 3 lancers | 1/0, 2/12 x2, 3/13 x3, 4/14 x3, puis 0 | appui + 9 (1938 -> 1947, 2151 -> 2160, 2370 -> 2379) | +1 toutes les 3 images |
| croix (4), 3 lancers | identique à la hache | appui + 9 (2201 -> 2210, 2638 -> 2647, 2851 -> 2860, 3072 -> 3081) | +1 toutes les 3 images |

Les neuf lancers hache et croix sont identiques image par image. Cause : chaque passe décompresse les graphismes de l'arme (`5593`, hache `A4C9`, croix `A272`, plus gros que la dague `A24E`) : la boucle principale déborde d'un VBLANK et la passe dure 3 ticks au lieu de 2 (`C003` le montre). La durée est donc **mesurée, pas déduite** ; elle dépend de la charge, la trace est faite dans le hall (salle 0, ennemis absents). Les mesures de `whip-variants.md` (8 ticks de `C42F` non nul pour `C416` = 3) concordaient, mais ne voyaient ni le tick `C422` = 1 à `C42F` = 0, ni la croix.

## Port

Le minuteur du coup part à 14 à l'appui. Dague : pose 12/13/14 sur les timers 14..9, lancement au timer 8 (6 ticks, inchangé). Hache et croix (`VK_WHIP_LEVEL_HEAVY`) : tick 14 sans pose, pose 12 sur 13-12, 13 sur 11-9, 14 sur 8-6, lancement au timer 5 (`VK_WHIP_HEAVY_END`), soit 9 ticks. Image accroupie : mêmes pas (tracée, section suivante).

Le vol de l'arme part donc 3 images plus tard qu'avant : toute trace de rejeu enregistrée avec la hache ou la croix change (rejeu 12-18 à réenregistrer).

## Lancer accroupi (trace openMSX du 2026-11-14)

Même outil (`tools/run_weapon_trace.py`), plan : BAS tenu 12 images, puis BAS+ESPACE 2 images, hall salle 0, objets 1B/1C/1D posés sous Simon, attente de 30 à 33 images entre lancers (phases de `C003` différentes).

- **Hache et croix accroupies** (4 hache, 2 croix, la croix ne repart pas tant que la précédente vole) : `C420` = 2 et `C42E` = 6 pendant toute la pose, puis séquence `C422/C42F` image par image **identique au lancer debout** : appui + 1 : 1/0 ; + 2, + 3 : 2/12 ; + 4 à + 6 : 3/13 ; + 7 à + 9 : 4/14 ; puis 0. Lancement `72A6` à appui + 9 chaque fois (2526 -> 2535, 2649 -> 2658, 2775 -> 2784, 2900 -> 2909 ; croix 3079 -> 3088, 3328 -> 3337) ; `C003` + 1 toutes les 3 images. Le port (mêmes pas debout et accroupi, colonne accroupie 1/2/3 pour 12/13/14) était donc **déjà correct** : aucune modification de `whip.c`, assertion de la colonne accroupie ajoutée à `tests/test_whip.c`, lancers accroupis ajoutés à `zb_throw_timing`.
- **Dague** : la trace corrige la mesure précédente. La dague **n'a pas une durée fixe** : passes de 2 ou 3 images (le chargement de ses graphismes `A24E` déborde parfois d'un VBLANK), lancement `72A6` de 6 à 9 images après l'appui. Accroupie, 3 séries : 7, 6, 6, 9, 7, 8, 7, 8, 7, 7, 7, 9, 8, 9 (14 lancers) ; 7, 8, 7, 7, 7, 7 (6 lancers) ; 7, 7, 7, 9 (premier plan). Debout, même plan : 7, 6, 7, 7, 6, 8, 8, 7, 7, 7 (10 lancers) mais 9, 9, 9, 9, 9, 9 dans une autre série (après une série accroupie) ; les 6 images des quatre lancers de la première trace sont donc le minimum, pas une constante. Posture sans effet mesurable (accroupie 7,3 en moyenne, debout 7,6 en moyenne). Valeurs par passe inchangées : `C42F` 12, 13, 14 et `C42E` = 6 accroupie. Le port garde 6 : la cause exacte de l'allongement (charge de la boucle principale, phase par rapport au VBLANK) n'est pas identifiée ; hache et croix (toujours 3 images par passe) ne sont pas concernées.
