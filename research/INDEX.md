# Index des notes de recherche

Synthèse de l'état de chaque sujet. Les notes détaillent les mesures, les traces
et les limites. « Prouvé » = mesuré en trace openMSX ou lu dans la ROM ;
« déduit » = conclu du code sans mesure ; « ouvert » = non résolu.

## Ennemis et combat

| Note | Sujet | Statut |
| --- | --- | --- |
| `enemy-behavior.md` | Spawn, mouvement, contact et mort des ennemis du hall (salles 0 à 7) | Prouvé pour les types 1 et 4 (spawn, trajectoire, contact, destruction au fouet via la table C800). Type 5 : ouvert. |
| `enemy-spawn.md` | Table et routine de spawn de la salle 1 | Prouvé. À corriger : Y=40 et le type 1 ne sont pas liés au bloc décrit. |
| `enemy-assets.md` | Motifs des ennemis, palette et couleurs (table F400) | Motifs prouvés (ROM et VRAM). Couleurs prouvées pour les types A et B ; motifs 50, 54, 64 non observés. |
| `enemies-platforms.md` | Premier ennemi observé, cadence, absence de plateforme mobile | Historique. Section 5 (plan d'implémentation) réalisée depuis dans `enemies.c`. |
| `player-health.md` | Énergie, contact, recul, invulnérabilité, mort, fin de partie | Prouvé par trace pour le contact et la mort. Le reste est lu dans le code. |
| `whip-hitbox.md` | Forme de la lanière du fouet (openMSX) | Prouvé pour les pixels mesurés. |
| `whip-hitbox-ingame.md` | Boîte du fouet et cible d'entraînement, en jeu (MD) | Prouvé pour la boîte debout ; accroupi non mesuré. |
| `whip-variants.md` | Fouets : variantes, séquence accroupie | Prouvé pour les niveaux 0 à 2 ; niveau 3 sans asset. |
| `door-room7.md` | Porte de sortie de la salle 7 du hall | Sortie prouvée (test forcé, arrivée stage 2 à x 16). Setter de C701 bit 0 identifié (une seule routine, `8E60`). Chemin naturel jusqu'à la salle 7 établi. Déclencheur du bit 0 : atteint en jeu naturel (run S2, objet de type 18 en salle 2) ; condition exacte à départager. |

## Mécaniques du joueur

| Note | Sujet | Statut |
| --- | --- | --- |
| `input-gate.md` | Déblocage du contrôle en openMSX, vitesse de marche | Prouvé (1 px par image). |
| `input-mapping-openmsx.md` | Mapping clavier openMSX | Prouvé. |
| `player-format.md` | Simon, collisions et saut (jalon 3) | Historique, à jour pour les collisions. |
| `jump-compare.md` | Saut MSX2 et MD : cadence et trajectoire | Prouvé : le MD allait deux fois plus vite. Corrigé (cadence à 30 Hz, commit `0e7d5a7`). Saut en mouvement non comparé. |
| `stairs.md` | Escaliers : mécanisme et positions | Mécanisme prouvé ; positions prouvées pour 2 volées du stage 1 salle 1, prédites pour les autres. |
| `down-stairs.md` | Descente par BAS | Entrée par la tuile 3 prouvée ; tuile 4 déduite. |
| `vertical-links.md` | Liens HAUT/BAS du hall | Prouvé pour 1→5, 5→1, 3→7, 7→3 et 5→4, 5→6. |
| `room-links.md` | Liens gauche/droite du stage 0 et du hall | Prouvé en jeu pour le stage 0 et le hall ; liens haut/bas et salles 4 à 7 : table seulement. |
| `room-format.md` | Formats de salles et adresses de carte (jalon 2) | Historique, à jour pour le chargement. |

## Décor, objets et sons

| Note | Sujet | Statut |
| --- | --- | --- |
| `candles-and-drops.md` | Bougies, torches et drops de la cour | Partiel : un cas complet prouvé (torche B, 17 répétitions). |
| `castle-candles.md` | Positions des bougies des 156 salles | Partiel mais solide pour les positions. |
| `courtyard-flames.md` | Graphismes des flammes de la cour | Prouvé. |
| `hit-sound.md` | Son du coup sur torche | Effet MSX 0x0C prouvé ; son MD = `SFX_14` de castlevania-md, identité déduite. |
| `sound-triggers.md` | Déclencheurs sonores du pilote MSX2 | Partiel (fouet, musique, arrêt du fouet). |
| `animation-inventory.md` | Inventaire des animations | Inventaire. |
| `mechanics-remaining.md` | Mécaniques restantes | Inventaire d'analyse. |

## Démarrage, pipeline et environnement

| Note | Sujet | Statut |
| --- | --- | --- |
| `intro-format.md` | Démarrage et entrée du château (jalon 4) | Historique, à jour pour le démarrage. |
| `start-environment.md` | Intégration de start.zip | Historique. |
| `diagnostic.md` | Diagnostic initial (identité du dump) | Historique. |
| `z80-vdp-io-analysis.md` | Boucle VBLANK et accès VDP | Analyse. |
| `loading-and-states.asm`, `known.asm` | Extraits désassemblés annotés | Référence. |
| `trace-plan.md` | Plan d'une session de trace | Ancien plan de travail. |
| `castlevania-nes-comparison.md` | Comparaison avec castlevania-md | Référence. |
| `licensing.md` | Inventaire des sources tierces et licences | Inventaire. Points à trancher (voir la note). |

Dossiers : `audio/` (VGM et CSV), `castle-navigation/`, `partner-baseline/`
(code partenaire, licence inconnue), `simon-assets/`, `simon-frame-audit/`.
