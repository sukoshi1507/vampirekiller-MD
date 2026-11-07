# Ennemi 09h — bilan vérifiable, 2026-11-07

**Statut : PARTIEL / NON TERMINÉ.** Référence unique : archive `vampirekiller-MD_base_2026-11-07.zip`, commit `8feb0c9`.

## Prouvé (statique)

ROM `9942h → AF5Fh` ; fenêtre AF5F–B08F, SHA-256 vérifié ; 13 occurrences stages 13–14, données de `research/late-enemy-unported-worklist.json`.

Test reproductible : `bash tests/run_pending_enemy09.sh` ; ce test compare la ROM par `probe.load_rom` et contrôle l’intégrité de données source. Il **n'exécute pas le Z80**.

## Non prouvé / à faire

Spawn, step, sprites, collisions, cadence et trace Z80 par image manquants. Aucun port C annoncé.

## Contraintes respectées

Aucun fichier de branchement interdit (`main.c`, `room_manager.c`, `gameplay.c`, `actors.c`, `candles.*`) modifié ; pas de modification d'`EnemyState`, pas de génération manuelle de `sgdk/src/generated/`, pas d'inclusion de ROM dans le ZIP. Pas de revendication de conformité image par image en l'absence de trace.
