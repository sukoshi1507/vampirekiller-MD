# VampireKiller V21 — Correction de l’assemblage accroupi

Retour utilisateur V20 : le bon sprite était retrouvé, mais le haut et le bas du corps n’étaient pas suffisamment rapprochés. Simon paraissait « coupé en deux » en position accroupie.

## Correctif appliqué

Le générateur `tools/build_simon_crouch_md.py` compresse maintenant les grands espaces transparents entre bandes occupées du sprite accroupi ROM-derived.

Cela conserve :
- les graphismes extraits de la ROM ;
- la mécanique BAS et BAS+B ;
- l’orientation et les collisions du fouet.

Cela corrige :
- le vide artificiel entre torse et jambes pour `LOW 06 / UP 00` ;
- le même problème pour la pose `LOW 06 / UP 02`.

## Validation exécutée ici

- `python -m unittest discover -s tests -p 'test_*.py' -q` → OK
- `sh tests/run_native.sh` → OK

## Limite restante

Cette V21 améliore l’assemblage visuel, mais l’ordre temporel exact des poses de fouet reste encore à confirmer par trace dynamique MSX2.
