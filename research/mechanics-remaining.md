# Mécaniques de Vampire Killer MSX2 : état du portage Mega Drive

Statut : **inventaire d'analyse**, aucun code modifié. Le fouet (variantes, accroupi) est traité par un autre travail et n'est ici qu'un point de référence.

ROM de référence : `Vampire Killer (Japan, Europe) (En).rom` extraite du zip de la racine, SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2` (vérifié). Adresses CPU Z80 sauf mention « ROM off. » (offset fichier). Banques : `4000-5FFF` fixe (banque 0), `6000-7FFF`, `8000-9FFF`, `A000-BFFF` commutables (mapper Konami sans SCC, voir `research/diagnostic.md`).

Légende des statuts :
- **porté** : code MD présent, comportement vérifié contre la ROM ou contre une mesure.
- **partiel** : code MD présent, mais simplifié, prototype, ou non relié à la mécanique d'origine.
- **absent** : aucun code MD.
- **inconnu** : mécanique non trouvée dans la ROM ou non mesurée.

Préfixes de preuve : **prouvé** (mesure openMSX ou exécution du code ROM), **déduit** (lecture statique, non mesurée), **inconnu**.

---

## 0. Vue d'ensemble

Le portage actuel (`sgdk/src/`, `main.c` 562 lignes) couvre le démarrage, la cour, le hall en prototype et un explorateur de 152 fonds. Il n'y a **ni ennemi, ni dégât, ni vie, ni objet, ni sous-arme, ni boss, ni game over** dans le code MD (vérifié par recherche des mots-clés dans `sgdk/src/*.c` et `*.h` : `hp`, `health`, `damage`, `lives`, `score`, `enem`, `boss`, `item`, `subweapon` ne donnent que des commentaires ou rien).

Structure de la ROM (prouvée par `research/room-format.md`, `castle-navigation/NAVIGATION_AUDIT.md`, `sgdk/src/generated/rooms.c`) :

| Zone | Contenu | Salles | Statut MD |
|---|---|---|---|
| Stage 0 | Cour extérieure, 3 écrans (connexions 0-1-2 de la table de l'intro) | 3 (hors table des 153 salles) | partiel : marche, transitions, porte simplifiée |
| Stage 1 | Hall du château | 8 | partiel : prototype, graphe de liens non ROM |
| Stages 2 à 18 | Intérieurs, 145 salles au total | 145 | absent en jeu ; explorateur de fonds seulement |

La demande parle de « progression entre les 3 zones ». Le dépôt ne définit pas de « 3 zones » au sens de trois grands niveaux. Les trois écrans de cour sont probablement ce qui est visé. **Inconnu** : confirmer l'équivalence avant de planifier.

### 0.1 Ce qui existe déjà dans le code MD

| Élément | Fichier | Statut |
|---|---|---|
| Logo, titre, scène animée devant la grille (320 positions, 14 motifs) | `main.c` (`show_castle_frame`, `begin_castle`), `generated/intro_assets.c` | porté (reconstruction exécutée depuis la ROM, `research/intro-format.md`) |
| Lecture manette, front du bouton C | `input.c` | porté |
| Physique marche / saut / chute / accroupi | `player.c` (`vk_player_tick_controls`) | porté pour les tables de saut et de chute (53 872 comparaisons C/Z80 dans `research/player-format.md`) ; poses et escalier non authentiques |
| Collisions de salle, cartes 768 octets | `generated/stage1_collision.c`, `generated/rooms.c`, `gameplay.c` | partiel : salle 0 du stage 1 vérifiée ; salles 1 à 7 avec 64 tuiles masquées mises à zéro (`research/RETROENGINEERING_NEXT.md`) |
| Cour : 3 écrans, défilement horizontal | `gameplay.c` (`vk_courtyard_neighbors`), `main.c` (`show_courtyard`) | partiel (liens de la table de l'intro, `research/intro-format.md`) |
| Porte du château | `gameplay.c` (`vk_game_tick_controls`) | partiel : HAUT à x 192-216 sur l'écran 2 ; pas le mécanisme d'origine |
| Graphe de salles du hall | `room_manager.c` (`vk_stage1_links`) | partiel : **explicitement un graphe de test** (commentaire du fichier) |
| Escalier | `player.c` (`vk_player_tick_controls`, `VKStair`) | partiel : une seule zone factice `{120,208,96,184}` activée dans le hall salle 1 |
| Fouet | `whip.c`, `targets.c` | hors périmètre (autre travail) |
| Cible d'entraînement, flamme de la cour | `main.c` (`reset_training_target`), `targets.c` | partiel : prototype (voir M17) |
| Flammes de la cour, 2 états ROM | `generated/flame_fx_md.h` (commit `19819d0`) | partiel : tuiles ROM, cadence `flame_clock>>3` sur 4 trames A,A,B,B ; décalage de 4 ticks non porté |
| Explorateur de 152 fonds + 153 cartes | `main.c` (`show_room`), `generated/rooms.c` | porté (outil de test, pixels comparés dans Genesis Plus GX, `README.md`) |
| Musique et effets | `psg_player.c`, `generated/music_md.c`, `generated/sfx_md.c` | partiel : voir M29 et M30 |
| Mode test « Konami » / sound test | `sound_test.c` | hors jeu |

### 0.2 Limites de l'analyse

- Le dump Z80 (`research/loading-and-states.asm`) couvre seulement les régions désassemblées. Les routines appelées depuis l'état de jeu (`5C25`, `5A2E`, `4495`, `94BA`, `9507`, `62D0`, `47B9`, `4E93`, `5E7D`) n'ont **pas** été désassemblées dans ce dépôt.
- La table de spawn des ennemis n'est pas identifiée (voir M08c).
- Aucune trace d'ennemi touché, de dégât, de ramassage, de mort ou de changement de salle en jeu n'existe (`research/sound-triggers.md`, lignes « non déclenché »).

---

## 1. Machine d'états de jeu (`C000`, `C001`)

Dispatch en `414D` : lecture de `C000` comme index de la table inline `4161` (14 cibles). Source : `research/diagnostic.md` et `research/loading-and-states.asm`. Le nom des états est déduit du code ; seuls les états 5 et les exits de 5 sont bien lus.

| `C000` | Cible | Lecture statique | Statut MD |
|---|---|---|---|
| 0 | `417D` | Retour titre (`C000=0` posé par `41B5`) | porté (titre) |
| 1 | `41A0` | Séquence de titre/intro, décompte `C004` | porté (ROM reconstruite) |
| 2 | `41AB` | Attente, dépend de `C413` | inconnu |
| 3 | `41D1` | Démarrage de musique (`C000=3` écrit par `43B5`) | partiel |
| 4 | `422C` | **Décrémente `C410` (BCD)**, met `C413=1`, appelle `794F`, puis passe à 5 | absent (candidat : perte de vie ou entrée de salle) |
| 5 | `4257` | **Jeu**. Sort vers 12, 9, 10, 8, 11 ou 6 selon `C40C`, `C41B`, `C408`, `C409`, `C40A`, `C413` (ordre de test dans le code) | partiel (boucle MD) |
| 6 | `4294` | Si `C410 ≠ 0` : `C420=0`, `C40D=1`, puis état 4. Sinon : son `8B` (`509F`), puis état 7 | absent (candidat : contrôle de vie / fin de partie) |
| 7 | `42B2` | Attend `C0A7` (musique) et `C004`, puis `C002 &= ~40` et `C000=0`. Chemin `42D3` (si `E600 ≠ 0` et `4314` renvoie NZ) : `C410=3`, `C405/C407=0` | absent (candidat : écran de game over puis retour titre) |
| 8 | `4324` | `C410` et `C411` BCD +1, `D002` (compteur à 6), `CF34` (compteur à 2) ; à 2 : retour à 0 | absent (candidat : ramassage ou bilan) |
| 9 | `4377` | Appelle `5A2E`, puis état 6 ou 5 | inconnu (candidat : transition de salle) |
| 10 | `4387` | `C701 &= ~1`, puis même code que 8 (`4347`) | inconnu |
| 11 | `43DA` | Lit `C00B` bit 0 | inconnu |
| 12 | `43F0` | Appelle `94BA`, compte `C004` puis appelle `9507` | inconnu |
| 13 | `4414` | Lit `C006 & 33` ; branches sur `E60B` (0, 1, 2) vers `5CFD`, `5D82`, `5D61` | inconnu (candidat : menu ou sélection de sous-arme) |

Flags à noter (prouvés par lecture du code) :
- `C002` bit 6 (`0x40`) : porte d'entrée. Mis à 1 par `43AC` ; effacé par `42CD` (fin de partie). `input-gate.md` le donne à 0x40 en jeu.
- `C0A7` : état de la file son (écrit à `07` au démarrage de musique, `00` à la fin).
- `E600` : drapeau de mode (lu en `42B4`, `42EF`, `43AF`). Sens non établi.
- `E60B` : valeur 0/1/2 lue en `4421`. Sens non établi.

---

## 2. Tableau des mécaniques

Les identifiants `M01`-`M28` servent à la section 3 (dépendances) et à la section 4 (ordre).

### 2.1 Simon : mouvement et contrôle

| ID | Mécanique | Statut | Preuves | Dépendances | Priorité |
|---|---|---|---|---|---|
| M01 | Marche gauche/droite, 1 px par image | porté | MSX : `C427` +1 par VBLANK (`research/input-gate.md`, mesure). MD : `player.c` boucle `n<1`, 1 px par appel (`vk_player_tick_controls`). Note : `input-gate.md` dit encore « 2 px par image » pour le MD, c'est obsolète. | aucune | P0 (fait) |
| M02 | Saut (table `6D95`, 19 valeurs) et chute (`6F81`) | porté | Table et sauts comparés en C/Z80 (`research/player-format.md`, `player.c` `vk_jump_step`, `vk_fall_deltas`). Poses de saut `6/7` non authentiques. | M03 | P0 (fait) |
| M03 | Collisions sol et mur (`7D2F`, `7C5E`, `7B88`, `7BA9`) | porté pour la salle 0 du stage 1 et la cour ; partiel ailleurs | `player.c`, `generated/stage1_collision.c`. Salles 1-7 du hall : 64 tuiles masquées mises à zéro. | M16 | P0 |
| M04 | Accroupi | partiel | `player.c` (`crouched`, sondes latérales réduites à 3). Pose authentique non extraite. Hors périmètre fouet ; l'accroupi est traité par l'autre travail. | M02 | hors périmètre |
| M05 | Escalier : entrée, montée/descente, sortie | partiel (faux escalier) | `player.c` (`on_stairs`, `VKStair`). `gameplay.c` : volume `{120,208,96,184}` posé à la main, marqué « NOT an original MSX2 stair coordinate ». Pas de table d'escaliers dans la ROM identifiée. Cycle de marche `{0,1,2,1}` = poses marche, pas poses escalier. | M16 (données) | P2 |
| M06 | Fouet (attaque, hitbox, sons) | hors périmètre | `whip.c`, `research/whip-hitbox*.md`, `sound-triggers.md` | aucune | hors périmètre |
| M07 | Sortie de saut ou de marche par bord (fin d'écran) | partiel | `gameplay.c` : bord x ≥ 248 ou ≤ 8 ; en cour, voisin par `vk_courtyard_neighbors`. Hall : `room_manager.c`, graphe de test. | M15 | P1 |
| M08 | Chute dans le vide, retour au départ (`y ≥ 208`) | partiel | `player.c` : `if (p->y >= 208) vk_player_reset(p)`. Comportement original non mesuré (pas de vie, pas de mort). | M18 | P1 |

### 2.2 Ennemis et combat

| ID | Mécanique | Statut | Preuves | Dépendances | Priorité |
|---|---|---|---|---|---|
| M08b | Ennemi de base (1 type observé, traversée 1 px par image, animation 8 images par pose) | absent | `research/enemies-platforms.md` §1.2 : ennemi observé en salle 1 du stage 1 (MSX), trajectoire et cadence mesurées sur 218 images. Pas de code MD. | M08c (spawn), M12 (hitbox) | P1 |
| M08c | Table de spawn par salle (position, type, nombre, déclencheur) | inconnu | `enemies-platforms.md` §1.4 : `5A49` décode la **carte de cellules** `E000-E47F` (constante en jeu), pas les ennemis. Candidats : banque 14 (`8000` et `800C` lus par `5A49`), `C426`, `C43x`. Non prouvé. | M16 (salle) | P1 (bloquant pour M08b) |
| M09 | Réapparition des ennemis (intervalles 111, 130, 152 images) | inconnu | `enemies-platforms.md` §1.2 : cause non identifiée | M08c | P2 |
| M10 | Huit sprites fixes à `X=E0` (salle 0, image 776) | inconnu | `enemies-platforms.md` §3 : nature non identifiée (décor, cierge, ou objet ?) | M08c | P2 |
| M11 | Contact ennemi → dégât, recul (table de delta), invulnérabilité | absent | Recul : `enemies-platforms.md` §3, candidat `6D4D` (table de delta de Y, utilisée pour le saut). Dégâts : pas de trace (`sound-triggers.md`). `VKPlayer` n'a aucun champ de vie. | M08b, M12 | P1 |
| M12 | Boîte de collision corps de Simon | absent | `enemies-platforms.md` §3 : boîte inconnue. `player.c` n'expose qu'un point (x,y) et des sondes. | aucune | P1 |
| M13 | Boss | absent | Aucune trace. Aucune donnée boss identifiée dans les notes. | M08b, M11, M14 | P4 |

### 2.3 Vies, dégâts, mort

| ID | Mécanique | Statut | Preuves | Dépendances | Priorité |
|---|---|---|---|---|---|
| M14 | Compteur de vies (`C410`, BCD) | absent | Décrémenté en `422C` (état 4), testé en `4294` (état 6), réinitialisé à 3 en `42D3` (chemin 7). Lecture statique seulement ; **sens (vies, cœurs, ou autre) non établi** : `C410` est aussi incrémenté en états 8 et 10. | M11, M15 | P1 |
| M15 | Game over, écran de fin, retour titre | absent | Candidat : état 6 (`C410=0`) → son `8B` (`42AB`) → état 7 → `C000=0` (`41B5`). Table de sons : `sound-triggers.md` (« Fin / game over : inconnu »). | M14 | P1 |
| M16 | Fond de salle, cartes et liens de salle, mémoire de la salle | partiel | `research/room-format.md`, `research/castle-navigation/*` : 153 cartes décodées et vérifiées. **Aucun lien entre deux salles n'est authentifié** (NAVIGATION_AUDIT, « Connexions authentifiées : aucune »). Écritures candidates de `D000/D001` listées dans `Z80_ROOM_STATE_AUDIT.md`. | aucune | P0 (bloquant) |
| M17 | Cible d'entraînement / flamme éteinte par le fouet | partiel (prototype) | `main.c`, `targets.c`. Le comportement « flamme éteinte » n'est pas un mécanisme d'origine connu (le message « FLAMME ETEINTE » est un texte de test). | M06 | hors périmètre |
| M18 | Chute mortelle et retour au départ de salle | inconnu | Voir M08 ; `C000=9` observé quand Simon atteint x=10 après avoir été à droite (`enemies-platforms.md` §1.5, déduit, non vérifié). | M16 | P2 |

### 2.4 Objets, bougies, cœurs, sous-armes

| ID | Mécanique | Statut | Preuves | Dépendances | Priorité |
|---|---|---|---|---|---|
| M19 | Bougies / cierges destructibles qui lâchent un objet | absent | Aucune trace. Candidat : 8 sprites fixes `X=E0` (M10). | M06, M08c | P2 |
| M20 | Ramassage d'objets (cœurs, sous-armes, bonus) | absent | `sound-triggers.md` : « Ramassage : non déclenché ». Aucun code MD. | M19, M21 | P2 |
| M21 | Cœurs (munitions de sous-armes ?) | inconnu | Candidat : `C411` (BCD, +1 en états 8 et 10). **Non établi.** | M20 | P2 |
| M22 | Sous-armes (lancer, dague, hache, etc.) : données, tir, collision | absent | Aucune trace. Pas de table d'objets identifiée. Le menu d'état 13 (`C006`, `E60B`) est un candidat pour la sélection. | M20, M21, M13 | P3 |
| M23 | Score | inconnu | Pas de trace. Candidats : `C405/C407` mis à zéro en `42D3`. | M14 | P3 |

### 2.5 HUD et affichage

| ID | Mécanique | Statut | Preuves | Dépendances | Priorité |
|---|---|---|---|---|---|
| M24 | HUD d'origine (fond des 32 lignes supérieures) | partiel | `research/room-format.md` : les 32 lignes supérieures sont réservées au HUD. `intro-format.md` : HUD reconstruit en exécutant les routines de l'intro. `main.c` ne met à jour aucune valeur. | M14, M21, M23 | P1 |
| M25 | Valeurs dynamiques du HUD (vies, cœurs, score, temps, état de sous-arme) | absent | Aucune écriture MD. Emplacement RAM : inconnu (`C405-C41x`, `C420` non étudiés pour le HUD). | M24 | P1 |
| M26 | Défilement écran par écran | absent (non nécessaire) | Les salles sont 32×24 tuiles, un écran (`room-format.md`). Pas de scroll observé. | aucune | hors portage |
| M27 | Décor animé (flammes de la cour) | partiel | `courtyard-flames.md` : deux états ROM, cadence 16 ticks, décalage de 4 ticks entre A et B. MD : `main.c` anime par `flame_clock>>3`, pas de décalage. `README.md` dit « statiques » : **obsolète** (le code anime). | aucune | P3 |
| M28 | Scène titre, logo, intro devant la grille | porté | `intro-format.md`, `main.c` (`front_mode`), `validation/md-konami.png` | aucune | fait |

### 2.6 Son

| ID | Mécanique | Statut | Preuves | Dépendances | Priorité |
|---|---|---|---|---|---|
| M29 | Musique de jeu (titre, cour, partie) | partiel | `sound-triggers.md` : démarrage musique prouvé (`C000=3` routine `5109`, `C000=5` même routine). MD : `music_md.c` est une **capture PSG candidate** (`research/audio/`), jouée seulement dans `sound_test.c`, pas en jeu. | M22 (ne bloque pas) | P3 |
| M30 | Effet du fouet | partiel, hors périmètre | `generated/sfx_md.c` : **le son du fouet est emprunté au projet castlevania-md (SFX_0A NES)**, pas à la ROM MSX2. | M06 | hors périmètre |
| M31 | Sons de dégât, mort, ramassage, impact, changement de salle | absent | `sound-triggers.md` : tous « inconnu ». Non déclenchés en test. | M11, M15, M20 | P3 |

### 2.7 Progression et monde

| ID | Mécanique | Statut | Preuves | Dépendances | Priorité |
|---|---|---|---|---|---|
| M32 | Cour : trois écrans reliés (0-1-2), départ `x=16 y=176` | partiel | `intro-format.md` (table des connexions, extraction après `422C`), `gameplay.c` | M16 | P1 |
| M33 | Porte du château (écran 2, HAUT) ouvrant le hall | partiel (simplifié) | `intro-format.md` : « adaptation du prototype ». `gameplay.c` : x 192..216, HAUT. Mécanique d'origine non identifiée. | M32, M16 | P1 |
| M34 | Hall du château (stage 1, 8 salles) : liens et entrée | partiel | `room_manager.c` : graphe marqué test. Liens authentiques : inconnus. | M16 | P1 |
| M35 | Retour de la cour au titre ou à la porte après un game over | absent | Voir M15 | M15 | P1 |
| M36 | Stages 2 à 18 (145 salles) | absent (explorateur seul) | `NAVIGATION_AUDIT.md`, `README.md` : fonds et cartes seulement ; tuiles 192-245 du stage 18 salle 9 non couvertes. | M03, M16, M14, M18 | P4 |
| M37 | Changement de tuileset et de palette par stage | porté pour l'explorateur ; partiel en jeu | `room-format.md` (palettes et tuiles par stage) ; `generated/rooms.c` `vk_stages`, `vk_collision_limits` (limites de solidité par stage) | M16 | P2 |

---

## 3. Dépendances

```
M16 (données salles, liens) ──┬── M07 (bords d'écran)
                              ├── M32 (cour) ── M33 (porte) ── M34 (hall) ── M36 (stages 2-18)
                              ├── M05 (escaliers, données)
                              └── M08c (spawn) ── M08b (ennemi) ── M11 (dégâts) ── M14 (vies) ── M15 (game over) ── M35
                                                       │                          │
                                                       └── M10, M19 (bougies) ──── M20 (ramassage) ── M21 (cœurs) ── M22 (sous-armes)
M12 (hitbox corps) ── M11
M14, M21, M23 ── M24/M25 (HUD)
M08b + M11 + M14 ── M13 (boss)
```

Points de blocage :
- **M16** est le prérequis commun. Sans liens de salle et tables de spawn authentiques, tout reste un test.
- **M08c** (spawn) est le blocage du premier ennemi. Sans lui, pas de dégât ni de bougie.
- **M14** (sens de `C410`) conditionne le HUD, le game over et la progression de vies.

---

## 4. Ordre de portage suggéré

Priorité estimée pour un portage jouable (P0 = fait ou bloquant immédiat).

1. **Recherche ROM prérequise (sans code)** : tracer les écritures de `D000/D001` en jeu (M16), tracer les écritures de `C410`/`C411` (M14, M21), et désassembler `5C25`, `5A2E`, `94BA`, `9507`, `4495`, `62D0` (section 0.2). Résultat attendu : liens de salle authentiques et sens de `C410`.
2. **Un ennemi authentique** (M08c puis M08b) avec hitbox de corps (M12) : traversée, animation 8 images par pose, spawn par table ROM.
3. **Dégâts, recul, invulnérabilité** (M11) reliés à `C408/C409/C40A/C40C/C413` (états 4 et 6).
4. **Vies, mort et game over** (M14, M15, M35) : état 6 → état 4 (vie perdue) ou état 7 (fin, retour titre).
5. **HUD minimal** : vies et énergie (M24, M25), sans score.
6. **Bougies et ramassage de cœurs** (M19, M20, M21), puis M23 (score).
7. **Porte et hall authentiques** (M33, M34, M07) : depuis la carte de liens confirmée en étape 1.
8. **Escaliers authentiques** (M05) : table de données à trouver (candidat : banque 14, données de salle).
9. **Sous-armes** (M22), en commençant par une seule arme.
10. **Sons d'événement** (M31) et musique en jeu (M29).
11. **Boss** (M13), une fois ennemis, dégâts et sous-armes stables.
12. **Stages 2 à 18** (M36) : grand chantier, à ne pas engager avant les étapes 1 à 8.

---

## 5. Inconnues majeures et mesures à faire

| # | Inconnue | Mesure proposée | Pourquoi c'est bloquant |
|---|---|---|---|
| U1 | Table de spawn des ennemis (position, type, déclencheur) | Watchpoints openMSX sur les blocs d'entités (`C420-C5FF`, `D600-D67F` déjà suivis par `tools/trace_entities_live.tcl`) pendant le passage des salles 1 à 7 du stage 1, avec le PC de l'écrivain et la banque active. | Bloque M08b, M09, M10, M19. |
| U2 | Sens de `C410` et `C411` (vies, cœurs, score ?) | En jeu : perdre une vie, ramasser un objet, mourir ; watchpoints sur `C410/C411`, relevé des états 4, 6, 8, 10. | Bloque M14, M15, M21, M24, M25. |
| U3 | Déclencheur exact des dégâts (quel état, quel bit) | Contact avec un ennemi (après U1) ; relever `C000`, `C408-C413`, `C40A/C40C`. | Bloque M11. |
| U4 | Liens authentiques entre salles (stage 1 et cour) | Tracer les écritures de `D000/D001` (candidats listés dans `Z80_ROOM_STATE_AUDIT.md`) en marchant sur chaque bord et en passant la porte. Relever la position de Simon avant et après. | Bloque M07, M33, M34. |
| U5 | Mécanisme de la porte du château | Même méthode que U4, à la porte de l'écran 2. | Bloque M33. |
| U6 | Table des escaliers et leur position | Recherche dans la banque 14 et les données de salle ; tracer l'entrée sur escalier (bit de `C42x` ou `C425/C427`). | Bloque M05. |
| U7 | Table des objets et des sous-armes | Après U1 : tuer ou casser une bougie, relever le pointeur d'objet écrit en RAM. | Bloque M19, M20, M22. |
| U8 | Menu d'état 13 (`C006 & 33`, `E60B`) | Ouvrir le menu (touche à identifier), relever `E60B` et les sorties vers `5CFD`, `5D82`, `5D61`. | Peut bloquer la sélection de sous-arme (M22). |
| U9 | Boss : emplacement, déclencheur, données | Après U1 et U4 : atteindre une salle de boss (stage à identifier) et relever les états. | Bloque M13. |
| U10 | Sens de « 3 zones » | Confirmer avec l'auteur : cour / hall / reste du château ? | Définit le périmètre d'un premier jalon jouable. |
| U11 | Sens de `E600` (drapeau de mode), `E60B` | Lire leurs écritures au démarrage et à la mort. | Peut conditionner le game over et la nouvelle partie. |
| U12 | HUD : positions et sources des valeurs | Dump VRAM des lignes 0-3 à chaque changement de vie ou de cœur (`research/whip-hitbox.md` a déjà une méthode de dump VRAM). | Bloque M25. |
| U13 | Contact et boîte du corps de Simon | Mesurer la VRAM ou les sondes de contact avec un ennemi (après U1). | Bloque M12. |
| U14 | Table de sons par événement | Déclencher chaque événement et noter les écritures de `C016`, `C096`, `C0A7` (méthode de `research/sound-triggers.md`). | Bloque M31. |

### 5.1 Contradictions relevées dans les notes existantes

- `research/input-gate.md` (section MD) dit que le port avance de 2 px par image. Le code actuel (`player.c`, `n < 1`) fait 1 px par image, ce qui correspond à la mesure MSX. Le passage est à corriger dans la note.
- `research/animation-inventory.md` (§1.2) dit que `flame_fx_md.h` n'est « pas issu de la ROM ». Le commit `19819d0` l'a remplacé par des tuiles extraites de la ROM (`research/courtyard-flames.md`). La note d'inventaire est à corriger.
- `README.md` dit que les flammes des nouveaux décors sont « statiques ». `main.c` les anime par `flame_clock`. Le README est à corriger.
- `research/castlevania-nes-comparison.md` cite des routines Z80 `6C2F`, `6D95`, `7B88`. Ces adresses sont valides pour la ROM MSX (pas la NES) ; la comparaison est à lire comme une comparaison de logique, non de données.
- `research/input-gate.md` donne la somme SHA-256 de la ROM extraite (`99eabaae`) et celle du zip (`afb01a5d`). Les deux sont distincts et ne doivent pas être confondus.

---

## 6. Prouvé / déduit / inconnu (résumé)

**Prouvé**
- Structure de la ROM : 18 stages, 153 salles dans les stages 1 à 18 (`castle-navigation/NAVIGATION_AUDIT.md`, `room-format.md`) ; cour = stage 0 (3 écrans).
- Dispatch d'états de `C000` (`414D`, table `4161`) et lecture des sorties de l'état 5 (`4257`).
- Marche : 1 px par VBLANK ; `C002 = 0x40` en jeu ; `C00B` n'est pas une porte de contrôle (`input-gate.md`).
- Ennemi observé en salle 1 du stage 1 : 1 px par image, 8 images par pose (`enemies-platforms.md`, 218 images).
- Absence totale de dégât, vie, mort, objet, sous-arme et boss dans le code MD.

**Déduit (lecture statique, non mesuré)**
- `C410` (BCD) : compteur de vies, de cœurs ou autre. État 6 : test de fin. État 7 : fin de partie.
- `6D4D` : candidat pour le recul.
- Le son `8B` (`42AB`) : candidat pour le game over.

**Inconnu**
- La table de spawn, les liens de salle authentiques, la porte, les escaliers, les objets, les sous-armes, les boss, le score, le HUD dynamique, les dégâts et la boîte de contact, les stages 2 à 18 en jeu.

---

## 7. Fichiers consultés

- Code : `sgdk/src/main.c`, `gameplay.c`, `player.c`, `player.h`, `room_manager.c`, `targets.c`, `whip.c`, `input.c`, `generated/flame_fx_md.h`, `generated/rooms.c`, `generated/stage1_collision.c`, `generated/player_assets.c`, `generated/sfx_md.c`, `generated/music_md.c`.
- Recherche : `research/enemies-platforms.md`, `animation-inventory.md`, `sound-triggers.md`, `room-format.md`, `player-format.md`, `intro-format.md`, `diagnostic.md`, `input-gate.md`, `courtyard-flames.md`, `castlevania-nes-comparison.md`, `loading-and-states.asm`, `known.asm`, `castle-navigation/*.md`, `RETROENGINEERING_NEXT.md`, `ENGINE_CHANGES.md`.
- ROM : `Vampire Killer (Japan, Europe) (En).rom` (dans `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/rom-ext/`, hors dépôt), lecture seule.
