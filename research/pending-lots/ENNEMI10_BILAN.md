# Ennemi 10h — livraison intermédiaire 3 (non terminé)

Base unique : `vampirekiller-MD_base_2026-11-07.zip`, commit `8feb0c9`.
Les contrôles des deux livraisons précédentes sont inclus.

## Prouvé par les octets de la ROM (analyse statique)

- `9942h` type `10h` pointe vers `ADF8h`.
- Le chemin de `AEAFh` à `AEDCh` vérifie 18 instructions. Sous ses
  conditions, `JP 9F61h` reçoit `HL=0000h`, `DE=0400h/FC00h`, et
  le pointeur IX du lanceur a été sauvegardé en `CFFCh`.
- `AF40h` calcule la distance horizontale absolue entre `C427h` et
  `(IX+05)`. Le test de tir `AEB8h` requiert distance **< 2Ah**, soit
  **0 à 41 pixels** (65 536 paires 8 bits contrôlées).
- `9F61h` transmet `BC=(X, Y-16)`, sauvegarde l'identifiant source,
  `HL` et `DE`, puis appelle `9F83h`.
- `9F83h` inspecte au maximum huit emplacements (`D700h`, stride `80h`)
  et retourne si aucun premier octet libre (zéro) n'est trouvé.
- Tests : `tests/run_enemy10_rom.sh`, `tests/run_enemy10_fire_path.sh`,
  `tests/run_enemy10_helpers.sh` et `bash tests/run_native.sh` : PASS.

## Provisoire / non prouvé

- La signification dans le jeu de l'identifiant obtenu via `A08Eh/4061h`.
- L'écriture des champs après `9FAAh`, les dimensions, sprites, vitesses
  appliquées et collisions ; leur lecture statique reste à faire.
- L'initialisation, la boucle de mise à jour complète `ADF8h`, ses
  transitions et sa cadence ; contexte de jeu non tracé.
- **Aucun modèle C n'est introduit, aucun raccord `enemies.c`, aucune
  modification de `EnemyState`.** Pas de comparaison image par image.
- Module Python `z80` indisponible ici. Les tests ci-dessus n'exécutent
  pas de code Z80 et ne sauraient remplacer le test natif sur trace Z80.

## Suite

Désassembler l'initialisation après `9FAAh`, caractériser `ADF8h` et son
état initial, obtenir une trace Z80 de plusieurs images, puis ajouter le
module C isolé et la comparaison native avant raccordement.

Références : `research/enemy10-fire-path.md` et
`research/enemy10-helpers.md`.
