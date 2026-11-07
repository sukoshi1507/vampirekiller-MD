# Inventaire des objets — bilan vérifiable, 2026-11-07

**Statut : PARTIEL / NON TERMINÉ.** Référence unique : archive `vampirekiller-MD_base_2026-11-07.zip`, commit `8feb0c9`.

## Prouvé (statique)

Audit item par item dans `research/items-inventory.md`, fonctions effectives et références ROM relevées depuis les commentaires des sources C ; bornes explicitement documentées.

Test reproductible : `bash tests/run_pending_items.sh` ; ce test compare la ROM par `probe.load_rom` et contrôle l’intégrité de données source. Il **n'exécute pas le Z80**.

## Non prouvé / à faire

Les effets non portés et les ambiguïtés sémantiques sont détaillés dans l’inventaire ; les traces des stages tardifs restent nécessaires.

## Contraintes respectées

Aucun fichier de branchement interdit (`main.c`, `room_manager.c`, `gameplay.c`, `actors.c`, `candles.*`) modifié ; pas de modification d'`EnemyState`, pas de génération manuelle de `sgdk/src/generated/`, pas d'inclusion de ROM dans le ZIP. Pas de revendication de conformité image par image en l'absence de trace.
