# V25 — Stabilisation du fouet accroupi

**Base : V24.** Le défaut signalé était un corps visuellement séparé pendant le fouet accroupi.

## Modifications

- Reconstruction des poses d'accroupissement à partir des couches graphiques de la ROM MSX2 de référence (SHA-256 contrôlé par `tools/probe.py`).
- Remplacement de la compression verticale dépendant des lignes transparentes par un décalage déterministe de 6 pixels de la composante basse, identique pour toutes les poses.
- Ancrage SGDK unique de `y - 25` ; dernier pixel de la silhouette au niveau `y - 1` pour chaque frame.
- L'entrée haute 9 reste dans l'atlas de recherche, mais n'est plus utilisée pour la pose accroupie jouable parce que ses couches pendantes traversent la ligne du sol avec ce cadrage. La pose haute 6 est maintenue durant l'extension, et le fouet graphique séparé reste visible.
- Aucune modification des états de gameplay, de la détection d'impact ou des escaliers.

## Traces Z80

**Aucune trace dynamique réelle acquise.** openMSX n'est pas installé ici, et aucune source de paquet installable n'a été trouvée. La correspondance temporelle exacte des séquences originales reste donc non authentifiée. L'archive conserve le banc de capture existant.

## Validation

- 14 tests Python réussis.
- Tests C natifs gameplay/physique/fouet réussis.
- Compilation SGDK et émulation BlastEm de la V25 non exécutées.
