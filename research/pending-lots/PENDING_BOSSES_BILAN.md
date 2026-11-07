# Boss 12, 15, 18 / fin — bilan vérifiable, 2026-11-07

**Statut : PARTIEL / NON TERMINÉ.** Référence unique : archive `vampirekiller-MD_base_2026-11-07.zip`, commit `8feb0c9`.

## Prouvé (statique)

Références existantes `sgdk/src/boss.c`, `research/stage3-boss.md`, `stage6-boss.md`, `stage9-boss.md`, `boss-boxes.md`. ROM globale certifiée ; cohérence des dispatch ennemis revérifiée.

Test reproductible : `bash tests/run_pending_bosses.sh` ; ce test compare la ROM par `probe.load_rom` et contrôle l’intégrité de données source. Il **n'exécute pas le Z80**.

## Non prouvé / à faire

Aucune IA de boss tardif, boîte de collision, jauge, mort ou séquence de fin validée ; non intégrées.

## Contraintes respectées

Aucun fichier de branchement interdit (`main.c`, `room_manager.c`, `gameplay.c`, `actors.c`, `candles.*`) modifié ; pas de modification d'`EnemyState`, pas de génération manuelle de `sgdk/src/generated/`, pas d'inclusion de ROM dans le ZIP. Pas de revendication de conformité image par image en l'absence de trace.
