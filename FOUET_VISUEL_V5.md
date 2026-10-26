# V5 — Animation visible du bras pendant le fouet

Retour BlastEm V4 : touche B détectée mais Simon reste figé dans sa pose.

## Changement

Le rendu Mega Drive utilise désormais un cinquième sprite, dédié au bras animé de Simon, avec quatre keyframes : préparation (timer 14–11), extension courte (10–8), extension longue (7–3), récupération (2–1). La position et l’orientation sont verrouillées au déclenchement du fouet. Le rendu utilise une palette dédiée pour distinguer le bras et le gant. Les segments et hitboxes V4 sont conservés.

Ce bras est une approximation graphique 8x8, non une animation récupérée du MSX2 ; les poses corporelles de Simon ne changent pas encore.

## Tests BlastEm

Dans le château, presse B : un petit bras coloré doit apparaître près de Simon, se déployer horizontalement puis revenir. Vérifie les deux directions et les escaliers V3. Si seul le diagnostic B apparaît, vérifier la présence des sprites n°1–4 dans le débogueur VDP.

Tests natifs : `sh tests/run_native.sh`. Compilation SGDK et BlastEm à effectuer dans ton environnement.
