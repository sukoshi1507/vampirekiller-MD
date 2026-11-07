# Marchands / murs 12–18 — bilan vérifiable, 2026-11-07

**Statut : PARTIEL / NON TERMINÉ.** Référence unique : archive `vampirekiller-MD_base_2026-11-07.zip`, commit `8feb0c9`.

## Prouvé (statique)

Référence des routines `9179h–954Fh`, `9428h`, `9418h` dans `research/merchant.md` ; code existant `sgdk/src/wall.c`, table `vk_shop_prices[9][4]` et achat `vk_candles_give` contrôlés.

Test reproductible : `bash tests/run_pending_merchants.sh` ; ce test compare la ROM par `probe.load_rom` et contrôle l’intégrité de données source. Il **n'exécute pas le Z80**.

## Non prouvé / à faire

Tables des vendeurs tardifs non extraites dans ce lot, pas de génération de prix ni de collisions murs tardifs : non intégrés.

## Contraintes respectées

Aucun fichier de branchement interdit (`main.c`, `room_manager.c`, `gameplay.c`, `actors.c`, `candles.*`) modifié ; pas de modification d'`EnemyState`, pas de génération manuelle de `sgdk/src/generated/`, pas d'inclusion de ROM dans le ZIP. Pas de revendication de conformité image par image en l'absence de trace.
