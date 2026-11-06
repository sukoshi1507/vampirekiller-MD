# Index des notes de recherche

Synthèse de l'état de chaque sujet. Les notes détaillent les mesures, les traces
et les limites. « Prouvé » = mesuré en trace openMSX ou lu dans la ROM ;
« déduit » = conclu du code sans mesure ; « ouvert » = non résolu.

## Ennemis et combat

| Note | Sujet | Statut |
| --- | --- | --- |
| `enemy-behavior.md` | Spawn, mouvement, contact et mort des ennemis du hall (salles 0 à 7) | Prouvé pour les types 1 et 4 (spawn, trajectoire, contact, destruction au fouet via la table C800 ; type 4 : dessin, 4 poses et courbe Y mesurés image par image). Type 5 (panthère, salles 2, 3, 7) : prouvé, code lu et trace image par image (liste ROM `0x1C674`, activation à moins de 64 px, course 2 px/image, saut et gravité, 8 poses, dessin ROM `0x15AB9`, palette, dégât 6, mort au fouet accroupi seulement). Reste : rôle du bit 7 de la liste, score non mesuré. Stage 2 : drapeaux, routines et chauve-souris (même dessin, Y = C425 - 22, cadence 168/328) prouvés ; type 1 de la salle 2 du stage 2 : le zombie du hall (mêmes couches et couleurs, Y `A0`, côté = bit 1 de `CF01`, sortie, contact et fouet de la ROM), prouvé et porté ; type 2 lu dans le code seulement. |
| `enemy-spawn.md` | Table et routine de spawn de la salle 1 | Prouvé. À corriger : Y=40 et le type 1 ne sont pas liés au bloc décrit. |
| `enemy-assets.md` | Motifs des ennemis, palette et couleurs (table F400) | Motifs prouvés (ROM et VRAM). Couleurs prouvées pour les types A et B (couche 1 : couleur 2, couche 2 : couleur 4 avec CC, recouvrement 6) ; 48/4C non observés à l'écran. |
| `enemies-platforms.md` | Premier ennemi observé, cadence, absence de plateforme mobile | Historique. Section 5 (plan d'implémentation) réalisée depuis dans `enemies.c`. |
| `player-health.md` | Énergie, contact, recul, invulnérabilité, mort, fin de partie | Prouvé par trace pour le contact et la mort. Le reste est lu dans le code. Porté : `enemies.c` (contact), `health.c` (vies, délais de mort et de fin de partie, test `tests/test_health.c`), `hud.c` (barre PLAYER, échelle provisoire). |
| `whip-hitbox.md` | Forme de la lanière du fouet (openMSX) ; fouet en saut : un seul test par coup (passe `C422 = 3`), à la position de Simon de cette passe, affichage qui suit Simon | Prouvé pour les pixels mesurés ; fouet en saut prouvé (code et trace) et porté (timer 11). |
| `whip-hitbox-ingame.md` | Boîte du fouet et cible d'entraînement, en jeu (MD) | Prouvé pour la boîte debout ; accroupi non mesuré. |
| `whip-variants.md` | Fouets : variantes, séquence accroupie | Prouvé pour les niveaux 0 et 1. Les « niveaux » 2 et 3 sont la dague et la hache lancées (`secondary-weapons.md`). |
| `secondary-weapons.md` | Armes lancées : dague (1B), hache (1C), croix (1D), eau bénite (1E), touche, emplacements `C450`/`C460`, trajectoires, collisions, perte de l'arme ; objet 04 | Prouvé (code et trace openMSX) et porté (`weapons.c`, test natif, scénario `zb`). Ouvert : pose accroupie, objets muraux, objet 04 et `D700`, marchand. |
| `weapon-targets.md` | Armes lancées et fouet sur les objets 0A, 0B, 0C, 0D, boules, os et boss 3/6/9 : dégâts, invulnérabilité, scores, cœurs, boîtes | Tracé (openMSX) ; test natif `test_weapons.c`. Non tracé : hache/croix/fouet sur le boss 13h. Bords hauts : voir `enemy-boxes.md`. |
| `enemy-boxes.md` | Boîtes des ennemis (0A, 0B, 0C, 0D, 08, boule, os) pour les armes lancées, le fouet et le contact, par re-exécution de `8500`/`84A7`/`847A` | Tracé ; bord haut 147/148 élucidé (interception) ; second emplacement d'arme C460 (L + 6, H + B) corrigé ; objet 08 isolé ; eau bénite non balayée. |
| `door-room7.md` | Clés blanches et portes de sortie : salle 2 / salle 7 du hall, salle 5 / salle 1 du stage 2 | Prouvé : clé = objet 0x18 (C701 bit 0), porte ouverte avec la clé (K3) et fermée sans (K4), levée du barreau et sortie mesurées image par image ; stage 2 (table `BB61`, trace K4 du stage 2, saut sans plafond, §18). Porté sur MD (`items.c`, §17-18) ; sortie du stage 2 vers un écran provisoire. Ouvert : blocage à x 166 en salle 2. Clé dorée et coffre : `castle-candles.md` 9.7. |

## Mécaniques du joueur

| Note | Sujet | Statut |
| --- | --- | --- |
| `input-gate.md` | Déblocage du contrôle en openMSX, masques clavier (`keymatrixdown`, ligne 8), vitesse de marche | Prouvé (1 px par image). |
| `player-format.md` | Simon, collisions et saut (jalon 3) | Historique, à jour pour les collisions. |
| `frame-timing.md` | Calendrier d'affichage : du calcul d'une passe à l'écran, par élément (sprites, décor, HUD, bordure, son, manette), MSX et MD | Prouvé (trace openMSX, harnais MD). Porté : sprites de Simon et des ennemis à 2 images comme le MSX. Restent : tuiles du décor et certains sons une image trop tôt. |
| `jump-compare.md` | Saut MSX2 et MD : cadence et trajectoire | Prouvé : le MD allait deux fois plus vite. Corrigé (cadence à 30 Hz, commit `0e7d5a7`). Saut en mouvement non comparé. |
| `stairs.md` | Escaliers : mécanisme et positions | Mécanisme prouvé ; positions prouvées pour 2 volées du stage 1 salle 1, prédites pour les autres. |
| `down-stairs.md` | Descente par BAS | Entrée par la tuile 3 prouvée ; tuile 4 déduite. |
| `vertical-links.md` | Liens HAUT/BAS du hall | Prouvé pour 1→5, 5→1, 3→7, 7→3 et 5→4, 5→6. |
| `room-links.md` | Liens gauche/droite du stage 0 et du hall | Prouvé en jeu pour le stage 0 et le hall ; liens haut/bas et salles 4 à 7 : table seulement. |
| `castle-entrance.md` | Entrée du château (cour 2 → hall 0) : porte, marche automatique, écran noir | Prouvé par trace (runs A et B) et code (`7718`, `778A`, `8580`, `55EC`) : zone x 216..223, marche 1 px / 4 images derrière le montant (sprites devant Simon), sortie x 232, 36 images noires, hall x 16, y 192. Porté (`gameplay.c`, `main.c`). Non porté : effet sonore 0x15. |
| `room-format.md` | Formats de salles et adresses de carte (jalon 2) | Historique, à jour pour le chargement. |

## Décor, objets et sons

| Note | Sujet | Statut |
| --- | --- | --- |
| `candles-and-drops.md` | Bougies, torches et drops de la cour | Partiel : un cas complet prouvé (torche B, 17 répétitions). |
| `castle-candles.md` | Positions des bougies des 156 salles | Partiel mais solide pour les positions. |
| `merchant.md` | Objets muraux (vieil homme) et marchand, stages 0-2 | Prouvé par le code et trace pour la fenêtre et les prix ; hasard `R` provisoire |
| `stage3-boss.md` | Boss du stage 3 (objet 12h), orbe, fin de stage | Prouvé par le code et trace pour le vol, le fouet, le contact et le script ; armes lancées, registre `R` et sortie d'écran de la boule provisoires |
| `stage4-enemies.md` | Stage 4 : objets 06 et 08, réarmement des coups, `D002` | Prouvé par le code et trace pour l'objet 06 en salle 0 ; poses droite de 08 déduites ; autres salles non comparées |
| `stage5-enemies.md` | Stage 5 : objets 06 et 08, plateformes mobiles (`906C`, `8524`, `6BAF`) | Prouvé par le code et trace (positions 06 salles 2 et 3, plateformes, transport, saut) ; arrêt par un mur, temps arrêté et bit 7 ouverts. |
| `stage6-enemies.md` | Stage 6 : chauve-souris 07, statue 0A et sa boule, pics du plafond (`8F9A`, `8FEA`, `85A6`), objet 08 des salles 3 et 4 | Prouvé par le code et trace (07 passe par passe, 0A et boule, pics et leurs deux dégâts, décor des 6 salles) ; boss de la salle 5 non porté, contact de la boule non tracé. |
| `stage6-boss.md` | Boss du stage 6 (objet 13h, salle 5), clé blanche, porte, orbe, passage au stage 7 | Prouvé par le code et trace passe par passe (objet 13h, 4 traces, 0 écart jusqu'à 2851 passes ; boule ; orbe et décor au pixel) ; armes lancées, score, phase de la première boule déduits ou non vérifiés ; stage 7 sans ennemis. |
| `stage8-enemies.md` | Stage 8 : hopper 0B (`B37E`) et son os D700 type 4 (`9F07`), leaper 0C (`A67D`), objets 08 et 0A, objets de salle, clé blanche, porte et sortie vers le stage 9 | Prouvé passe par passe contre 4 traces (0B 2557 transitions, 0C 990, os 2131 : 0 écart, comparaison octet par octet), dégâts relevés, décor des 8 salles, sortie ; fouet, armes lancées, boîtes exactes, état 7 du 0C, hasard du lancer, murs `CF` et `D3` déduits ou non vérifiés ; stage 9 sans ennemis. |
| `death-flame.md` | Flamme de mort (objet 1E, `9A3E`) : appelants, création (premier bloc libre, X, Y - D de la table `9B33`), routine `9B71` (16 passes, poses 85/86), SAT, motifs F4/F8, couleur 8, interaction (ni contact ni fouet, pas figée par D010), tirage du drop | Prouvé (code et 64 flammes en trace openMSX : fouet, armes, contact ; stages 1 à 9) et porté (`enemies_death.inc`, test natif, scénario `zzh`). Drop porté (`9ABE`, tirage `R` et `CF40`, cœur 24 et flammes 26 réutilisant `candles.c`, 39 cœurs et 47 flammes 26 tracés, scénario `zzi`) ; non porté : type 0E, boss. |
| `stage9-enemies.md` | Stage 9 : objets 08, 0A, 0B, os, 0D de la liste `86B6`, tuiles, objets fixes, clé et porte | Prouvé passe par passe contre la trace R (hopper 640 transitions, os 259 : 0 écart hors échantillons en cours de passe) et en scénario (08, statue et boule, hopper, puces, positions de création) ; puce (hasard `R`), coffres 09, 02, 05, 1E, décor non vérifiés. |
| `hasard-R.md` | Registre R du Z80 (`LD A,R`) : recensement des 15 lectures de la ROM, distribution tracée (puce, os, boss) et générateur du port | Prouvé en trace : A < 80h, bits 0 à 6 uniformes (stages 7 à 9, boss 3 et 9, marchand, drop `9AE5` : plus de 2 000 appels) ; séquence exacte non reproductible ; boss, marchand et os/puce sur le même générateur ; sites du stage 18 (gerbe `684F`) et 10+ (`AA81`, `BBA8`) documentés, non portés ; 4 `ED 5F` de la banque 14 = données musicales. |
| `candle-flames.md` | Durée des flammes de bougies et blocs (objets 00, 01, 02-1D, 15), chute de la flamme 26, apparition de l'objet, repos au sol | Prouvé (traces openMSX, 12 flammes sur 8 stages) ; port corrigé (`candles.c`), dessin des flammes (motifs F4/F8/EC, couleurs, poses) tracé et porté (`candle_view.c`). |
| `stage9-candle-object.md` | Objet 1A/1B/1C libéré par les bougies et blocs d'objet 15 (`8AD0`, routine `B4EA`, init `B4D2`), stages 4 à 6 (1A) et 7 à 9 (1C) | Prouvé : création, chute, marche, contact, fouet, durée de la flamme tracés (openMSX) et 16 291 passes comparées aux routines ROM (émulateur Z80) ; 1A tracé aux 17 bougies/blocs des stages 4-6 (même image, couleur 15, base de pose 9D/9F/9B) ; 1B (stage 10) non porté (stage 10 absent) ; hache sur un bloc = autre objet (C500). |
| `chests-stage1-9.md` | Coffres des stages 1 à 9 (31 coffres, contenu, effet, son, durée d'affichage) et décor du stage 9 | Prouvé en trace openMSX ; écarts corrigés : orbe 64 images (30 au port), contenu 0F (`C431` bit 6), contenu 12 (trois clés) ; scénario `zzz_chests_stage1_9` |
| `stage9-boss.md` | Boss du stage 9 (deux objets 14h, salle 7), boule D700 type 7, clé, porte, orbe, passage au stage 10 | Prouvé passe par passe contre 3 traces (14h : 488 + 658 transitions, 0 écart ; boule : 427 transitions, 5 échantillons en cours de passe), dégâts, mort, orbe (Y C0h), entrée du stage 10 ; armes lancées, orbe pris, boîtes exactes, vx initial déduits ou non vérifiés. |
| `stage10-level.md` | Stage 10 : décor des 9 salles, plateformes mobiles (`906C`), bougies, blocs, 4 coffres, clé, porte et sortie vers le stage 11, objet 1B | Prouvé en trace openMSX (décor : 0 tuile différente ; plateformes : X début + 1 puis 191 ; 4 coffres ; clé ; porte et entrée du stage 11 : D000 11 à 168 images du son 1Ah) ; 1B inatteignable (aucune bougie 15) ; stage 11 non porté. |
| `boss-boxes.md` | Boîtes de contact des boss 3, 6, 9 et de leurs boules, tracées en openMSX (points d'arrêt `847A`/`84A7`) | Classe de la boule du stage 6 = (6, 6) (corrigé), corps et fouet conformes, limites exactes aux stages 6 et 9 ; limites verticales, fouet contre boules et armes lancées tracés par appel forcé (0 écart) ; les armes détruisent les boules (corrigé). |
| `stage7-enemies.md` | Stage 7 : squelette 0B et ses os (D700 type 4), puce 0D, créature volante 0C, clé, coffres, porte, sortie vers le stage 8 | Prouvé : trace = routines ROM exécutées dans un émulateur Z80 (1982 échantillons sur 2010), port = ROM exécutée (4122 passes, 0 écart), décor des 9 salles ; `LD A,R`, contact du squelette, fouet et armes déduits ou non vérifiés. |
| `stage-map.md` | Carte du stage, objet 0F | Prouvé par le code et trace (cour) ; rendu sans étapes |
| `reinstall.md` | Réinstaller l'environnement de travail | Relevé sous CachyOS, testé sous Omarchy |
| `courtyard-flames.md` | Graphismes des flammes de la cour | Prouvé. |
| `hit-sound.md` | Son du coup sur torche | Effet MSX 0x0C prouvé ; table d'effets corrigée par `audio-driver.md` ; le son MD est maintenant le 0C de la ROM. |
| `audio-driver.md` | Pilote son MSX, tous les effets et musiques | Pilote prouvé, extraction par émulation conforme à la trace openMSX (8A, 80, 05) ; lecteur MD musique + effets ; identités en partie provisoires. |
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
| `castlevania-nes-comparison.md` | Comparaison avec castlevania-md | Référence. |
| `licensing.md` | Inventaire des sources tierces et licences | Inventaire. Points à trancher (voir la note). |

Dossiers : `audio/` (VGM et CSV), `castle-navigation/`
(code partenaire, licence inconnue), `simon-assets/`, `simon-frame-audit/`.

## Stages 1 à 10

Notes issues d'un lot de corrections externe (`stages-1-10/`). Elles ont été écrites sans
compilation SGDK ni émulation : la compilation et les scénarios Mega Drive ont été passés
depuis, sur les stages 1 et 2 seulement. Leur jugement sur le côté du type 1 (`C42C`, `9D30`)
et sur les cartes (0 écart avec `4F83`) a été recontrôlé sur la ROM.

| Note | Sujet | Statut |
| --- | --- | --- |
| `stages-1-10/GLOBAL_LEVEL_VALIDATION.md` | Parcours global des 73 salles, progression entre niveaux, sélecteur de salle | Tests natifs, pas d'émulation |
| `stages-1-10/MSX2_EXITS_VERIFICATION.md` | Clés et portes (`BB37`, `8580`, `6402`) | Prouvé contre la ROM (`tools/check_stage_exits.py`) |
| `stages-1-10/ROOM_TRANSITIONS_VERIFICATION.md` | Liens de salles (`B99A`) | Tests natifs, pas d'émulation |
| `stages-1-10/ENEMY_INTEGRATION.md` | Drapeaux et listes d'ennemis fixes | Extrait de la ROM ; comportement hors stages 1-2 non comparé |
