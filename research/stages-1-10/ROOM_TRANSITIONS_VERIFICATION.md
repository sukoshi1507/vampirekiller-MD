> État actuel : voir GLOBAL_LEVEL_VALIDATION.md. Cartes complètes restaurées et points d’apparition du mode test contrôlés.

# Transitions corrigées — 4 novembre 2026

Le blocage signalé concerne les changements de salle au niveau 4. Les tables
provisoires ont été remplacées par celles de la ROM MSX2 fournie, pour les niveaux
4 à 10. Les tables 1 à 3 concordaient déjà et leurs valeurs sont conservées.

93 entrées directionnelles modifiées : niveau 4 : 14 ; 5 : 15 ; 6 : 13 ;
7 : 24 ; 8 : 9 ; 9 : 9 ; 10 : 9. Il s’agit de directions, pas de passages distincts.

## Niveau 4

Salles numérotées depuis 0. Un tiret indique l’absence de liaison.

| Salle | Haut | Bas | Gauche | Droite |
|---|---|---|---|---|
| 0 | — | — | 2 | 1 |
| 1 | 4 | — | 0 | 2 |
| 2 | 5 | — | 1 | 0 |
| 3 | — | 0 | — | 4 |
| 4 | — | 1 | 3 | 5 |
| 5 | — | 2 | 4 | — |

La liaison 3 → 0 vers le bas est à sens unique dans la ROM. Les sorties verticales
ne sont pas des téléportations sur pression haut/bas : le moteur les déclenche au
franchissement du bord de l’écran ou par les escaliers.

## Validation effectuée

- Lecture de la table de pointeurs B9D3 (banque 13), décomposition de ses quatre
  directions par salle, puis comparaison à l’exécution de la routine Z80 B99A.
- Concordance de toutes les tables du portage : 73 salles, 292 directions.
- Tests C natifs avec les cartes et le contrôleur existants : 146 passages
  directionnels aux bords ; les absences de liaison sont également testées.
- Tests d’approche des dix portes avec la clé, animation complète et événement
  de sortie. Chargement du niveau suivant vérifié pour 1 → 2 jusqu’à 9 → 10.
  Le retour au titre après 10 reste géré par main.c, non exécuté dans ce test natif.
- Tests des clés, portes et du choix des 73 salles toujours réussis.
- AddressSanitizer et UndefinedBehaviorSanitizer : aucune erreur détectée.

Ces tests placent Simon près des portes ou aux frontières. Ils ne constituent
pas un parcours complet des niveaux depuis leur entrée et ne valident pas
l’accessibilité de tous les escaliers, les collisions, les boss ni les passages
spéciaux scriptés. Les liaisons à sens unique de la ROM sont conservées.

## Reproduire

```sh
sh tools/test_room_transitions.sh
sh tools/test_stage_exits.sh
python3 tools/extract_room_links.py /chemin/vers/reference.rom
```

La comparaison ROM nécessite z80==1.2.0. Pour régénérer les tables : ajouter
--write à la dernière commande. La ROM n’est pas incluse dans l’archive.
Le fichier tests/native_include/genesis.h ne contient que des types pour les
ressources des tests sur PC ; il n’émule aucune fonction SGDK et ne remplace
pas le SDK lors de la compilation Mega Drive.

Recompiler avec le conteneur SGDK habituel puis tester dans Kega Fusion.
Compilation SGDK et émulation non effectuées ici. Les points d’apparition
intermédiaires du mode test et les collisions restent à valider en jeu.

Ce rapport remplace les anciennes mentions de topologie provisoire dans les
rapports historiques. Les corrections de sélection de salle et des clés/portes
MSX2 sont incluses dans cette archive.
