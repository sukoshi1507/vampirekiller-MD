# Ennemi 10h — bilan vérifiable, 2026-11-07

**Statut : PARTIEL / NON TERMINÉ.** Référence unique : archive `vampirekiller-MD_base_2026-11-07.zip`, commit `8feb0c9`.

## Prouvé (statique)

ROM `9942h → ADF8h` ; 28 occurrences stages 14–18. Vérifications `ADF8h`, `AF40h`, `9F61h`, `9F83h` des précédentes livraisons reprises et contrôles additionnels du bloc code (hash).

Test reproductible : `bash tests/run_pending_enemy10.sh` ; ce test compare la ROM par `probe.load_rom` et contrôle l’intégrité de données source. Il **n'exécute pas le Z80**.

## Non prouvé / à faire

Pas de trace dynamique : trajectoire, champs initialisés après 9FAAh, cadence et sprites non validés. Aucun raccord C.

## Contraintes respectées

Aucun fichier de branchement interdit (`main.c`, `room_manager.c`, `gameplay.c`, `actors.c`, `candles.*`) modifié ; pas de modification d'`EnemyState`, pas de génération manuelle de `sgdk/src/generated/`, pas d'inclusion de ROM dans le ZIP. Pas de revendication de conformité image par image en l'absence de trace.
