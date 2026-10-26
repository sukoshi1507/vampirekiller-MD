# VampireKiller V20 — Reconstruction visuelle de Simon

Cette version remplace la silhouette d'accroupissement provisoire par des compositions **ROM-derived** basées sur les couples d'indices MSX2 suivants :

- `LOW 06 / UP 00` : accroupi au repos
- `LOW 06 / UP 02` : préparation / récupération candidate
- `LOW 06 / UP 06` : extension candidate 1
- `LOW 06 / UP 09` : extension candidate 2

## Ce qui change

- `tools/build_simon_crouch_md.py` génère désormais des frames à partir de la ROM originale.
- `sgdk/src/generated/simon_crouch_md.*` contient 5 frames de 32x32.
- `sgdk/src/main.c` affiche ces frames pour **BAS** et **BAS + B**.
- `sgdk/src/whip.c` ajoute `vk_whip_crouch_visual_step()`.

## Ce qui est fiable

- la mécanique accroupie est conservée ;
- le fouet accroupi garde la collision validée précédemment ;
- les nouvelles images proviennent bien de la ROM MSX2 ;
- la direction de l'attaque accroupie est figée correctement au déclenchement.

## Ce qui reste à confirmer

- l'ordre exact de ces poses dans la ROM pendant le fouet ;
- les durées exactes de chaque pose ;
- la reconstruction finale du fouet debout à partir d'une trace dynamique openMSX.

## Validation exécutée ici

- `python -m unittest discover -s tests -p 'test_*.py' -q` : 13 tests réussis
- `sh tests/run_native.sh` : gameplay / physique / fouet OK

Compilation SGDK et validation BlastEm de cette V20 : **à confirmer chez toi**.
