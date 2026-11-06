# Boîtes de contact des boss (stages 3, 6, 9) tracées en openMSX

Code : `sgdk/src/boss.c` (`contact_box`, `whip_box`, `vk_boss_shot_*`). Tests : `tests/test_boss.c`, `tests/test_stage9.c`, scénario `zz_stage6_ball_box.txt`.

Méthode : points d'arrêt openMSX sur les routines de boîte de la ROM (`847A` contact, `84A7` fouet, `84F0` armes lancées) : HL (H, L de la table
`8318`/`8338`/`8420`...), DE (X, Y de l'objet), appelant (pile), Simon (`C427`, `C425`, `C420`, `C42C`, `C416`), plus les écritures de `C415` (énergie de Simon)
et `C418` (énergie du boss). Simon est posé en X aléatoire toutes les 25 images (énergie et `C418` remis), fouet une fois sur deux. Outils non versionnés
(`bx.tcl`, `gen.py`, `an*.py`, dossier de scratch de session). Appelants : `7E94` corps, `7F0E` boule (contact), `7DC9` corps (fouet), `7F63` boule (fouet).

## Résultats

| Cas | HL tracé (H, L) | boss.c avant | Contact (passes avec dégât) |
|---|---|---|---|
| corps stage 3 (12h) | 180C | (18h, 0Ch) | idem formule (hit à \|dX\| 11, rien à 17) |
| boule stage 3 | **0603** | (6, 3) | formule vérifiée (hits \|dX\| 1, 3, 5) |
| corps stage 6 (13h) | 180C | (18h, 0Ch) | hits jusqu'à \|dX\| 13, rien à 17 |
| boule stage 6 | **0606** | (6, 3) **écart** | hits à \|dX\| 8 et 9 (impossibles avec L = 3 : \|dX\| < 8), rien à 11 et 13 |
| corps stage 9 (14h) | 2805 | (28h, 5) | hit à \|dX\| 9, rien à 10 (limite exacte) |
| boule stage 9 | 0606 | (6, 6) | HL seul (pas de passage près de la limite) |

Fouet sur le corps (`84A7`) : formule de `whip_box` reproduite sans écart, limite exacte au stage 9 (demi-largeur 17 : hit à \|dX\| de la pointe = 16, rien à 17) et
stage 6 (hits à demi-largeur - 3, rien à + 2). Compte : stage 3 3 coups/75 passes, stage 6 5/49, stage 9 16/114 (4 dégâts de `C418` sans passe `84A7` correspondante :
autre source de dégât de la même image).

## Corrigé

Boule du stage 6 : (6, 6) au lieu de (6, 3) (contact et fouet), `boss.c`. Test natif et scénario sur la limite (\|dX\| 11 sans effet, 10 touche).

## Oracle : limites exactes par appel forcé (2026-11-06)

Méthode : aux appels naturels des routines `847A`, `84A7` et `84F0` (appelants `7E94`/`7F0E` contact, `7DC9`/`7F63` fouet, `803D`/`7F96` armes), un point
d'arrêt à l'entrée relève l'appel, puis **ré-exécute la routine de la ROM** (PC remis à l'entrée, adresse de retour empilée, point d'arrêt au retour qui lit la
retenue) avec des paramètres forcés : D, E (objet), `C420`, `C42C`, `C416`, emplacements d'armes `C450`+1/+4/+5 ; H, L remis à chaque essai (la routine les modifie).
Les valeurs d'origine sont rendues et l'appel naturel s'achève (retenue finale = retenue d'origine : 306 appels naturels de `847A` conformes à la formule du port, 0 écart).
Chaque balayage couvre dY de -(H+40) à +30 par état de `C420` (0 à 3), dX de -(L+10) à +(L+10), pour le fouet la pointe ± 30 (cuir et chaîne, regard gauche et droit), pour
les armes les types 2 à 5. Outils non versionnés : `bb.tcl`, `an.py`, `edges.py` (dossier de scratch de session). Sites et sorties : stage 3 (12h, boule 0603), stage 6 (13h,
boule 0606), stage 9 (marcheurs 2805, boule 0606), Simon en vol libre, boss, boules et armes réels. **Tous les balayages concordent avec les formules de `boss.c` et
`weapons.c` : 0 écart sur plus de 40 000 essais.** Bornes relevées (dX = X objet - X Simon ou - pointe ; dY = Y objet - C425, ou Y arme - Y objet) :

