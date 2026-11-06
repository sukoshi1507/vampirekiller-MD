# Stage 12 — fondations ROM MSX2 (livraison du 6 novembre 2026)

## Portée et statut

**État : préparation technique réelle, stage 12 NON jouable.** Cette livraison ne prétend pas
avoir porté les ennemis, les portes, le boss, les objets, les collisions dynamiques ni les
passages secrets en jeu. La sortie du stage 11 garde son comportement de sécurité (retour au
titre) : elle n'a pas été forcée vers un stage dont les tableaux runtime restent sur 11 niveaux.

Méthode : ROM d'origine vérifiée SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2` ; aucune donnée reconstituée
par analogie avec un autre épisode ou un autre stage. Le code et les tests sont ajoutés à la
base cumulative du stage 11.

## Éléments extraits, précis et reproductibles

- **12 salles** : 12 × 768 octets de cartes **statiques** extraites de la ROM MSX2 en
  décodant les pointeurs, métatuiles et graphismes de `tools/extract_rooms.py`.
  Fichiers : `sgdk/src/generated/stage12_foundations.c/.h` ; ces 12 salles figuraient déjà
  dans le visualiseur statique `generated/rooms.c`, sans gameplay stage 12.
- **Navigation** : 12 entrées de table ROM, pointeur `0xBA9D` (banque 13,
  table de pointeurs `B9D3`). Table conservée telle quelle, même lorsqu'une liaison est
  à sens unique. Les liens ne prouvent pas que chaque franchissement est praticable à pied.
- **Passages secrets** : 10 entrées de 4 octets de la table ROM `BBCD` (banque 13) ;
  sources/destinations, coordonnées et offsets exacts ci-dessous. Leur machine d'état
  `85F4` / `70FB` / `B98C` est documentée dans `research/enemy-behavior.md`,
  **pas encore portée ni testée en gameplay**.
- **Entrée stage 12** : X=16, Y=176, direction droite, **issue de la trace KD**
  consignée dans `research/stage11-level.md` ; pas une nouvelle capture openMSX.

### Liaisons : tables ROM brutes

| Salle | Haut | Bas | Gauche | Droite |
|---:|---:|---:|---:|---:|
| 0 | — | — | 2 | 1 |
| 1 | — | — | 0 | 2 |
| 2 | — | — | 1 | 0 |
| 3 | — | — | — | 4 |
| 4 | — | — | 3 | 5 |
| 5 | — | — | 4 | 6 |
| 6 | — | — | — | — |
| 7 | — | — | 8 | 8 |
| 8 | — | — | 7 | 7 |
| 9 | — | — | 11 | 10 |
| 10 | — | — | 9 | 11 |
| 11 | — | — | 10 | 9 |

**Particularité** : salle 5 → salle 6 à droite ; la salle 6 n'a aucun lien vers la
gauche. Cette asymétrie existe dans les octets ROM et ne doit pas être normalisée.

### Passages secrets — table BBCD

| Salle | Destination | X | Y | Offset ROM |
|---:|---:|---:|---:|---|
| 0 | 3 | 88 | 168 | `0x1BBCD` |
| 1 | 4 | 184 | 136 | `0x1BBD1` |
| 2 | 11 | 152 | 168 | `0x1BBD5` |
| 3 | 0 | 88 | 168 | `0x1BBD9` |
| 4 | 1 | 184 | 136 | `0x1BBDD` |
| 5 | 8 | 88 | 168 | `0x1BBE1` |
| 7 | 10 | 56 | 168 | `0x1BBE5` |
| 8 | 5 | 88 | 168 | `0x1BBE9` |
| 10 | 7 | 56 | 168 | `0x1BBED` |
| 11 | 2 | 152 | 168 | `0x1BBF1` |

## Modifications de cette livraison

1. **Ajout** `tools/build_stage12_foundations.py` : extraction reproductible, validation
   du SHA-256 par `probe.load_rom`, vérification des 12 cartes, lecture des liens et
   des 10 records des passages secrets. Nécessite la ROM fournie localement par son
   propriétaire, non redistribuée.
2. **Ajout** `sgdk/src/generated/stage12_foundations.c/.h` : tableaux C des tuiles
   et des connexions ROM et coordonnées de passage ; aucune activation du gameplay.
3. **Ajout** `research/stage12-foundations.json` : inventaire machine-lisible avec
   offset et SHA-256 de chaque carte et graphisme.
