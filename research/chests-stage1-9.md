# Coffres des stages 1 à 9 et décor du stage 9 : trace openMSX contre le port

Code : `sgdk/src/candles.c` (`step_drop`, `vk_candles_give`, `pick`), `candle_view.c` (icône de clé du HUD), `tools/gen_chest_scenario.py` (génère
`tests/md_scenarios/zzz_chests_stage1_9.txt`). Tests : `tests/test_candles.c`, `tests/test_gameplay.c` (recul du coffre 0C), scénarios `zzz_chests_stage1_9`,
`p_gold_key_chest`. Outils de trace (non versionnés) : `scratchpad/chest/` (plans, hook de journal, analyse).

## Méthode

Chaque coffre fixe (objet 0x19 de `vk_fixed_object_defs`, décodé de la ROM par `build_candles_md.py` : 31 coffres, `D000` = 1 à 9) est ouvert dans openMSX :
`D000`, `D002` (table `5E6A`), `D001` posés, `C000` = 4 (rechargement de la salle), `C700` = 1 (une clé dorée), `C701` = 2, Simon posé en (X + 8, Y + 16)
du coffre, énergie remise à 32 toutes les 9 images. Journal par image (hook `4028`) : `C403-C44F`, `C700-C70F`, `C500-C5FF` (objets) et chaque appel
du son (`509F`, registre A). L'effet est la différence de RAM après le ramassage. Côté port : `tools/md_harness.py`, god mode, mêmes positions, lecture de
`vk_candles`, `vk_enemies`, `vk_hud_score`.

## Prouvé (trace)

