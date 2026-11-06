# Vérification globale des niveaux et transitions

Périmètre : les dix niveaux jouables du portage, soit 73 salles. Les niveaux
11–18 ne sont pas intégrés au moteur de jeu : leurs décors consultables ne
constituent pas huit niveaux supplémentaires validés.

## Corrections de cette version

1. Restauration de 4 414 cellules dans les deux rangées cachées de 72 salles.
   Ces rangées étaient remplies de zéros. Les 704 cellules visibles de chaque
   salle concordaient déjà avec l’original et sont conservées.
2. Remplacement du point d’apparition fixe du débogage par une recherche de sol
   avec dégagement du corps. Le point est choisi uniquement dans la salle demandée.
   Les entrées normales des niveaux conservent leurs coordonnées MSX2 et les
   transitions normales conservent leur placement et leur mouvement existants.
3. Une sélection de salle invalide ne modifie pas la partie courante.

Les corrections précédentes sont incluses : liaisons MSX2 des niveaux 4–10,
clés, portes à gauche/droite, positions d’entrée, sélection de salle jouable.

## Contrôles exécutés avec succès

| Contrôle | Résultat |
|---|---|
| Cartes complètes, routine Z80 4F83 | 73 × 768 = 56 064 cellules identiques |
| Seuils de solidité, table ROM 7C78 | 19 valeurs identiques |
| Liaisons, routine Z80 B99A | 292 directions identiques |
| Changements de salle aux frontières, moteur C | 146 passages dirigés testés ; absences de liaison testées aussi |
| Arrivées du mode test | 73/73 sans mur détecté aux côtés et sans chute/changement de salle pendant 120 images sans entrée |
| Escaliers, tests locaux de montée | 504 prises d’escalier exercées ; 144 franchissements verticaux |
| Clés et portes | Dix niveaux ; extraction ROM et tests de collecte/ouverture |
| Prédicat de contact des portes | 56 320 cas C/Z80 concordants |
| Approche et sortie des niveaux | Dix portes atteintes depuis une position de proximité, ouverture et sortie |
| Chargement du niveau suivant | 1→2 à 9→10, salle 0 et entrée correcte |
| Sélecteur de test | 73 couples valides ; indices invalides rejetés |

Tests natifs exécutés avec AddressSanitizer et UndefinedBehaviorSanitizer.
La détection de fuites est désactivée à cause du traceur de l’environnement.
Les sondes d’escaliers commencent à des positions locales prédéfinies : les
504 cas ne désignent pas 504 escaliers distincts et ne prouvent pas que tous
les escaliers sont accessibles depuis l’entrée du niveau. Les 144 passages
peuvent inclure plusieurs essais du même escalier.

## Limites restantes avant l’intégration des ennemis

- Aucun parcours complet et aucune compilation SGDK/émulation Kega Fusion
  effectués ici. Les tests utilisent le contrôleur C et les données du jeu,
  sans affichage, VDP, audio ni simulation des ennemis.
- Les octets des cartes et les seuils sont authentiques ; cela ne valide pas
  intégralement la physique, les collisions dynamiques, les escaliers et la
  précision du portage à chaque image.
- Le bon placement des clés/portes et la réussite d’une approche locale ne
  prouvent pas leur accessibilité de bout en bout depuis l’entrée.
- Les boss, passages scriptés et ennemis avancés ne sont pas validés ici.
  Les liaisons à sens unique de la ROM sont conservées telles quelles.
- Le niveau 10 conserve le retour au titre du prototype ; le test natif
  vérifie son événement de sortie, pas l’écran titre SGDK.
- Les objets des niveaux 4–10 restent limités aux clés nécessaires à cette
  passe. L’animation saccadée de Simon n’est pas corrigée ici.

## Reproduction

```sh
sh tools/test_stage_exits.sh
sh tools/test_room_transitions.sh
```

Avec Python et z80==1.2.0, et la ROM originale fournie séparément :

```sh
python3 tools/extract_collision_maps.py /chemin/reference.rom
python3 tools/extract_room_links.py /chemin/reference.rom
python3 tools/check_stage_exits.py /chemin/reference.rom
```

Les extracteurs de cartes et de liens acceptent --write pour régénérer les
sources. Les rapports MSX2_COLLISION_MAPS.json et MSX2_ROOM_LINKS.json conservent
les empreintes/cartes de contrôle et l’inventaire des modifications initiales.
La ROM originale et les binaires de test PC ne sont pas inclus dans l’archive.

Recompiler avec la commande Docker SGDK habituelle puis effectuer le parcours
global dans Kega Fusion. Ce bilan remplace les anciennes mentions de cartes
tronquées et de positions de test fixes dans les rapports historiques.

## Progression et sélecteur de salle (reprise des notes retirées)

- Sortie du niveau 3 : même changement de niveau que les sorties 1 et 2. L'écran
  bloquant « FIN DU STAGE 3 / STAGE 4 NON PORTÉ » a été supprimé. Le niveau 11 n'est
  jamais initialisé : une sortie du niveau 10 revient au titre.
- Contrôle : `python3 tools/audit_stage_progression.py` (tables des salles, branchement
  de progression, absence de l'ancien blocage).
- Explorateur (titre, A) : haut/bas choisit le niveau, gauche/droite la salle, B démarre
  dans la salle affichée, via `vk_game_debug_start()` (clés et objets réinitialisés).
  Le code Konami en pause ouvre le même choix ; C ou START revient au jeu. Changer de
  niveau ramène la salle à 0.
- Niveaux 11 à 18 : décors consultables, mais B affiche « SALLE NON JOUABLE - VISUEL ».
- Mode debug : le point d'apparition est une recherche de sol dans la salle demandée
  (plus de point fixe 16,192). Les entrées normales gardent leurs coordonnées MSX2.
