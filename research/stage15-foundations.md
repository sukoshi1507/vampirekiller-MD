# Vampire Killer MD — Stage 15 — extraction ROM MSX2

**Statut : ressources statiques vérifiées, stage NON jouable et NON raccordé.**

## Référence et chaîne de preuve

- ROM MSX2 authentifiée SHA-256 : `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`.
- `tools/build_stage15_foundations.py` décode les tables et fonds depuis cette ROM, sans simulation d'ennemis.
- `tools/verify_stage15_rom.py` recalcule indépendamment les tables et compare les **7 680 octets de tuiles** effectivement émis dans `sgdk/src/generated/stage15_foundations.c`.
- Détails d'offsets, palettes et empreintes par salle dans `research/stage15-foundations.json`.

## Résultats extraits

- **10 salles** de 32 × 24 tuiles statiques (768 octets/salle), leurs tables de couleurs RGB333 et les empreintes des graphiques.
- **Navigation**, pointeur CPU `BADD`, table Z80 `B9D3` (banque 13), nibbles haut/bas/gauche/droite, `F` signifiant absence de lien.
- **Drapeaux de canaux d'apparition**, pointeur CPU `863E`, données fichier `0x1C63E` : `00 80 80 80 00 00 00 80 00 00` (salles 0 à 9). Les drapeaux ne précisent ni les types, ni le nombre d'ennemis.
- **Porte de fin**, entrée fichier `0x1BB8E`, octets `88 80 0C` : salle 8, Y=128, X=12, drapeaux bruts `88h`. Cela ne prouve pas les règles de clé ni d'ouverture.
- **Groupe de stage**, octet à `0x1E79`, valeur 4.

### Navigation brute

| Salle | Haut | Bas | Gauche | Droite |
|---:|---:|---:|---:|---:|
| 0 | 2 | — | 1 | — |
| 1 | — | — | — | 0 |
| 2 | 4 | 0 | 3 | 3 |
| 3 | — | — | 2 | 2 |
| 4 | 7 | 2 | 5 | 5 |
| 5 | — | 3 | 4 | 4 |
| 6 | — | — | 7 | — |
| 7 | — | 4 | 8 | 6 |
| 8 | — | 5 | 9 | 7 |
| 9 | — | — | — | — |

**Asymétries conservées sans modification** : 5 bas → 3 (3 haut absent), 8 bas → 5 (5 haut absent), 8 gauche → 9 (9 droite absent).

## Portage effectué

Les constantes ROM, les 10 fonds de salles statiques, les liens et drapeaux sont écrits dans `sgdk/src/generated/stage15_foundations.h/.c`. Aucun appel depuis `gameplay.c`, `room_manager.c` ou la progression : les données ne rendent pas le stage jouable.

Tests ajoutés : `tests/test_stage15_foundations.c` et `tests/run_stage15_foundations.sh` ; contrôle des dimensions, drapeaux, bornes, asymétries, porte. Vérification indépendante des données C contre la ROM par le script Python susmentionné.

## Portage non réalisé (aucune déduction autorisée)

- Exécution Z80 de la routine `4F83` pour les collisions dynamiques des 10 salles.
- Exécution des routines `5A49`/`5B1B` pour bougies, blocs, coffres, clés et objets.
- Exécution de `6154` pour les ennemis fixes ; identification/traçage des ennemis à apparition conditionnelle et de leurs comportements.
- Événements spéciaux, projectile(s), éventuel boss, son, animations et validation image par image sur MSX2.
- Intégration effective au moteur SGDK, compilation et validation en émulateur Mega Drive.

La dépendance d'émulation Z80 pour les routines dynamiques n'est pas présente dans le projet livré : les valeurs manquantes restent *non extraites* et ne sont pas fabriquées.
