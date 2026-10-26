# VampireKiller V22 — Fix d'ancrage du Simon accroupi

Retour utilisateur V21 :
- Simon "flotte" de quelques pixels quand on se baisse ;
- Simon se décale dans la direction où il regarde.

## Cause

Les nouvelles frames accroupies 32x32 sont correctes visuellement, mais n'emploient pas le même point d'ancrage que le sprite debout. Les offsets d'affichage étaient donc faux pour la position au sol et le centrage horizontal.

## Correctif

`sgdk/src/main.c` utilise maintenant des offsets spécifiques aux frames accroupies :

- maintien de l'appui au sol ;
- suppression du décalage latéral lors de l'entrée en position basse ;
- conservation de la mécanique BAS+B et du fouet accroupi.

## Validation exécutée ici

- `python -m unittest discover -s tests -p 'test_*.py' -q` → OK
- `sh tests/run_native.sh` → OK

## À vérifier dans BlastEm

- debout → accroupi → debout ;
- accroupi face à droite et face à gauche ;
- accroupi + fouet dans les deux directions.
