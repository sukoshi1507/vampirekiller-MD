# Objets muraux et marchand (référence initiale stages 0-2)

Extension LOT08 stages12..18, preuves exécutées et limites : [trade-late.md](trade-late.md).

Code ROM lu en banque 2 (`9179`-`954F`) ; prix et fenêtre confirmés par trace openMSX
(`scratchpad/objects-0F-10-11/trace_marchand_*`, hors dépôt). Port : `sgdk/src/wall.c`
(logique, testée par `tests/test_wall.c`), `sgdk/src/wall_view.c` (affichage).

## Objets muraux

- Enregistrements `C5B5` et `C5C5`. Créés par `9179`, soit au chargement de la salle
  (objet direct : `5BB7`), soit à la casse d'un bloc 0x1F dont `+9 & C0 = C0` (`8814`).
  Vieil homme dans le mur : image 32 x 32 copiée par `925E`.
- Blocs de ce type : (192,160) `E1` hall salle 7, (192,160) `CE` stage 2
  salle 0, (160,64) `DB` stage 2 salle 2, (160,96) `DC` stage 3 salle 2. Objet direct :
  (160,32) `D5` stage 2 salle 0.
- Coup de fouet (`80E5` -> `83A1`, `83BC`), arme lancée (`80DC`) ou croix 05 (`928D`,
  `+3` = FF). Animation de 20h passes (`9229`), puis résultat `C70C` (`92BB`, table `9300`
  selon la ligne `entry & 3` et le nombre de coups, table `9320` pour l'image).
- Résultats (`92A7`) : marchand (`9327`), `D012` ± 1 (`9333`, `933C` : vitesse d'apparition,
  `9CD8`), cœurs ± 5 (`9344`, `934E`), rien (`9343`), disparition définitive avec 5000 points
  (`9358`).
- Compteurs de coups et objets vendus : `DE00` (`5AAF`, `944F`), remis à zéro seulement par
  le décodage du flux du stage (`5A49` : nouvelle vie, début de stage).

## Marchand (état 0C, `43F0`)

1. `9327` : `C40C` = FF, `C703` = enregistrement. État 0C à la passe suivante ; le jeu est figé.
2. `9387` -> `93FF` : `C708` = objet vendu, `C706` = 14h, son 19, fenêtre `9397` 80 x 32
   (X = `+2` − 10h, ou − 20h à partir de X = 80h ; Y = `+1` − 18h). La zone recouverte est
   sauvée par `4946`.
3. `94BA` : `C706` − 1 quand `C003 & 1F` = 0 ; à 0, la réponse est non. Oui : cœurs − prix,
   objet donné (`8D30`), son 12. Non, ou cœurs insuffisants : son 02.
4. `43FD` : 15 passes, puis fenêtre restaurée (`9507`) et état 5.

Prix (`9428`, cœurs) : voir `research/castle-candles.md`, objets 10 et 11. 11 baisse les prix,
10 les augmente.

## Choix du port

- Touches : B = oui, C = non.
- La fenêtre est écrite aussitôt (jeu figé), les cellules recouvertes du plan A sont relues
  en VRAM puis restaurées à la fermeture.
- Hasard `92E8` (résultats 7, 8, 9 de la table `9300`, paires 3-5, 4-5, 3-4) : bit 0 du registre R,
  1 = premier de la paire. Tracé (137 appels, 121 résultats relus, 0 exception) : `research/hasard-R.md`.
  Le port tire `Enemies_randomR() & 1` (plus de compteur d'images).
- `tests/md_scenarios/zo_merchant.txt` : achat de l'objet 04 au stage 2 salle 0, fenêtre visible.
