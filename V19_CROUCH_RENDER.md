# V19 — Correction du rendu accroupi

## Diagnostic
La V18 traitait `extended-20.png` (23x17 pixels) comme une pose complète d'accroupissement. Or le fragment n'est pas une silhouette complète de Simon et n'a pas été identifié avec certitude par une trace des indices Z80.

## Correctif
Le générateur `tools/build_simon_crouch_md.py` utilise désormais la silhouette complète déjà extraite et utilisée en marche (`assets/player/pose-00.png`) et la compresse verticalement sur 22 pixels en conservant l'ancrage au sol. Cette **pose provisoire reconnaissable** remplace le fragment, mais **ne reproduit pas encore la véritable animation originale d'accroupissement MSX2**. Le catalogue de sprites d'origine reste intact pour les recherches futures.

Aucune modification à la physique, aux collisions, à la hauteur du coup de fouet accroupi, aux escaliers ni à la navigation. L'orientation horizontalement reflétée reste appliquée par le moteur.

## Tests
- `tests/run_native.sh` réussi
- `python -m unittest discover -s tests -p 'test_*.py' -q` : 13 tests réussis
- Compilation SGDK et émulation BlastEm **non effectuées** pour cette version.

## Test BlastEm
Maintenir BAS dans la cour, observer la pose, presser B en restant accroupi, répéter vers la gauche et la droite et vérifier que le fouet touche toujours les flammes.
