# V8 — banc de trace dynamique openMSX des animations de Simon

## État de validation

- La ROM de référence est contrôlée par SHA-256 avant lancement.
- Le script Tcl échantillonne `C42E` (bas) et `C42F` (haut), ainsi que les variables connexes à chaque passage à `4028` (routine d'interruption documentée dans le projet).
- Le pulse ESPACE au titre réutilise la technique du banc openMSX de démarrage.
- Les colonnes `C422` / `C423` sont volontairement désignées *raw*, leur rôle exact n'étant pas confirmé par cette expérience.
- Le script agrège les couples d'indices consécutifs en segments avec durée en interruptions, sans prétendre identifier une séquence d'attaque.
- **La trace n'a pas été exécutée dans l'environnement de production de cette archive : openMSX n'y était pas installé.** L'exécution, la correspondance de `4028` et les entrées gameplay doivent être vérifiées.

## Lancement (Linux, avec openMSX et C-BIOS_MSX2 configurés)

```bash
python3 tools/run_simon_openmsx.py "/chemin/Vampire Killer (Japan, Europe) (En).zip" \
  --openmsx /usr/bin/openmsx \
  --system-data /chemin/vers/share/openmsx \
  --output build/cache/simon-live
```

Fichiers créés : `simon-live.csv`, `simon-sequences.json`, `simon-live-complete.txt` et `openmsx-trace.log`. Si la capture ne produit aucun échantillon, vérifier l'exécution du breakpoint et le mapping de la ROM.

## Limite importante : fouet

Cette première capture démarre la partie, mais **ne déclenche pas automatiquement le fouet MSX2** : la correspondance de la commande d'attaque sur la machine MSX2 n'a pas encore été confirmée. Elle ne permet donc pas, seule, de publier les durées de l'attaque authentique. Pour l'obtenir, il faut instrumenter une commande d'attaque validée et marquer son intervalle dans le journal. La ROM SGDK n'est pas modifiée.

## Reproductibilité

L'archive n'inclut pas la ROM commerciale ni un BIOS. Le chargement est assuré via `--rom`/`--system-data` côté utilisateur. Le lanceur refuse les variantes avec empreinte différente et le traceur n'altère pas la RAM de jeu (hormis une entrée clavier ESPACE au titre).

## V9 — Analyse des segments et expérience clavier optionnelle

Le script `tools/analyze_simon_trace.py` regroupe les couples `(C42E,C42F)` par interruptions consécutives et fournit des statistiques distinctes pour chaque marqueur. Les sorties ne prouvent **ni l'affichage effectif des frames ni le déclenchement du fouet** sans contrôle visuel ou validation de la commande MSX2.

Lanceur V9 (essai clavier non authentifié, **facultatif**) :

```bash
python3 tools/run_simon_openmsx.py "Vampire Killer (Japan, Europe) (En).zip" \
  --openmsx /usr/bin/openmsx --system-data /usr/share/openmsx \
  --seconds 45 --experimental-key 8 0 --probe-delay 6 --probe-hold 0.15
```

**Attention : `8 0` est un exemple de coordonnées de matrice et PAS une commande de fouet validée.** Le traceur marque `experimental_input_held` puis `experimental_input_released`, sans modifier directement la RAM. Le délai est compté depuis la détection du titre. Cette instrumentation sert à découvrir le mapping réel de l'attaque, une fois le jeu en cours.

Pour réanalyser une trace existante :

```bash
python3 tools/analyze_simon_trace.py build/cache/simon-live/simon-live.csv \
  --output build/cache/simon-live/simon-pair-analysis.json
```

L'exécutable openMSX n'est pas disponible ici : aucune mesure dynamique nouvelle n'a été produite avec cette V9. Les tests synthétiques du parseur se lancent avec `python3 -m unittest discover -s tests -p 'test_simon_trace_analysis.py'`.