- **Séquence de l'ouverture**, identique pour tous les coffres lisibles : `C700` 1 -> 0 et son 11 à t ; l'objet devient son contenu (type 05, orbe, 8 px plus haut) ;
  son compteur `+6` monte de 0 à 20h **une fois par passe (2 images)** : type 02 à t + 64 (63-64 ; 67-69 quand Simon a été déplacé par un ennemi) ;
  type 03 (8 px plus bas, dessiné dans le décor) à + 4 après le type 02, type 04 à + 6, type 84 (ramassable) à + 8 ; ramassage à t + 73 ou + 74 (son 12, 0F ou 10 de l'effet).
- **Case d'objet du HUD** (`8F23`) : `C5E5` = FF, `C5E6` = 20h au ramassage puis - 1 par passe (64 images), pour tous les contenus sauf 01, 17 et plus (sauf 1E) :
  ni 1A ni 1C ne passent par la case. Même comptage que `box_step` du port.
- **Effets** (colonne MSX du tableau) : `C701` bits 7, 6, 5, 4, 3 pour 0F, 0A, 04, 03, 1E ; `C431` bits 6, 3, 4 pour 0F, 0C, 0D ; `C702` bits 7, 6 pour 11, 10 ; `C416` = 1 ou 3 et
  `C440` = 96h pour 1A et 1C ; `C434` = 96h pour 09 ; `C43E` = 18h et son 1B pour 05 ; score + 5000 (`C406` 00 -> 50) et son 10 pour 13 ; coeurs + 5 et son 0F pour 02.
- **Contenus 12 et 1B** (blocs 0x1F du stage 8 : salle 1 (64, 112), salle 7 (192, 128)) : tracés en posant l'octet `+13` d'un coffre fixe (`C50D`, stage 8 salle 3) : 12 donne
  `C431` 00 -> 02, `C700` 0 -> 3, `C701` bit 2 (trois clés, icône 12, code `8E79`), son 0F ; 1B donne `C416` 0 -> 2 et `C440` = 96h, son 12.
- **Recul du coffre 0C** (`C431` bit 3) : même coup en stage 9 salle 4 (Simon en (16, 112), salle 3 à gauche), `C431` = 0 puis 8. 0 : X 16, 14, 12, 10, 8, salle 3 en F6, atterrissage en
  X 212. 8 : X 16, 13.80, 11, 8.80, salle 3 en F6, atterrissage en X 201. Le port donne les mêmes valeurs (`test_gameplay.c`).
- **Bloc 0x1F contenant un coffre** (stage 6 salle 2, bloc (128, 128), contenu 09) : le fouet casse le bloc (sons 05, 0C, 0E), le coffre apparaît, s'ouvre avec la clé (son 11), le 09
  donne `C434` et le son 12 : même mécanique que les coffres fixes.

## Écarts trouvés et corrigés

1. **Durée d'affichage de l'orbe du coffre : 30 images au port contre 64 sur MSX.** `step_drop` comptait 32 images au lieu de 32 passes. Corrigé : attente de 64 images, 4 images
   avant le type 03, ramassage possible à t + 9 (MSX : + 73 après l'ouverture). Mesure du port : 73 images entre l'ouverture et le ramassage pour les 31 coffres. `test_candles.c` et
   `p_gold_key_chest` mis à jour (leur « 32 images » était faux).
2. **Contenu 0F : `C431` bit 6 manquant** (le port ne tenait que `C701` bit 7 et `C70F` = 3). Ajouté ; rien d'autre dans le port ne lit ce bit.
3. **Contenu 12 non porté** (`8E79`). Porté : trois clés, `C431` bit 1, icône 12 dans le HUD (tuile de la clé rechargée), son 0F, le drapeau tombe avec la dernière clé (`8C1D`),
   remis à zéro à la mort. Seule source : le bloc (64, 112) de la salle 1 du stage 8.
4. Contenu 0C du coffre du stage 8 (`stage8-enemies.md` : « effet non porté ») : **l'effet l'était déjà** (`C431` bit 3 = pas de recul de 2,5 px, `gameplay.c`) ; il est maintenant
   prouvé contre une trace. Le défaut signalé n'existait pas.

## Tableau stage / salle / coffre / contenu / conformité (D000 = 1 à 9)

Conforme : même contenu (la RAM MSX le donne par l'effet et, quand l'objet de type 05 a été échantillonné, par son octet `+4`), même effet, même son, délai 73 images après correction.
« non échantillonné » : l'objet de type 05 n'a pas été vu dans les 8 emplacements journalisés (le contenu est alors établi par l'effet seul).

| Stage | Salle | Coffre (X, Y) | Contenu MSX (octet +4 du type 05) | Contenu MD | Effet MSX (trace) | Effet MD (port) | Conforme |
|---|---|---|---|---|---|---|---|
| 1 | 2 | (176, 176) | non échantillonné | 02 | C417 +5 (BCD), son 0F | coeurs +5, son 0F | oui |
| 1 | 3 | (48, 112) | 0D | 0D | C431 bit 4, son 12 | c431 bit 4 | oui |
| 1 | 4 | (32, 48) | 0C | 0C | C431 bit 3, son 12 | c431 bit 3 | oui |
| 1 | 4 | (32, 112) | non échantillonné | 03 | C701 bit 4, C441 = 10h, son 12 | c701 bit 4, armure 10h | oui |
| 2 | 1 | (16, 48) | 11 | 11 | C702 bit 7, son 12 | c702 bit 7 | oui |
| 2 | 2 | (96, 80) | 0F | 0F | C431 bit 6, C701 bit 7, C70F = 3, son 12 | c431 bit 6 (corrige), c701 bit 7, c70f = 3 | oui |
| 3 | 0 | (64, 112) | non échantillonné | 04 | C701 bit 5, C441 = 10h, son 12 | c701 bit 5, armure 10h | oui |
| 3 | 2 | (112, 112) | 0A | 0A | C701 bit 6, son 12 | c701 bit 6 | oui |
| 3 | 2 | (192, 176) | 09 | 09 | C434 = 96h, son 12 | killTimer 96h | oui |
| 3 | 4 | (48, 80) | 05 | 05 | C43E = 18h (eclair), objets C800 effaces, son 1B | eclair 18h, ennemis effaces, son 1B | oui |
| 3 | 4 | (48, 176) | non échantillonné | 03 | C701 bit 4, C441 = 10h, son 12 | c701 bit 4, armure 10h | oui |
| 4 | 1 | (144, 176) | non échantillonné | 0F | C431 bit 6, C701 bit 7, C70F = 3, son 12 | c431 bit 6 (corrige), c701 bit 7, c70f = 3 | oui |
| 4 | 3 | (16, 176) | 11 | 11 | C702 bit 7, son 12 | c702 bit 7 | oui |
| 4 | 4 | (32, 112) | non échantillonné | 0A | C701 bit 6, son 12 | c701 bit 6 | oui |
| 4 | 4 | (160, 112) | 1C | 1C | C416 = 3, C440 = 96h, son 12 | arme 3, pause 96h | oui |
| 4 | 5 | (16, 144) | 03 | 03 | C701 bit 4, C441 = 10h, son 12 | c701 bit 4, armure 10h | oui |
| 5 | 2 | (160, 112) | 09 | 09 | C434 = 96h, son 12 | killTimer 96h | oui |
| 5 | 3 | (176, 80) | 13 | 13 | score +5000 (C406 00 -> 50), son 10 | score +5000, son 10 | oui |
| 5 | 3 | (176, 144) | 13 | 1E | C701 bit 3, son 12 | c701 bit 3 | oui |
| 6 | 2 | (32, 160) | non échantillonné | 1A | C416 = 1, C440 = 96h, son 12 | arme 1, pause 96h | oui |
| 7 | 1 | (64, 128) | 10 | 10 | C702 bit 6, son 12 | c702 bit 6 | oui |
| 7 | 3 | (208, 96) | 03 | 03 | C701 bit 4, C441 = 10h, son 12 | c701 bit 4, armure 10h | oui |
| 7 | 4 | (64, 64) | 04 | 04 | C701 bit 5, C441 = 10h, son 12 | c701 bit 5, armure 10h | oui |
| 7 | 4 | (160, 112) | 1C | 1C | C416 = 3, C440 = 96h, son 12 | arme 3, pause 96h | oui |
| 7 | 5 | (32, 128) | 0F | 0F | C431 bit 6, C701 bit 7, C70F = 3, son 12 | c431 bit 6 (corrige), c701 bit 7, c70f = 3 | oui |
| 8 | 3 | (32, 128) | 0C | 0C | C431 bit 3, son 12 | c431 bit 3 | oui |
| 9 | 0 | (128, 96) | 09 | 09 | C434 = 96h, son 12 | killTimer 96h | oui |
| 9 | 4 | (224, 160) | 09 | 09 | C434 = 96h, son 12 | killTimer 96h | oui |
| 9 | 5 | (144, 160) | non échantillonné | 02 | C417 +5 (BCD), son 0F | coeurs +5, son 0F | oui |
| 9 | 6 | (32, 64) | 05 | 05 | C43E = 18h (eclair), objets C800 effaces, son 1B | eclair 18h, ennemis effaces, son 1B | oui |
| 9 | 8 | (112, 64) | non échantillonné | 1E | C701 bit 3, son 12 | c701 bit 3 | oui |

Coffres des blocs 0x1F (`+9` & C0 différent de C0, `candles_md.c` : contenu = `+9` & 1F) : stage 2 salle 1 (32, 160) 0A et (64, 160) 0C ; stage 6 salle 2 (128, 128) 09 ; stage 8 salle 0
(160, 112) 09, salle 1 (64, 112) 12 et (96, 112) 0F, salle 5 (160, 80) 02, salle 7 (192, 128) 1B. Prouvés : la mécanique (stage 6 salle 2), tous les effets (fixes pour 02, 09, 0A, 0C, 0F ;
par poke pour 12 et 1B). **Non tracés bloc par bloc** : les sept autres blocs n'ont pas été cassés au fouet dans openMSX (Simon ne les atteint pas avec les positions essayées) ; le port
utilise le même code que pour les coffres fixes.

## Décor du stage 9 : les 9 salles comparées au MSX

Méthode du stage 8 : instantané VRAM MSX de la trace R (`scratchpad/stage9/out/R/snaps`, salle vérifiée par `D000/D001` de l'instantané, 6 à 891 candidats par salle, le meilleur
retenu), couleurs MD `(v + 18) // 36`, capture MD de la salle (`shot`, Simon et ennemis du port présents). Masques : 32 premières lignes (HUD), dessins parasites en (64, 144) et
(192, 144), sprites du MSX (+8 px). Pixels différents restants hors masque : salle 0 : 173 (aigle 08 du port), salle 1 : **0**, salle 2 : 20, salle 3 : **0**, salle 4 : 65, salle 5 : 304,
salle 6 : 618, salle 7 : 264, salle 8 : 332. Tous vus à l'oeil sur des images côte à côte : Simon du port, ennemis du port (hopper, squelettes, statues, aigle) absents de l'instantané MSX
(pris avant leur création) ; **aucune tuile de décor ne diffère**. Coffres, clés, bougies, blocs, porte et clé blanche de la salle 8 sont aux mêmes places. Conforme.
