# V7 — Atlas de compositions MSX2 pour l'animation de Simon

Le projet principal SGDK reste inchangé. Cette livraison prolonge la rétro-ingénierie des sélecteurs C42E (bas) et C42F (haut) grâce à un catalogue de 64 compositions croisées directement décodées depuis la ROM de référence.

## Contenu
- `tools/build_simon_crosspairs.py` : extraction reproductible, SHA-256 de la ROM contrôlé par `probe.load_rom`.
- `research/simon-assets/crosspairs/crosspair-sheet.png` : planche 8 × 8 indexée, colonnes = haut, lignes = bas.
- `research/simon-assets/crosspairs/lower-XX_upper-YY.png` : 64 images transparentes.
- `research/simon-assets/crosspairs/crosspair-manifest.json` : indices, dimensions, ancrage relatif, nombre de couches non positionnables et statut.

Le rendu des couches utilise les tables de motifs A281/A2D1 et les coordonnées 7985/79D5; il compose les couches selon leurs décalages signés. 56 combinaisons sont entièrement positionnées et 8 sont partielles. Une image « complète » n'implique **pas** que la combinaison soit réellement utilisée par le jeu : ce n'est qu'une proposition graphique dont les sources sont réelles.

## À ne pas inférer
- Les combinaisons haut 6, 9 et 12 sont des **candidats** intéressant la séquence d'attaque; le code Z80 y affecte C42F mais la chronologie de fouet n'a pas été tracée.
- Les durées, l'ordre des frames, la visibilité/masquage de certains éléments et la priorité d'affichage restent inconnus.
- Les combinaisons ne doivent pas être injectées automatiquement dans la ROM SGDK comme animations authentiques.

## Vérifications
- Extraction : 64 PNG; 56 assemblages entièrement positionnés; 8 partiels.
- Suite `sh tests/run_native.sh` : PASS.
- Compilation SGDK et émulation BlastEm de cette livraison non exécutées (code SGDK inchangé).

## Reproduction
```sh
python3 tools/build_simon_crosspairs.py 'Vampire Killer (Japan, Europe) (En).zip'
```

## Suite logique
Capturer en exécution openMSX les valeurs de C42E/C42F à chaque frame pendant une attaque, puis utiliser le manifeste pour vérifier chaque paire réelle avant de générer les sprites SGDK.