| Boîte (HL) | Horizontal | Vertical |
|---|---|---|
| contact corps 180C (stages 3, 6) | \|dX\| <= 16 | debout -27..22, saut (état 1) -27..14, accroupi (2) -19..22, état 3 -27..22 |
| contact marcheur 2805 (stage 9) | \|dX\| <= 9 | debout -27..30+ (borne haute hors du balayage), accroupi -19..30+ |
| contact boule 0603 (stage 3) | \|dX\| <= 7 | debout -27..4, saut -27..-4, accroupi -19..4 |
| contact boule 0606 (stages 6, 9) | \|dX\| <= 10 (observé, plus déduit) | idem |
| fouet corps 180C | pointe ± 23 (cuir), ± 27 (chaîne) | debout et saut -21..6, accroupi -13..14 |
| fouet marcheur 2805 | pointe ± 16 / ± 20 | debout -21..22+, accroupi -13..30+ |
| fouet boule 0606 (`7F63`) | pointe ± 17 / ± 21 | debout -21..-12, accroupi -13..-4 |
| armes corps 180C (`803D`) | \|dX\| <= 17 | dague -24..5, hache, croix, eau bénite -24..11 |
| armes boule 0606 / 0603 (`7F96`) | \|dX\| <= 11 / <= 8 | dague -6..5, autres -6..11 |

Le contact debout-saut-accroupi suit donc `847A` (`C420` = 1 ou 2 : hauteur 12h au lieu de 1Ah ; `C420` = 1 : Y de Simon - 8), et la boule du stage 6 à \|dX\| = 10 touche,
11 non (limite maintenant observée). Fouet contre les boules (`7F63`) : mesuré aux stages 6 (ci-dessus) ; l'appel du stage 3 existe (`HL = 0603`) mais le gabarit est
celui de la même routine. Le regard à droite (`C42C` = 0) n'a pas été vu au stage 6 (Simon collé au bord droit : pointe > 255, test `84B6`) mais l'a été aux stages 3 et 9, mêmes lignes de code.

## Armes lancées (`84F0`) et dégâts

- Les armes lancées **détruisent les boules** (`7F79` -> `8258` -> `8328`/`8348` -> `84F0`, `7F96`) : l'objet D700 devient type 0Ch (effet de mort `822A`, son 0C,
  100 points), la hache et la croix continuent, la dague disparaît (`7F9E`, `74EC`). Trace p6b, image 2254 : hache à (A0, B8) contre la boule (A0, C3), D700 `06` -> `0C`.
  **Écart trouvé et corrigé** : le port ne le faisait pas (`actors.c`, `vk_boss_shot_target`/`vk_boss_shot_kill`).
- Dégâts sur le boss 13h (stage 6, `C418` 80h) : dague 8 (120, 112, 104, 96), hache 12 (116, 104, 92), croix 12 (116, 104, 92), comme 12h (table `7E59`/`7E60` par `C416`) ; eau bénite (`C416` = 0) 8 aux
  stages 3 et 6 ; stage 9 : dague 4, hache et croix 6 (déjà tracés), eau bénite non rejouée sur 14h.
- Eau bénite (jetée en saut, `C701` = 9, GAUCHE) sur 12h et 13h : flammes de 24 passes, `999F` (rechargement de `+E` bit 0) aux passes où `C003` & 4, d'où des coups sur
  4 passes consécutives dès que le boss entre dans la flamme (trace w6, images 1250 à 1258, `C003` 3B à 3F, -8 chacun pour `C416` = 0 ; stage 3 : séries de 3 à 4 coups, -8).
  Même rythme que `weapons.c` (`tick & 4`).

## Non corrigé / non tracé

- Fouet sur le corps aux stages 3 et 6 : la boîte est confirmée par l'oracle, pas par un coup réel du stage 3 (trace B du stage 6 : 10 coups, -8, déjà dans `stage6-boss.md`).
- `zz_stage6_boss.txt` : non modifié. Hypothèse seulement, non prouvée : un coup d'essai laisse `armed` du boss à 1 (`vk_boss_target` l'ignore ensuite) tant qu'un fouet
  fini, une arme au rechargement ou `999F` ne l'a pas remis, ce qui ferait manquer le coup suivant.
- Les bords hauts du balayage du marcheur 2805 (dY > 30) ne sont pas bornés ; les valeurs du port suivent la formule, sans écart jusque-là.
