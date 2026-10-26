# VampireKiller V26 — Fouet graphique MSX2

Cette version conserve le moteur V25 validé et remplace, pendant les attaques, les segments de fouet dessinés artificiellement par les **graphismes présents dans les compositions du personnage extraites de la ROM MSX2**.

## Intégration

- Le moteur ne dessine plus les trois carrés de fouet temporaires par-dessus l'attaque ROM. Les sprites matériels restent chaînés mais masqués.
- Les compositions MSX2 d'attaque debout incluent des tracés du fouet dans les parties hautes des images.
- Les compositions candidates accroupies `LOW 06 / UP 06` et `LOW 06 / UP 09` incluent elles aussi le fouet. La table d'animation garde ces deux variantes pendant les étapes actives de collision.
- Les tests des fenêtres de collision et des orientations restent en place.

## Fidélité et limites

Les pixels des compositions proviennent bien de la ROM MSX2 d'origine. En revanche, l'**ordre des poses et leur durée** ne sont toujours pas établis par une capture dynamique Z80. Les masques de collision actuels du portage restent les mêmes ; ils ne reproduisent pas automatiquement les courbes exactes du fouet visible.

Cette V26 ne comporte pas de nouveaux ennemis ni de modification de l'accroupissement ou des escaliers. La compilation SGDK et le rendu BlastEm doivent être vérifiés sur la machine de développement.

## Tests effectués

- `sh tests/run_native.sh` — OK
- `python3 -m unittest discover -s tests -p 'test_*.py' -q` — 14 tests OK

## Vérifier dans BlastEm

Depuis la cour, frapper une flamme en position debout, face aux deux directions, puis en position accroupie dans les deux directions. Vérifier le déploiement visuel du fouet ROM et l'absence de doublage graphique, tout en contrôlant que le contact sur la cible fonctionne encore.