4. **Ajout** `tests/test_stage12_foundations.c` et `tests/run_stage12_foundations.sh` :
   test natif des dimensions, limites, asymétrie 5→6, et dix coordonnées de passage.
5. **Ajout** ce journal de portage et les journaux de validation dans `LIVRAISON/`.

## Validations exécutées

- `sh tests/run_stage12_foundations.sh` : **PASS** (structure et constantes des
  tableaux déjà générés ; ne remplace PAS un replay Z80).
- `sh tests/run_native.sh` : **PASS**, tests cumulés jusqu'au stage 11.
- `sh tests/audit_run_native.sh` : **PASS**, tests cumulés jusqu'au stage 11.
- Le décodeur de fonds ROM a reconstruit les 12 cartes sans identifiant de tuile
  inconnu. Les valeurs des cartes stage 11 ont été comparées ponctuellement au
  généré `4F83` (salles 0 et 3), résultat identique, mais **aucun replay 4F83
  n'a été effectué pour le stage 12**.

## Bloquants pour rendre le stage 12 jouable

1. Exécuter `4F83` sur les 12 salles avec le module Python `z80==1.2.0`, comparer
   chaque octet des cartes et obtenir les règles de collision du stage 12.
2. Exécuter `5A49` / `5B1B` pour les bougies, coffres, blocs, clés et portes.
   Mettre à jour les tableaux et les limites de tableaux runtime (actuellement
   stage 0–11 / 1–11, neuf salles maximum par stage dans plusieurs structures).
3. Exécuter `6154` pour les ennemis fixes et relever les canaux de spawn ; porter
   les types et comportements réellement présents, sans substitution.
4. Porter l'automate du passage secret `85F4` / `70FB` / `B98C` et ses délais,
   comparer avec des traces openMSX propres au stage 12.
5. Identifier et tracer le boss du stage 12 et ses attaques, les musiques et SFX,
   la clé, la porte et l'enchaînement vers le stage 13.
6. Ensuite seulement, raccorder gameplay, visualisation et sortie du stage 11 ;
   compiler SGDK, tester les 12 salles dans l'émulateur Mega Drive, comparer les
   événements image par image avec openMSX.

**Contrainte rencontrée** : le module Z80 n'est pas installé dans cet environnement
et son installation réseau n'est pas accessible. Il serait incorrect de déclarer
`4F83`, `6154` ou les objets validés sans leur exécution. Aucune ROM d'origine
n'est fournie dans cette livraison.

## Ajout de consolidation : drapeaux ROM et planches de contrôle

- La table de pointeurs de **drapeaux de canaux d'apparition** (banque 14,
  pointeur en offset fichier `0x1C5BE`, valeur CPU `0x861E`, données à
  `0x1C61E`) fournit exactement **12 octets nuls** pour les douze salles.
  Ces octets ne représentent **pas** la liste d'ennemis fixes ; une liste fixe
  non nulle reste possible et doit être mesurée par exécution de `6154`.
- Le sélecteur de groupe de stage conservé en RAM `D002` provient de la table
  ROM à l'offset `0x1E76`, valeur **03h**. Sa présence ne valide pas
  l'exécution de `5A49`, `5B1B` ou `6154`.
- `vk_stage12_raw_spawn_flags` est ajouté au module généré
  `stage12_foundations.c/.h`, toujours **non raccordé au moteur**.
- `tools/verify_stage12_rom.py` compare, avec le SHA-256 de la ROM vérifié,
  les **12 cartes**, les liens, les **10 passages**, les **12 drapeaux** et
  l'octet `D002` aux inventaires. Aucun code Z80 n'est exécuté.
- `research/stage12-previews/` contient 12 PNG et une planche des fonds
  **statiques** décodés directement des tuiles et palettes ROM ; ces images
  ne montrent ni Simon, ni ennemis, ni objets, ni collisions runtime.

### Validation et limites

Les tests natifs de la base préexistante ont réussi après cet ajout, ainsi que
`tests/run_stage12_foundations.sh` et le nouveau vérificateur ROM indépendant.
Le long audit optionnel n'a pas été achevé dans cette session (limite de temps).
Le stage 12 reste **non jouable** : ne pas activer la sortie du stage 11 tant
que les routines d'objets, les collisions et les ennemis fixes n'ont pas été
exécutés et vérifiés. Le module Python `z80` requis est absent de ce système.
