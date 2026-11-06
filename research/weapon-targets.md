# Armes lancées et fouet sur les objets des stages 3 à 9 : traces openMSX

Statut : **tracé** (openMSX, objets lus dans `C800` + 80h k, emplacements `C450`/`C460`, écritures de `+D`, `+E`, `C418`, score `C405-C407`, cœurs `C417`,
effets sonores par point d'arrêt sur `509F`, avec la pile pour les écritures du score). Outils non versionnés : `scratchpad/sw/` (`w_extra.tcl`, `gen.py`,
`sum.py`, `run.py`) ; les plans forcent le stage (`D000`, `D002`, `D001`, `C000` = 4) et posent Simon, l'arme (`C416`), les cœurs et la cible par `poke`.
Test natif : section « openMSX trace » de `tests/test_weapons.c`.

## Résultats (chiffres de la trace)

| Cible | PV | Dague (C416 = 2) | Hache (3) | Croix (4) | Fouet C416 = 0 / 1 | Score |
| --- | --- | --- | --- | --- | --- | --- |
| 0B (stage 8 salle 2) | 4 | -1, la dague disparaît (915 : 4 -> 3) | -4, mort (1309) | -2 puis, après réarmement, -2 : mort (1708, 1788) | -1 / -2 (2069, 2316) | 200 |
| 0A statue (stage 8 salle 6) | 8 | -1 par dague (1618, 1975, 2155 : 7, 6, 5) | -4 puis -4 (3013 : 8 -> 4) | -2 / -2 (3388 : 6 ; 3456 : 4 ; 3948 : mort) | -1 / -2 | 300 (C406 09 -> 12 BCD) |
| 0C (stage 8 salle 6) | 1 | tué | tué (4682) | tué (5068) | tué | 100 |
| 0D (stage 9 salle 5) | 1 | tué (5951) | tué (6359) | tué (6739) | - | 200 |
| boule D700 de la statue, os D700 | - | détruite, la dague disparaît (895, 920) | détruite à chaque réarmement | idem | - | 100 |
| boss 12h (stage 3) | C418 80h | -8 (128 -> 120) | -12 (116), puis -12 (104) 14 images après | -12 (116), puis -12 au retour (1311) | - | - |
| boss 13h (stage 6) | C418 80h | -8 (120, 112, 104, 96) | -12 (116, 104, 92) | -12 (116, 104, 92) | - | - |
| boss 14h (stage 9, deux marcheurs) | C418 80h | -4 (124) | -6 (122) | -6, -6 aller, -6, -6 retour (104 en tout) | -4 / -6 | - |

- **Invulnérabilité** : `+E` bit 0 effacé par chaque touche (`8083`, `7DD9`), remis par `999F` : au lancer (`7286`), au passage d'état de la hache et de la croix
  (`7356`) et pendant les flammes (`7422`). Vu : 0B après la croix, `e` 0 de 1709 à 1727 ; marcheurs de la croix, 2445 et 2472 puis 2555 et 2572.
- **Dégâts** (`8048` : 1, 4, 2, 2 ; `7E59`/`7E60` pour les boss, par `C416` et non par type d'arme) et **points** (`81CE`, premier octet de chaque paire : 0A 300,
  0B 200, 0C 100, 0D 200) confirmés. Le « 200 pour 0B, 100 pour 0C et os » annoncé non vérifié est donc **vérifié** (0C : trace 4682, +1 en `C406`).
- **Coût** : dague, hache, croix 0 cœur (`C417` reste 30) ; eau bénite 5 (30 -> 25 à l'image 893, trace `wat`).
- **Eau bénite sur le 0B** (jetée à l'image 893 en saut, atterrit 953, flammes 955-1001) : -2 à l'atterrissage (952) puis -2 aux flammes (962) : mort, 200 ;
  elle détruit aussi un os en vol (920, +100). Les flammes finissent à l'image 1001 (24 passes), emplacement libéré.
- **Boîtes** : échelles sur la statue immobile, tir à Y 128 : hache et croix touchent à E = 117, pas à 116 (formule `8500` : `(Y - E + H) < H + 12`) ; dague touche à
  124, pas à 120. Les bords hauts de la formule sont confirmés (E 152 touche, 153 non, dague à Y 128) : l'ancien 147 / 148 venait d'un autre objet qui arrêtait l'arme (voir
  `enemy-boxes.md`, qui donne aussi le chaînage du second emplacement C460).
- Hits sur le boss : une touche à l'aller et une au retour, `C418` seul compte pour la mort, le boss 14h compte deux cibles indépendantes.

## Écart trouvé et corrigé

`Enemies_shotHit` (`enemies.c`) écrasait tout `extraPhase` d'un objet à points de vie touché sans mourir (`hp | HIT`) ; le fouet conserve les autres bits. Corrigé
(masque des points de vie seul) : sans effet visible mesuré (le clignotement de la statue est recalculé à chaque passe).

## Non tracé

Stage 6 : le boss 13h n'a été touché qu'à la dague (la table de dégâts est la même que pour 12h, `7E59`). Hache et croix sur lui, fouet sur les boss 12h et 13h (le boss
bouge hors de portée de la lanière dans les plans) : non rejoués ; la table est lue et confirmée sur les stages 3 et 9. Objet 08 et dague sur la statue sans interception : isolés dans `enemy-boxes.md`. Scénario MD aux limites : `zzl_enemy_boxes.txt`.

Mise à jour 2026-11-06 : hache, croix, eau bénite sur 13h et boîtes `84F0` des boss et boules tracées (`boss-boxes.md`) ; les armes détruisent aussi les boules des boss.
