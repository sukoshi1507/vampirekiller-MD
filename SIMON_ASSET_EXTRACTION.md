# Inventaire des graphismes de Simon — extraction vérifiée

## Résultat

ROM source : *Vampire Killer (Japan, Europe) (En).rom*, SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`.

Le nouvel outil `tools/export_simon_asset_catalog.py` examine les indices 0 à 23 des tables de motifs `A281/A2D1` et de coordonnées `7985/79D5`. Pour chaque indice, il rapporte les pointeurs CPU, décalages de fichiers, tailles de flux décompressés et nombre de composants déclaré par la table de placement.

10 paires sont décodables par le format **24 × 32, deux masques 16 × 16 par partie** : indices 0–5 et 15–18. Elles sont exportées dans `research/simon-assets/candidate-XX.png` avec transparence, un `catalog-sheet.png` et un `manifest.json` détaillé.

Les 14 autres indices ne sont **pas des erreurs de ROM**. Beaucoup ont des flux supérieurs de 128/192 octets (au lieu de 64), signe d'une composition différente. Pour les indices où les deux flux font 64 octets, la table de coordonnées ne correspond pas au schéma actuel. Ces entrées nécessitent le décodage de plusieurs composants/une nouvelle interprétation du placement, pas le réemploi aveugle du parseur 24 × 32.

**Aucune image nouvelle n'est étiquetée « attaque », « escaliers », « accroupi » sans preuve issue du dispatch Z80**. Certaines paires décodées existent déjà dans le moteur avec des combinaisons bas/haut différentes; l'outil d'inventaire ne change pas `sgdk/src/generated/player_assets.c` et ne modifie pas le gameplay testé.

## Régénération

```sh
python3 tools/export_simon_asset_catalog.py '/chemin/vers/Vampire Killer (Japan, Europe) (En).rom'
```

Dépendances : Python 3 + Pillow (mêmes dépendances que les scripts existants). La ROM originale n'est pas redistribuée dans le projet.

## Suite technique

1. Décoder les dispositions multi-composants (flux 128/192 octets) et leur table de placement.
2. Reconstituer le dispatch Z80 reliant état d'attaque/escalier/accroupissement aux indices bas/haut et aux séquences temporelles.
3. Une fois les rôles prouvés, exporter les vraies poses en tuiles 4bpp compatibles Mega Drive et remplacer les sprites temporaires.
4. Vérifier en émulation la position relative du corps, du bras et du fouet dans les deux directions.
