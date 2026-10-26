# Décodage des 14 entrées supplémentaires de Simon

ROM de référence : `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2` (SHA-256).

Commande de reproduction depuis la racine du projet :

```bash
python tools/decode_extended_simon.py '/chemin/vers/Vampire Killer (Japan, Europe) (En).rom' --output research/simon-assets/extended
```

## Résultat

Les index **6, 7, 8, 9, 10, 11, 13, 14, 19, 20, 21, 22, 23** sont décodés comme des compositions de couches 16×16 positionnées d'après les tables de la ROM. **Index 12** : deux couches disposent de coordonnées, quatre couches bitmap supplémentaires ont été décodées séparément sans emplacement certifié. Elles figurent dans des PNG `unpositioned` ; aucun placement fictif n'a été ajouté.

La taille des images varie (jusqu'à 48×38) : les précédents échecs résultaient surtout de la limite artificielle 24×32 ou de l'hypothèse fixe de deux couches. Plusieurs images montrent graphiquement le fouet. Cela n'établit **pas encore** leur ordre temporel, leur affectation à des états Z80 ni leur intégration SGDK. Les PNG sont une extraction de recherche et non une modification de la ROM Mega Drive.

## Fichiers

- `extended-sheet.png` : aperçu global, zoomé sans interpolation.
- `extended-XX.png` : compositions positionnées sur cadre étendu sans rognage.
- `extended-12-upper-unpositioned-02.png` à `-05.png` : quatre couches de l'entrée 12, sans coordonnées attestées.
- `extended-manifest.json` : adresses CPU, offsets fichier, nombre de couches, coordonnées signées et bornes par entrée.
- `tools/decode_extended_simon.py` : extraction reproductible et contrôles de format.

## Étape suivante

Tracer le code Z80 qui sélectionne l'index des motifs et les indices des tables de position pendant une attaque, pour déterminer la séquence authentique et lever l'ambiguïté de l'entrée 12.
