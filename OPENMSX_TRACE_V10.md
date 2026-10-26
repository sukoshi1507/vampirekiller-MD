# V10 — validation des traces openMSX

Cette version **ne change pas le moteur SGDK**. Elle renforce la vérification du protocole de traçage des animations Z80.

## Changements

- `tools/analyze_simon_trace.py` : rapport `quality` (marqueurs de jeu et d'essai clavier, indicateurs manquants, diversité des couples d'indices, trous dans la capture).
- `tools/run_simon_openmsx.py` : vérifie la présence du fichier de fin de trace avant d'analyser les échantillons.
- `tests/test_simon_trace_analysis.py` : tests complémentaires pour éviter les conclusions erronées sur une attaque, détecter les interruptions manquantes et refuser les indices malformés.

**Attention :** `gameplay_unclassified` est uniquement une étiquette du script, pas une preuve visuelle que le personnage est contrôlable. Le booléen `attack_confirmed` reste toujours `false` tant qu'un scénario d'attaque MSX2 n'est pas authentifié indépendamment. La cadence d'interruptions n'est pas automatiquement la cadence réelle d'affichage des sprites.

## Exécuter le test

```sh
python3 -m unittest discover -s tests -p 'test_simon_trace_analysis.py'
python3 tools/run_simon_openmsx.py "/chemin/Vampire Killer (Japan, Europe) (En).zip" \
  --openmsx /usr/bin/openmsx --system-data /usr/share/openmsx \
  --seconds 45 --output build/cache/simon-live
```

Le rapport `build/cache/simon-live/simon-sequences.json` comprend `quality.warnings`. Consulter également `openmsx-trace.log`, `simon-live.csv` et `simon-live-complete.txt` avant toute interprétation.

## Validation locale

- Analyseur : 5 tests Python réussis.
- Gameplay, physique et fouet : tests C natifs réussis.
- Compilation syntaxique des outils Python : réussie.
- openMSX : **non disponible ici**, aucune capture réelle d'attaque effectuée.
- SGDK / BlastEm : non relancés pour cette livraison, code du moteur inchangé.
