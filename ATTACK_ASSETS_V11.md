# V11 — Compositions ROM MSX2 utilisées pour l'attaque (séquence expérimentale)

## 1. Liaison documentée aux états Z80

Le code Z80 original utilise les index **C42E (bas)** et **C42F (haut)**, liés respectivement aux tables **A281** et **A2D1**. La logique statique observe l'usage des valeurs hautes 6 et 9, mais ne prouve **pas** qu'elles sont successives lors d'un fouet. Voir `research/simon-assets/ANIMATION_SEQUENCES.md`. Le changement d'orientation est associé à une correction des deux index; les candidates de cette version sont rendues par miroir horizontal sur Mega Drive, pas par une seconde extraction gauche/droite du jeu original.

## 2. Candidats graphiques et choix de l'ordre

| Phase SGDK (`vk_whip_arm_frame`) | Couple ROM bas/haut | Confiance |
|---|---|---|
| 1 — préparation | 0/0 | graphiques sourcés; affectation expérimentale |
| 2 — extension courte | 0/6 | graphiques sourcés; affectation expérimentale |
| 3 — extension complète | 0/9 | graphiques sourcés; affectation expérimentale |
| 4 — récupération | 0/0 | graphiques sourcés; affectation expérimentale |

Les **quatre dessins** proviennent réellement de la ROM (trois compositions distinctes); **l'ordre et la durée de chaque phase sont une hypothèse de portage**, pas une preuve d'authenticité Z80. Les collisions et la durée du fouet restent celles de la V10.

## 3. Conversion Mega Drive

Générateur : `tools/build_simon_attack_md.py`. Il utilise les masques RLE, les coordonnées signées par couche et la palette de la première salle de la ROM. Chaque cadre 64×32 est aligné sur `(Simon.x - 32, Simon.y - 30)`, et découpé en **deux sprites 32×32** (16 tiles chacun) ; chaque phase produit 32 tiles en 4bpp. Les couches 0,1,2,3 utilisent respectivement les index de palette 0,1,2,3. Les graphismes sont dans `sgdk/src/generated/simon_attack_md.c/.h` et les previews `attack-candidate-*.png` sont exportées à côté.

Commande depuis la racine :

```sh
python tools/build_simon_attack_md.py '/chemin/vers/Vampire Killer (Japan, Europe) (En).rom' --output sgdk/src/generated
```

Le rendu V11 affiche les compositions ROM à la place du personnage et des sprites provisoires **uniquement pendant l'attaque**. Il utilise deux sprites SGDK supplémentaires (slots 5/6), avec chainage 0→1→2→3→4→5→6→0. L'orientation droite emploie le miroir horizontal et inverse les moitiés. La palette PAL2 est chargée par la scène comme en V10. Les ressources originales de la V10 sont conservées pour les séquences hors attaque.

**Important :** le fouet peut sembler incomplet : les index supérieurs 6 et 9 sont des candidats de posture, et non une reconstruction démontrée de la totalité des segments de fouet. Le rendu est destiné à tester la qualité du décodage, pas la fidélité définitive des animations.

## 4. Vérifications et consignes de régression

- `python -m unittest discover -s tests -p 'test_*.py' -v`
- `gcc -std=c99 -Wall -Wextra -Werror -fsyntax-only sgdk/src/generated/simon_attack_md.c`
- Compiler avec le SGDK existant : `cd sgdk && make GDK=/chemin/vers/sgdk`
- Dans BlastEm : entrer dans le château, attaquer avec B à droite puis à gauche, vérifier que la composition reste alignée et que les phases F1–F4 s'affichent ; contrôler A+HAUT et HAUT/BAS en salle 1 pour la non-régression des escaliers.

**Tests SGDK/BlastEm et capture openMSX non exécutés dans cet environnement.** Le script de trace de la V10 reste disponible pour remplacer la séquence expérimentale par les paires et temporisations réellement observées.
