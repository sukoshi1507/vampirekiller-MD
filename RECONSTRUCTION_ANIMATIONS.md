# Reconstruction / interactions — version expérimentale

## ROM originale vérifiée

`tools/audit_simon_frames.py` utilise les tables de motifs et de positions déjà reconnues par `tools/extract_player.py` pour extraire un catalogue *exploratoire* de paires de frames du binaire MSX2. Voir `research/simon-frame-audit/candidate-frames.png` et `catalog.json`. **Une paire de frames décodable n’identifie ni un état d’accroupissement, ni une montée d’escalier, ni un fouet.** Les associations exigent l’étude des tables d’animation et des branchements du Z80. Ces candidats ne sont pas injectés en jeu.

## Moteur SGDK

- Sur l’escalier de démonstration, arrêt à l’une des extrémités sans retour intempestif à la physique gravitationnelle.
- Sans touche verticale, pose de repos orientée selon le dernier sens de déplacement.
- HAUT/BAS animent les poses de marche extraites de la ROM, sans utiliser les poses de saut.
- Le saut quitte toujours le mode escalier ; après le saut les collisions classiques reprennent.
- Le fouet B reste un prototype à sprites géométriques ; les nouveaux candidats ROM ne sont **pas** annoncés comme animations de fouet d’origine.

## Limites

L’escalier du stage 1 / salle 1 reste une zone de **test** placée par A+HAUT. Les coordonnées, l’orientation des vrais escaliers et les associations des animations originales n’ont pas encore été authentifiées par une trace d’exécution MSX2. La compilation SGDK et l’exécution BlastEm doivent être validées par l’utilisateur.

## Vérifications

`sh tests/run_native.sh` : tests C de collisions, physique, déplacement, changement de salle et fouet.
