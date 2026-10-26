# Fusion conservatrice — projet principal de référence

## Source et politique

- Base active: projet personnel `vampirekiller` (archive de sequences Z80, incluant gameplay, escaliers V3, fouet V6 et outils d extraction).
- Projet comparé: `vampirekiller2.zip` (branche `BASELINE-01`, jalon 4).
- Le code de `sgdk/src` du projet principal **n a pas été remplacé** par le code du partenaire.
- La ROM livrée dans `build/` est un ancien binaire témoin; **recompiler avec SGDK** avant tout test des nouveaux modules.

## Audit de différences

- 90 chemins communs dont 80 identiques (hors .git); 10 fichiers diffèrent.
- Le partenaire ne possède pas `gameplay.c`, `whip.c`, `stage1_collision.c`, ni les tests natifs et audits de Simon apparus dans le projet principal.
- Ses apports différenciants portent sur l'observation du démarrage MSX2 (référence openMSX), la vérification d'une compilation reproductible et les consignes de toolchain.
- Aucun portage plus avancé des ennemis, du fouet, ou des escaliers n'est présent dans le code partenaire fourni.

## Éléments intégrés

- `tools/verify_msx_startup.py`: validation du démarrage à partir de la référence indépendante, avec la ROM originale.
- `tools/capture_msx_startup.py` et `.tcl`: procédure pour reprendre une capture openMSX.
- `validation/msx/startup-reference.zip`: archive de référence du partenaire, contrôlée par l'empreinte dans `config/baseline.json`.
- `research/partner-baseline/`: sa documentation et ses scripts originaux de build, archivés sans modifier le système de compilation actif.

## Vérifier

```sh
sh tests/run_native.sh
python3 tests/verify_hall_map.py
python3 tools/verify_msx_startup.py "/chemin/vers/Vampire Killer (Japan, Europe) (En).rom"
```

La troisième commande nécessite les dépendances `Pillow==12.3.0`, `z80==1.2.0` et `z80dis==1.0.6` (voir `research/partner-baseline/requirements-validation.txt`), la ROM MSX2 exactement attendue et la référence partenaire. Dans cet environnement elle n’a pas pu être exécutée car le module Python `z80` n’est pas installé. Pour la ROM Mega Drive, continuer avec le Makefile SGDK principal : `make -C sgdk GDK=/chemin/vers/SGDK`.

## Reste à valider

- Exécuter la vérification openMSX fusionnée sur la ROM source exacte et traiter les éventuelles différences des ressources d'intro.
- Recompiler avec l'installation SGDK du développeur et vérifier BlastEm.
- Comparer et adapter les tests de reproductibilité aux fichiers sources actuels avant de les rendre obligatoires.
- Tracer les animations Z80 en exécution afin d'authentifier les séquences de fouet et d'escalier.

Ce merge conserve une séparation claire entre **validations historiques importées** et **tests exécutés sur le moteur actif**.
