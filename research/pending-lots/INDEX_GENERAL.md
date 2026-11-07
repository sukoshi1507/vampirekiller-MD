# Vampire Killer MD — bilan consolidé des 8 lots du 7 novembre 2026

Référence : archive base du 2026-11-07 (commit déclaré `8feb0c9`). Aucun ancien fichier de branchement restauré.

**Résultat : audit multi-lots livré ; comportements tardifs NON TERMINÉS.**

- Types fixes manquants : 09h (13 occurrences, AF5Fh), 10h (28, ADF8h), 12h (6, BE70h), soit 47, sources : ROM 9942h et liste figée `research/late-enemy-unported-worklist.json`.
- Canaux tardifs : 74 salles de stages 12–18 ; tableaux des flags comparés aux octets ROM ; les bits 40h/80h ne sont PAS interprétés comme des créations démontrées.
- Boss, marchands, murs et secrets tardifs : non programmés faute de traces des routines pertinentes, inventaire des sources indiqué dans bilans respectifs.
- Les 12 catégories d'objets sont inventoriées avec code C et adresses ROM dans `research/items-inventory.md`. Plusieurs effets sont partiels.
- Tests : `bash tests/run_native.sh` terminé avec code retour 0, contrôles ROM statiques et tests natifs. Pas de validation dynamique Z80 de la fidélité des nouveaux comportements.
- Environnement : module Python `z80` absent. Il est indispensable d'obtenir les exécutions / traces pour respecter « on TRACE » avant d'ajouter du gameplay.

## Sécurité d'intégration

Aucune modification de `sgdk/src/enemies.c`, `sgdk/src/enemies.h`, `sgdk/src/main.c`, `room_manager.c`, `gameplay.c`, `actors.c`, `candles.*`, ni de `sgdk/src/generated/`. Pas de ROM dans les archives livrées.

## Fichiers livrés

- `tools/audit_pending_lots.py`, contrôles `--check` de la ROM et des éléments source.
- `tests/run_pending_{enemy09,enemy10,enemy12,channels,bosses,merchants,secrets,items}.sh`.
- `tests/run_native.sh`, appels aux huit tests supplémentaires.
- `LIVRAISON/PENDING_*_BILAN.md`, un bilan par lot.
- `research/items-inventory.md`.
- Lot 10 : investigation supplémentaire déjà produite, scripts et notes ROM (dans son ZIP).

Les ZIP individuels contiennent seulement les nouveaux fichiers du lot et les fichiers de test mutualisés requis. L'archive consolidée est la référence pratique pour intégrer les huit audits d'un coup.
