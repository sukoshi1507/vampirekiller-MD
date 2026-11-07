# Ennemi 12h — bilan vérifiable, 2026-11-07

**Statut : PARTIEL / NON TERMINÉ.** Référence unique : archive `vampirekiller-MD_base_2026-11-07.zip`, commit `8feb0c9`.

## Prouvé (statique)

ROM `9942h → BE70h` ; fenêtre BE70–BF49 vérifiée par hash, six occurrences stage 16.

Test reproductible : `bash tests/run_pending_enemy12.sh` ; ce test compare la ROM par `probe.load_rom` et contrôle l’intégrité de données source. Il **n'exécute pas le Z80**.

## Non prouvé / à faire

Routine possiblement partagée : initialisation, état initial, IA, sprites et collisions non établis par trace.

## Contraintes respectées

Aucun fichier de branchement interdit (`main.c`, `room_manager.c`, `gameplay.c`, `actors.c`, `candles.*`) modifié ; pas de modification d'`EnemyState`, pas de génération manuelle de `sgdk/src/generated/`, pas d'inclusion de ROM dans le ZIP. Pas de revendication de conformité image par image en l'absence de trace.
