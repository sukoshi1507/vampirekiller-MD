# Canaux 40h/80h — bilan vérifiable, 2026-11-07

**Statut : PARTIEL / NON TERMINÉ.** Référence unique : archive `vampirekiller-MD_base_2026-11-07.zip`, commit `8feb0c9`.

## Prouvé (statique)

Flags 12–18 comparés octet à octet à la ROM : salles marquées 40h/80h, avec nombre par stage documenté dans la sortie de test.

Test reproductible : `bash tests/run_pending_channels.sh` ; ce test compare la ROM par `probe.load_rom` et contrôle l’intégrité de données source. Il **n'exécute pas le Z80**.

## Non prouvé / à faire

Chemin 5EB8h/5ED8h, choix des types, fréquence et direction non démontrés. Ne pas brancher sur des spawns supposés.

## Contraintes respectées

Aucun fichier de branchement interdit (`main.c`, `room_manager.c`, `gameplay.c`, `actors.c`, `candles.*`) modifié ; pas de modification d'`EnemyState`, pas de génération manuelle de `sgdk/src/generated/`, pas d'inclusion de ROM dans le ZIP. Pas de revendication de conformité image par image en l'absence de trace.
