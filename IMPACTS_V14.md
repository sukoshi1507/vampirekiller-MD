# V14 — Cible d'entraînement et impacts du fouet

Cette version repart directement de la V13 (validée par le développeur sous SGDK/BlastEm).

## Changements
- `sgdk/src/targets.c/.h` : cible rectangulaire testable sans SGDK ; intersection avec `vk_whip_bounds`, un seul impact par cible, disparition après contact.
- `sgdk/src/main.c` : cible 16x16 dessinée avec un sprite Mega Drive supplémentaire, à droite de la position initiale de Simon dans le hall et les salles de test. Elle utilise un motif géométrique temporaire, **pas** un asset du MSX2.
- Dans le château : `A+B` replace la cible près de Simon ; `B` déclenche le fouet. Le message `CIBLE TOUCHEE!` atteste un impact. Dans la salle d'essai accessible depuis l'explorateur, le fouet est déclenché par `A` comme auparavant.
- Les compositions de Simon V13, la convention d'orientation V12 et la physique d'escalier sont conservées.
- `tests/test_targets.c` : vérifie les impacts gauche/droite, un seul impact par cible, absence de collision hors des limites.

## Contrôles BlastEm
1. Entrer dans le château et repérer le carré-cible à droite de Simon.
2. Approcher et presser B : le fouet doit toucher le carré, le faire disparaître et afficher `CIBLE TOUCHEE!`.
3. Tester avec Simon orienté vers la gauche : se déplacer à droite de la cible avant de presser B.
4. Presser A+B pour replacer une cible ; confirmer que l'escalier (A+HAUT, HAUT/BAS dans la salle 1) fonctionne toujours.

## Limites
- Cible et sprites graphiques de la cible sont des outils de test, pas des objets reconstruits depuis la ROM.
- Pas de score, de points de vie, d'ennemis, ni de dégâts authentiques MSX2.
- Tests C natifs réussis ; compilation SGDK/BlastEm **non exécutée** dans cet environnement.
