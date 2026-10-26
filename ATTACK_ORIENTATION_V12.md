# V12 — Correction de l’orientation des poses d’attaque

## Symptôme BlastEm
Lors d’une attaque, Simon semble pivoter vers la direction opposée. Les commandes, l’orientation du déplacement, la physique et les escaliers fonctionnent.

## Cause et correction
Dans `sgdk/src/main.c`, les deux sprites d’attaque de 32×32 utilisaient un miroir horizontal appliqué quand `whip.facing == 0` (droite), et aucun miroir quand `whip.facing == 1` (gauche). La correction inverse cette condition et l’ordre des deux moitiés graphiques : droite = affichage direct, gauche = miroir + inversion des moitiés.

La convention `player.facing` est 0 pour droite et 1 pour gauche. Le latch `whip.facing` reste inchangé. Les trajectoires, animations d’escalier et collisions du fouet ne changent pas.

## Limites
Les poses d’attaque sont extraites de la ROM MSX2, mais leur ordre et leurs durées ne sont pas encore validés par une trace dynamique openMSX. Le rendu nécessite validation BlastEm sur cette V12.

## Essais
Compiler SGDK, lancer BlastEm, attaquer à droite puis à gauche ; vérifier que Simon conserve son orientation avant/pendant/après B. Tester ensuite les escaliers, sans changement attendu.
