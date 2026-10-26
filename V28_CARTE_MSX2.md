# VampireKiller V28 — Cartographie ROM et audit des passages

Cette version conserve **strictement le moteur SGDK V27** et toutes ses commandes.

## Ajouts

- `tools/audit_castle_navigation.py` relit la ROM MSX2 et construit un inventaire de **153 salles, 18 stages**.
- `research/castle-navigation/navigation-audit.json` : catalogue vérifiable, provenance des cartes, statut des fonds et statut de vérification des connexions.
- `research/castle-navigation/NAVIGATION_AUDIT.md` : bilan par stage et sondes de collisions des bords du stage 1.
- `tests/test_castle_navigation_audit.py` : trois tests de cohérence et anti-fausse-identification.

## Résultat

Le stage 1 contient huit salles. Les règles géométriques confirment qu'il serait incorrect de traiter tous les bords comme des passages MSX2. L'exemple le plus clair : bord gauche de la salle 4, bloqué sur toute la plage verticale échantillonnée. Les déplacements séquentiels V27 restent volontairement **des transitions de démonstration**.

## Limites

Les conditions d'ouverture des portes, la topologie exacte, les escaliers originaux, et les drapeaux de progression nécessitent une recherche du dispatch Z80 ou une capture dynamique. Cet audit ne les invente pas.

## Vérifications

- Exécution de l'extracteur avec la ROM de référence : OK.
- Tests Python : OK.
- Tests C natifs : OK.
- Compilation SGDK et BlastEm pour V28 : non effectuées ici (moteur non modifié).

Reproduction :

`python3 tools/audit_castle_navigation.py "Vampire Killer (Japan, Europe) (En).zip"`
