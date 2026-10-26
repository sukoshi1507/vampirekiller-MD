# VampireKiller V27 — Gestionnaire de salles, stage 1

## Périmètre

- Nouveau `sgdk/src/room_manager.c` et `.h`, indépendants de SGDK.
- Graphe de test explicite pour les 8 salles extraites du stage 1.
- Transition bord droit/gauche, conservation de la hauteur, orientation,
  phase de saut et état du personnage. Pas de passage pendant les escaliers.
- Chargement du décor, des palettes, des sprites Simon et du fouet au changement de salle.
- Raccourcis de sélection `A + gauche/droite` conservés.
- Cour, escaliers de test, fouet MSX2, accroupissement et flammes inchangés.

## Distinction entre données connues et hypothèses

Les décors sont extraits de la ROM MSX2. La première salle du stage 1 possède
une carte de collision complète vérifiée. Les 7 suivantes ont 704 tuiles
visibles reconstruites et 2 lignes cachées non authentifiées.

**Les connexions 0 ↔ 1 ↔ ... ↔ 7 sont uniquement un graphe expérimental**,
non un tracé confirmé des portes et escaliers originaux. Les liaisons
verticales authentiques et les conditions d'accès nécessitent une nouvelle
analyse Z80. Le gestionnaire isole cette future correction des autres modules.

## Commandes de test BlastEm

Entrer dans le château ; franchir le bord droit de la salle pour avancer,
ou le bord gauche pour reculer. `A + gauche/droite` permet de rejoindre
une autre salle même si les collisions rendent son bord inaccessible.
Dans la salle 1, `A + haut` active toujours l'escalier expérimental.

## Vérifications effectuées

- `sh tests/run_native.sh` : OK (y compris test room manager).
- `python3 -m unittest discover -s tests -p 'test_*.py' -q` : 14 tests OK.

Compilation SGDK et comportement BlastEm de cette version à vérifier sur
l'environnement cible. Aucun tracé original des passages n'est revendiqué.
