# V15 — Flammes de test dans la cour

Base: V14 validée par l'utilisateur (fouet, cible du hall, huit salles, escaliers).

## Test BlastEm

1. Passer l'introduction et entrer dans la cour (sans entrer dans le château).
2. Une petite flamme animée de deux images doit apparaître à environ 48 pixels devant le point d'apparition de Simon.
3. Appuyer sur **B** à distance de fouet : l'animation d'attaque existante joue, la flamme affiche un court scintillement d'impact, puis disparaît, et `FLAMME ETEINTE!` apparaît.
4. Appuyer sur **A+B** pour rallumer la flamme; cela ne lance pas d'attaque involontaire.
5. Tester les changements d'écran de la cour, puis l'entrée du château. La cible d'entraînement du hall et les escaliers doivent continuer à fonctionner.

## Choix et limites

- Cette flamme est un **sprite-test dessiné pour le portage**; elle ne remplace pas les flammes MSX2 existantes éventuellement présentes dans les fonds du décor. Elle est placée relativement à Simon pour être accessible immédiatement, même en début de cour.
- Les deux frames sont des graphismes Mega Drive générés directement en tiles 4 bpp et alternent toutes les 8 boucles de jeu; l'effet d'impact dure 12 boucles.
- Le moteur de collision du fouet et les ressources d'attaque MSX2 restent ceux de V14. Cette version ne corrige pas l'incomplétude des frames originales d'attaque.
- Les flammes reviennent lors du chargement d'une nouvelle scène de cour. Le hall conserve sa cible géométrique.
- Le chargement VRAM ajoute 8 tiles après les 4 tiles de cible précédentes. La palette PAL3 devient orange/blanc pour accentuer la flamme; cela modifie aussi la couleur des segments provisoires du fouet.

## Validation locale

Tests natifs C et tests Python exécutés. Compilation SGDK et BlastEm non disponibles ici : l'affichage et la synchronisation exacte demandent validation sur émulateur.
