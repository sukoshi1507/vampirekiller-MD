# Trace openMSX de l'attaque (fouet) de Simon

## Résultat

Sur la ROM MSX2 de référence (SHA-256 contrôlé), une attaque à la touche SPACE produit toujours la même séquence d'index de composition :

| Ticks | Index basse `C42E` | Index haute `C42F` |
|---|---|---|
| 2 | 0 | 6 |
| 2 | 0 | 7 |
| 8 | 0 | 8 |

- Durée totale : **12 ticks**, soit 0,20 s à 60 Hz.
- 14 attaques déclenchées à 1,5 s d'intervalle : 14 séquences identiques.
- L'index basse reste à 0 pendant tout le coup.
- L'index haute n'atteint pas 9 dans cette trace. L'hypothèse « 6 et 9 » de `ATTACK_ASSETS_V11.md` est donc **incomplète** : la séquence observée est 6 → 7 → 8.

## Méthode

- openMSX 21.0 compilé depuis la source officielle (MD5 de l'archive vérifié), dans un dossier de travail hors du dépôt.
- Machine `C-BIOS_MSX2`, données C-BIOS copiées dans un dossier de travail.
- Script Tcl dérivé de `tools/trace_simon_live.tcl` : lecture de `C42E`/`C42F` à chaque tick du point d'entrée `0x4028`, touche de démarrage `(8,1)` reprise de la routine de titre existante, puis appuis répétés sur la même touche de 40 s à 60 s.
- SPACE = démarrage et attaque, d'après votre indication. La correspondance matricielle `(8,1)` vient du harnais existant ; elle n'a pas été vérifiée autrement.

## Limites

- La trace ne mesure que les **index de composition** de Simon. Le fouet lui-même (la lanière) est peut-être dessiné par une autre routine ou d'autres sprites, non observés ici.
- Le tick est l'échantillon au point d'entrée `0x4028`, pas une mesure directe de la frame affichée. La correspondance avec les 60 Hz repose sur l'intervalle entre appuis.
- Les déplacements (gauche/droite) ne sont pas testés ici ; la correspondance de ces touches reste à établir.
- Le jeu MD n'utilise pas encore cette séquence : `vk_attack_keys` dans `sgdk/src/whip.c` reste une hypothèse.
