# Vampire Killer MD — Stage 14, extraction vérifiée de la ROM MSX2

**Statut : fondations statiques, non jouable, non raccordé.** Aucun ennemi, coffre, bougie, clé ou collision dynamique du stage 14 n'est déclaré porté.

## Source de vérité et méthode

ROM MSX2 de référence contrôlée par `tools/probe.py` avant extraction ; SHA-256 :
`99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`.
La ROM de référence n'est pas incluse dans cette livraison. Le fichier `research/stage14-foundations.json` consigne les offsets, métadonnées, SHA-256 des salles et valeurs extraites.

- 8 salles, chacune avec 768 identifiants de tuiles et palette RGB333, générées par `tools/extract_rooms.py` et `tools/build_stage14_foundations.py` ; fonds statiques seulement, **pas** des cartes de collisions.
- Navigation depuis la table Z80 `B9D3` (banque 13), pointeur stage 14 `0xBACD` ; sens des nibbles décrit dans `tools/decode_room_links.py`.
- Drapeaux d'apparition : table de pointeurs fichier `0x1C5C2`, pointeur CPU `0x8636`, données fichier `0x1C636`. Les huit valeurs sont `0x80` ; elles ne décrivent pas le type ou le nombre des ennemis.
- Porte de sortie : table d'offset fichier `0x1BB61 + stage*3`, entrée stage 14 à `0x1BB8B` = `87 80 0C`. Donnée brute : salle 7, Y=128, X=12 ; **ni clé, ni conditions d'ouverture démontrées**.
- Groupe du stage : octet fichier `0x1E6A + 14` (valeur brute reproduite dans le manifeste).

## Navigation — octets ROM décodés

| Salle | Haut | Bas | Gauche | Droite |
|---:|---:|---:|---:|---:|
| 0 | 3 | — | — | — |
| 1 | — | — | 2 | — |
| 2 | — | 5 | — | 1 |
| 3 | — | 0 | 4 | — |
| 4 | — | 1 | 5 | 3 |
| 5 | — | — | 6 | 4 |
| 6 | — | — | 7 | 5 |
| 7 | — | — | — | 6 |

**Deux liens asymétriques, conservés tels quels** : salle 2 bas → 5, mais salle 5 haut → aucun ; salle 4 bas → 1, mais salle 1 haut → aucun. Aucune correction inventée.

## Code et validations livrés

- `sgdk/src/generated/stage14_foundations.c/.h` : 8 fonds statiques, liens bruts, drapeaux et constantes brutes de porte. Aucune activation dans `gameplay.c` ou `room_manager.c`.
- `tools/build_stage14_foundations.py` : extraction reproductible à partir de la ROM authentifiée.
- `tools/verify_stage14_rom.py` : recharge la ROM, compare le manifeste, les 8 × 768 octets émis en C, les liens, drapeaux, la porte et les empreintes.
- `tests/test_stage14_foundations.c` / `tests/run_stage14_foundations.sh` : contrôle natif de la taille, des valeurs, des asymétries et de l'entrée de porte. **Ne teste pas le gameplay.**
- `LIVRAISON/STAGE14_ETAT_ET_CHANGEMENTS.md` : journal détaillé et limites.

## Blocages factuels

Les routines originales Z80 `4F83` (collisions), `5A49/5B1B` (objets) et `6154` (ennemis fixes) n'ont pas été exécutées pour le stage 14. La dépendance `z80` nécessaire à `tools/castle_room_objects.py` est absente de l'environnement. Sans la sortie de ces routines ou des traces openMSX équivalentes, le portage complet des bougies, coffres, clés, ennemis et de leurs comportements ne peut être certifié, et ces données n'ont pas été fabriquées.

**Non réalisé** : compilation SGDK/ROM Mega Drive, test graphique/sonore, activation et liaison à la progression. Le stage 14 demeure indépendant.
