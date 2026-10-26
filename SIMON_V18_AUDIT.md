# V18 — Audit fonctionnel de Simon

Base : V17 validée par l'utilisateur (SGDK/BlastEm). Aucun changement de trajectoire, de collision de salle, de progression d'escalier ni de cinématique.

## Correctifs intégrés

- BAS sur terrain stable : la physique disposait déjà du booléen `crouched`, mais le rendu choisissait toujours une pose debout. Le renderer utilise désormais un sprite 32×32 dédié, positionné au niveau des pieds et retourné vers la gauche lorsque nécessaire.
- Asset : `extended-20.png` (23×17), véritable image reconstruite des tables de la ROM MSX2 ; **l'identification comme accroupissement reste une hypothèse visuelle**, non une preuve dynamique. Génération reproductible par `tools/build_simon_crouch_md.py`, fichiers `simon_crouch_md.c/.h` et manifeste.
- Attaque accroupie : le déclenchement du fouet conserve les hauteurs d'impact adaptées à la position basse, sans modifier les fenêtres temporelles, et verrouille la hauteur au moment de l'attaque. Pendant l'accroupissement, le corps conserve sa silhouette basse ; les compositions d'attaque debout sont masquées afin d'éviter la téléportation visuelle.
- Les cibles et flammes restent dans la cour et le hall, et toutes les mécaniques V17 sont conservées.

## Matrice de comportement

| Contrôle / état | Physique | Visuel | Authenticité MSX2 |
|---|---|---|---|
| Repos debout | Oui | Pose existante | Extraction originale disponible |
| Marche droite/gauche | Oui | 3 poses par sens | Assemblages existants |
| Saut/chute | Oui | Pose de saut | Approximation de la cadence |
| BAS, accroupissement | Oui (préexistant) | Nouveau candidat ROM V18 | Fonction de la pose non confirmée |
| BAS + B, fouet bas | Oui V18 | Silhouette basse + segments | Chronologie non confirmée |
| B, fouet debout | Oui | 8 compositions candidates V17 | Ordre non confirmé |
| Escalier | Oui (démonstration) | Poses de marche V3 | Coordonnées et sprites non authentifiés |
| Dégâts/recul/mort | Non | Non | À extraire / porter |
| Objets secondaires/ramassage | Non | Non | À extraire / porter |

## À ne pas confondre

« Le sprite provient de la ROM » n'implique pas « la pose est la bonne au bon instant ». Le portage n'a toujours pas de chronologie d'attaque attestée par openMSX, ni de vraie physique d'escaliers localisés sur les salles MSX2. La V18 ne prétend pas rendre Simon complet : elle répare une lacune visible, produit un audit et maintient la base fonctionnelle.

## Essais BlastEm recommandés

1. Dans la cour, BAS : silhouette visiblement basse, sans déplacement. Relâcher BAS : retour debout.
2. BAS + B : fouet à hauteur réduite, tester les deux directions. Approcher une flamme : les impacts sont désormais plus bas et ne doivent pas nécessairement toucher une flamme située haut.
3. B debout à droite/gauche : comparer aux séquences V17 (pas de changement d'ordre), puis cour → hall → escalier V3.
4. A+B rallume toujours les flammes.

Validation exécutée ici : `bash tests/run_native.sh` et `python -m unittest discover -s tests -q`. Compilation SGDK et exécution BlastEm de V18 non exécutées.
