# Passages secrets 13–18 — bilan vérifiable, 2026-11-07

**Statut : PARTIEL / NON TERMINÉ.** Référence unique : archive `vampirekiller-MD_base_2026-11-07.zip`, commit `8feb0c9`.

## Prouvé (statique)

Point de départ `research/stage12-foundations.md` et routine `85F4h/70FBh/B98Ch`, table `BBCDh` indiqués dans TRAVAIL.md ; validation de l’empreinte ROM globale.

Test reproductible : `bash tests/run_pending_secrets.sh` ; ce test compare la ROM par `probe.load_rom` et contrôle l’intégrité de données source. Il **n'exécute pas le Z80**.

## Non prouvé / à faire

Aucune destination tardive ni condition de déclenchement attestée. Pas de branchement.

## Contraintes respectées

Aucun fichier de branchement interdit (`main.c`, `room_manager.c`, `gameplay.c`, `actors.c`, `candles.*`) modifié ; pas de modification d'`EnemyState`, pas de génération manuelle de `sgdk/src/generated/`, pas d'inclusion de ROM dans le ZIP. Pas de revendication de conformité image par image en l'absence de trace.
