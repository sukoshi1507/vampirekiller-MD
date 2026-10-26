# V6 — Diagnostic d'affichage du fouet dans BlastEm

## Symptôme
La touche B est reconnue et le message « B: ATTAQUE DETECTEE » apparaît, mais aucune animation n'est visible.

## Modification
- `sgdk/src/main.c`: remplacement des appels `VDP_setSprite()` pour Simon, les 3 segments du fouet et le bras par `VDP_setSpriteFull()` avec une chaîne explicite d'indices 0→1→2→3→4→0. Cette chaîne suit la méthode employée dans l'introduction du projet.
- Indicateur de phase `F0` à `F4` en haut à droite de l'écran pendant le jeu dans le château ; `F0` signifie aucune attaque, `F1` préparation, `F2` extension courte, `F3` extension complète, `F4` récupération.
- Aucune modification de la physique et des escaliers V3.

## Vérification dans BlastEm
1. Entrer dans le château, hors escalier, puis appuyer une fois sur B.
2. Vérifier que l'indicateur passe brièvement de `F1` à `F2`, puis `F3`, `F4` et `F0`.
3. Vérifier l'apparition des segments et du bras, face à droite et face à gauche.
4. Si les phases changent mais pas les sprites, suspecter encore les attributs / chargements VRAM ou un conflit du rendu matériel, et fournir une capture ainsi que le résultat F0–F4.
5. Si F1–F4 ne s'affichent pas, chercher un problème dans la branche `vk_whip_tick`, non dans la chaîne matérielle des sprites.

## Vérifications effectuées
- `bash tests/run_native.sh`: PASS.
- Aucune compilation SGDK ni test BlastEm réalisés dans cet environnement.

Les sprites de bras et de fouet restent provisoires, et non extraits de la ROM MSX2.
