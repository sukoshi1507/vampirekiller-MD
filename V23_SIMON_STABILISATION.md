# VampireKiller V23 — Stabilisation de Simon accroupi

Base : V22 confirmée visuellement comme meilleure sous BlastEm.

## Correctif

`sgdk/src/main.c` conserve désormais la silhouette accroupie pendant toute la durée d'une attaque commencée accroupi, même si BAS est relâché avant la fin du fouet. Cette sélection s'appuie sur `whip.crouched`, figé au début de l'attaque. Aucune modification de la trajectoire, de la collision ni des escaliers.

La précédente correction d'ancrage V22 (offsets par frame et orientation) est conservée.

## Régressions automatisées

Le test `tests/test_simon_crouch.py` vérifie les 5 frames, le contact des pixels de pieds avec la ligne `y-1`, le centrage horizontal au repos à droite et à gauche, et le verrouillage de la pose d'attaque.

- 14 tests Python : réussis.
- Tests natifs C (gameplay, physique, fouet, cibles) : réussis.

La compilation SGDK et la validation BlastEm de **V23** restent à effectuer chez toi. La chronologie originale MSX2 de l'attaque n'est pas encore authentifiée.
