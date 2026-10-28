# État du joueur : dégâts, recul, invulnérabilité, vies, mort, game over

Statut : **prouvé par trace openMSX** pour le contact ennemi, le recul, la fenêtre d'invulnérabilité, la perte d'énergie, la mort et la fin de partie jusqu'au titre. **Lu dans le code seulement** pour la table de dégâts et l'état des vies au-delà de ce qui est tracé. **Non vérifié** : affichage HUD (vies, énergie), fin de partie avec continuation, chute dans le vide.

Adresses CPU. Banque fixe `4000-5FFF` (banque 0) ; `6000-7FFF` et `8000-BFFF` dépendent du mapper. Toutes les adresses de routines de cette note ont été relues dans le dump mémoire CPU pris pendant le jeu (mapping réel), sauf mention contraire. ROM de référence : SHA-256 `99eabaae...`, la même que `research/enemies-platforms.md`.

## 1. Octets

| Adresse | Rôle retenu | Statut |
|---|---|---|
| `C415` | **Énergie de Simon**. Maximum `20` (32). Chaque contact ennemi retire `2` dans les runs ; à `0`, la mort est déclenchée. Remise à `20` par `62D0`. | prouvé (trace) |
| `C418` | Seconde jauge, remise à `80` (128) avec `C415` dans `62D0` et au début de partie (`44E5`). Plafond `80` dans `4618`. Rôle inconnu. | prouvé (écriture), rôle inconnu |
| `C420` | **Drapeau de recul** : `5` pendant la phase de recul (écrit par `7EC3`, `7F1D`). Les boucles de collision ennemi sont désactivées tant que `C420 >= 5`. Remis à `0` en fin de recul (`702D`, `6F76`). | prouvé (trace) |
| `C423` | **Phase du recul** : `1` au coup (`6FC3`), `3` à l'atterrissage (`6FFC`), `0` en fin (`7030`, `7EC7`). | prouvé (trace) |
| `C42D` | **Chronomètre d'invulnérabilité** : `5A` (90) écrit à l'entrée du recul (`6F9F`), décrémenté ensuite (`75D2`). Tant qu'il est non nul, aucune boucle de collision ennemi n'est exécutée (`7E6D`, `7EE4`). | prouvé (trace + garde) |
| `C42A`, `C42E`, `C42C` | Compteurs d'animation et d'arc du recul (`C42A` compte 4 ou 16 selon l'énergie, `6FFF`). | lu dans le code |
| `C428`, `C421` | Chronomètre `5A` et drapeau à `70A0-70B2` (sons `00` et `89`). Non lié à la garde d'invulnérabilité. | inconnu |
| `C413` | **Drapeau de vie** : `1` quand la partie est active (écrit en `422C`, puis `4240`). Remis à `0` par `70D9` dans la routine de mort. Quand `C000=5` et `C413=0`, le jeu passe en état 6. | prouvé (trace) |
| `C410` | **Compteur de vies** (BCD). Initialisé à `3` au début de partie (`44DD`). Décrémenté par `422C` (`4233`) à chaque entrée en état 4. Testé en état 6 (`4294`) : `0` = fin de partie. | prouvé (trace), sens « vies de réserve » non établi |
| `C411` | Incrémenté avec `C410` dans le chemin d'état 10 (sortie d'écran). Rôle inconnu (cœurs ?). | lu dans le code, trace partielle |
| `C408`, `C409`, `C40A`, `C40C`, `C41B` | Sorties de l'état 5 (voir `research/mechanics-remaining.md` §1). `C408=1` (écrit en `7802`) mène à l'état 10 ; `C41B=4` (écrit en `7715`) mène à l'état 9 (défilement). | prouvé (trace) |
| `C434`, `C43A` | Gardes de la routine de contact (`7EB0`, `7F14`, `7EE4`). Valeurs nulles dans tous les runs. | lu dans le code |
| `C43C` | Sens du recul (copie de `C42C`, écrit à `7EC1`/`7F27` depuis le bit 7 de `IX+10` de l'ennemi). | lu dans le code, valeur observée `0` à `1` |
| `C701` bit 4 | Influe sur le dégât (chemin `817E`). Valeur `00` au coup observé. | lu dans le code |
| `C0A7` | Requête de musique/jingle. `7` est écrit par `5136` à la mort (`8B` en état 6, ou fin de recul à `0` énergie). | prouvé (trace) |
| `E600` | Valeur à `0` en partie (`40A5`) ; chemin « continuer » (`42B4`) inaccessible dans les runs. | prouvé (trace), rôle inconnu |
| `C002` bit 6 (`40`) | Porte d'entrée. Effacé en `42CD` (fin de partie). | prouvé (trace) |

## 2. Routines

| Adresse | Rôle | Statut |
|---|---|---|
| `7E7D` (boucle `C800`, 7 entrées) | Test de collision entre Simon et les objets `C800` (type `IX+0`, actifs si non nuls). Garde : `C42D=0` (`7E6D`), `C420<5` (`7E77`). | prouvé (garde, trace `run6`) |
| `7EFA` (boucle `D700`, 8 entrées) | Même rôle pour les objets `D700`. Garde : `C42D=0` (`7EE4`), `C420<5` (`7EF4`). | prouvé (garde, trace `run6`) |
| `7EB0`-`7EC1`, `7F14`-`7F3D` | Branches après un chevauchement : `C434≠0` ou non. Sans `C434`, chemin de contact : `7EC3` écrit `C420=5`, `7EC7` écrit `C423=0`, puis `816C`. | prouvé (`7EC3`) ; garde lu |
| `816C` | Calcule le dégât `B` depuis la table `81CE` indexée par `2*(type-1)`, doublée selon la comparaison X (`8180`-`8196`). | lu dans le code ; table à partir du dump |
| `462B` | `C415 -= min(B, C415)` (plancher à `0`). | prouvé (trace PC `4638`) |
| `4605` | `C415 += B`, plafond `20` (soin, appelant non identifié). | lu dans le code |
| `6F9x` (entrée non identifiée) | Début du recul : `C42D=5A` (`6F9F`), `C423=1`, `C42E/F` animation, `C43C` copié dans `C42C`. Si `C42B≠0` et `C415≠0`, chemin `765F` avec `C420=3`. | prouvé (écritures) ; appelant non identifié |
| `6D4D` (table de delta de Y) | Arc vertical du recul, `C425` : `C0 -> BD -> B0 -> C0` (run2). | prouvé (écritures `6D5D`) |
| `6C98` | Déplacement horizontal (marche et recul). Pendant le recul, `C427` diminue de 2 par image. | prouvé (écritures `6C98`) |
| `6FFF`-`7016` | Fin de la phase d'impact : `C42A=4` si `C415≠0`, sinon `10`. | lu dans le code |
| `7030`-`7039`, `702x` | Fin du recul : `C420`, `C421`, `C423`, `C422`, `C42A`, `C428` remis à `0`. | prouvé (trace `7030`) |
| `70B0`-`70E5` | **Finalisation de la mort** : `C413=0` (`70D9`), `C434`, `C43A`, `C421`, `C422`, `C416=0`. | prouvé (trace `70D9`) |
| `4251` (dans `4249`) | Passage d'état avec `C004=20` puis `INC C000`. Sert à passer en état 6 ou 7. | prouvé (trace) |
| `4294` (état 6) | Si `C410≠0` : `62D0` (refill), `C420=0`, `C40D=1`, état 4. Sinon : jingle `8B` (`509F`), puis état 7. | prouvé (trace) |
| `422C` (état 4) | Décrémente `C410` (BCD), `C413=1`, appelle `794F`, puis état 5. Appelé à chaque entrée en état 4 : démarrage, sortie d'écran (via 10), mort. | prouvé (trace) |
| `42B2` (état 7) | Attend `C004` puis `C0A7=0`, efface `C002` bit 6, `C000=0` (titre). Chemin `42D3` (`C410=3`) seulement si `E600≠0` et `4314` (touche SNSMAT ligne 7 bit 1, front montant) : non tracé. | prouvé (fin de partie), chemin continuer non tracé |
| `62D0` | Remise de `C415=20`, `C418=80`, puis `70DC`, `548D`, `5768`, `53B6`. Appelé en `4294` (mort avec vies), `4211` (état 3), `4E17` (entrée de partie). | prouvé (trace PCs `62D2`) |
| `44D2`-`44E5` | Initialisation de partie : `C410=3`, `C415=20`, `C418=80` (PCs `44DD`, `44E0`, `44E5`). | prouvé (trace) |

## 3. Mécanismes

### 3.1 Contact ennemi, dégât et recul (M11) : prouvé

- Premier contact (run 3, 40,617 s, image 2191) : Simon est à `X=136` (`C427`), l'ennemi actif est l'objet `C800` slot 0 (type `01`). `7EC3` écrit `C420=5`, `462B` écrit `C415` de `20` à `1E` (`PC 4638`).
- Recul : `C425` fait un arc de `C0` vers `B0` puis revient à `C0` (écritures `6D5D`), et `C427` décroît de 2 par image jusqu'à `5C`. Fin à 41,91 s (`C423=0`).
- `C423=3` à l'atterrissage (41,38 s), puis `C423=0` et `C420=0`.
- Pas de dégât pendant la fenêtre d'invulnérabilité (voir 3.2).
- Dégât observé : **2** pour un contact (2 coups : `20 -> 1E -> 1C` ; 4 coups au total dans run 3 : `20 -> 1E -> 1C -> 1A -> 18`).
- Dégât : `B` vient de la table `81CE` par type ; le type 1 vaut `1` dans la table et `816C` peut le doubler (chemin `8193`). La valeur `2` est donc cohérente avec le type 1 doublé. **Déduit**, non vérifié par le registre `B`.

### 3.2 Invulnérabilité (M11) : prouvé

- `6F9F` écrit `C42D=5A` à 40,643 s. La garde `7E6D` (et `7EE4`) interdit l'entrée dans les boucles de collision tant que `C42D≠0`.
- Mesure (run 6, journal des entrées de boucle) : la boucle `C800` n'est pas entrée entre 40,62 s et 43,62 s, soit **3,00 s (180 images)** de fenêtre, puis reprise à `C42D=0`. Le pas de décrémentation mesuré est de 1 toutes les 2 images (run 3 : `89` à l'image 2193, `88` à 2195).
- Aucune boucle de collision n'a été vue avec `C42D≠0` ou `C420≠0` (`0` occurrence sur 1041 entrées, run 6).
- Le second contact arrive à 46,49 s, après la fin de la fenêtre.
- Hypothèse non testée : le clignotement de Simon est porté par `C421`/`C428` ou par `C42D`. Non établi.

### 3.3 Vies et énergie (M14) : prouvé en partie

- Début de partie (run 3, 12,28-12,33 s) : `C410=3`, `C415=20`, `C418=80`.
- Premier passage en état 4 (démarrage, 18,66 s) : `C410` passe de `3` à `2`. Le passage en état 4 est donc aussi le chemin de démarrage ; le même `422C` est appelé à chaque sortie d'écran et à la mort.
- Sortie d'écran à droite (run 1, 38,0 s, état 5 vers 10 vers 4) : `C410` monte à `3` puis redescend à `2`. Le net est nul.
- Mort avec vies restantes (run 4, état 6 vers 4) : `C410` passe de `2` à `1` (45,27 s, `4233`).
- Hypothèse cohérente avec tout ce qui est tracé : **affichage des vies = `C410 + 1`** (3, 2, 1, puis fin). Le HUD n'a pas été lu : **non établi**.

### 3.4 Mort (M15, M35 partiel) : prouvé

Chronologie du run 4 (énergie forcée à 2 à 40 s, un contact amène `C415` à 0) :

1. 40,617 s : contact, `C415=0`. Recul normal (`C420=5`, `C423` 1 vers 3 vers 0).
2. 41,94 s : `C0A7=7` (`5136`) pendant le recul, avec `C415=0`.
3. 44,814 s : `C413=0` (`70D9`), après un délai d'environ 4,2 s depuis le contact.
4. 44,824 s : passage en état 6 (`4251`), `C000=6`.
5. 44,841 s : `62D0` (refill) : `C415=20`, `C418=80`. Vies restantes, donc chemin vie perdue.
6. 45,262 s : `41B6` : état 4. `C420=0` et `C40D=1` par `42A3`.
7. 45,274 s : `422C` décrémente `C410` (`2 -> 1`). Puis état 5 avec `C413=1`.

Fin de partie (run 5, `C410=0` forcé à 40 s, `C415=2`) :

1. 40,617 s : contact, `C415=0`.
2. 44,814 s à 44,824 s : identique au run 4 (`C413=0`, état 6).
3. 44,841 s : état 6 avec `C410=0`, jingle `8B` (`509F`, `C0A7=7` par `5136`), passage en état 7 (`4251`).
4. 44,975 s : état 7, `C004=78` (120). Compte à rebours sur `42C4`.
5. 51,13 s : `C002` bit 6 effacé (`42CD`), `C000=0` (`41B5`) : retour au titre.
6. Ensuite titre (état 1, puis 2) : hors périmètre.

Chute dans le vide : **non testée**. La cause de `C000=9` observée sur le bord droit est un défilement (`C41B=4` puis état 9 et retour en 5 à `X=10`), pas une mort.

### 3.5 Game over et retour au titre (M15, M35) : prouvé jusqu'au titre

Voir 3.4 (run 5). Le chemin « continuer » (`42D3` : `C410=3`, `C405/C407=0`) demande `E600≠0` et une touche SNSMAT (`4314`). `E600` est à `0` en partie dans tous les runs ; ce chemin **n'a pas été exercé**.

## 4. Scénarios et preuves

| Run | Script | Scénario | Preuve obtenue | Données |
|---|---|---|---|---|
| 1 | `tools/player_health_trace.tcl` (`natural`) | Marche à droite, contacts naturels | Premier passage en état 4 (démarrage), `C410` 3 vers 2 ; sorties de salle 5 vers 9, 5 vers 10 | `scratchpad/health/run1/` |
| 2 | version antérieure | Dump `C400-C5FF` par image | Contact à 40,617 s, `C420=5`, recul ; `C415` 20 vers 1E | `scratchpad/health/run2/` |
| 3 | `natural` étendu à 200 s | Quatre contacts (`20->1E->1C->1A->18`) | Dégât `2` par contact ; fenêtre d'invulnérabilité | `scratchpad/health/run3/` |
| 4 | `death` | Énergie forcée à 2 | Mort avec vies restantes : `C415=0`, `C413=0`, refill, `C410` 2 vers 1 | `scratchpad/health/run4/` |
| 5 | `gameover` | Énergie à 2 et vies à 0 | Fin de partie, état 7, retour au titre à 51,13 s | `scratchpad/health/run5/` |
| 6 | garde | Journal d'entrée dans les boucles de collision | Fenêtre de 3,00 s sans collision | `scratchpad/health/run6/` |

Toutes les exécutions sont déterministes entre elles (contact à 40,617 s dans run 2, 3, 4, 5, 6). Le script committé reproduit le run 5 (`VK_SCENARIO=gameover`).

Méthode : la recherche statique des références aux octets candidats (`scratchpad/health/refs.py`, 424 hits) a servi à localiser les écritures ; les PCs réels ont été relevés par watchpoints (`debug set_watchpoint write_mem`). Les routines ont été désassemblées sur le dump mémoire CPU du moment (`scratchpad/health/dis.py`), donc avec la banque réelle.

## 5. Inconnues

1. Affichage HUD : vies, énergie `C415`, jauge `C418`. Lecture VRAM à faire (U12, `research/mechanics-remaining.md`).
2. Sens exact de `C410` (vies de réserve, ou vies totales ?) : confirmer par HUD.
3. Sens de `C411` (cœurs ?) et `C418` (seconde jauge ?).
4. Enregistrement du type ennemi à la table `81CE` et de l'enemy actif lors du contact (type `01` observé dans `C800`).
5. Valeur de `B` au moment de `462B`. Le dégât `2` est déduit de la table et de la doublure.
6. Appelant de `6F9x` (début de recul) et de `4605` (soin).
7. Cadence exacte de décrémentation de `C42D` (1 pas par 2 images dans run 3 et 6, 1 par image dans run 4 pendant la mort).
8. Chemin « continuer » (`E600`, `4314`, `42D3`) : non exercé.
9. Chute dans le vide (M18) : non testée.
10. Rôle de `C421`/`C428` (chronomètre `5A`, son `89`).
11. Table `81CE` complète, et la routine `4E20` (état 2) qui lit `C413`.

## 6. Fichiers

- Script de trace committé : `tools/player_health_trace.tcl` (variable `VK_SCENARIO=natural|death|gameover`, sortie dans `VK_OUT`).
- Analyse : `tools/player_health_analyze.py <dossier>`.
- Sorties et outils de travail (non commités) : `scratchpad/health/` (`refs.py`, `dis.py`, `xref.py`, `dumpdiff.py`, `ev.py`, runs 1 à 6).
- Ce document : `research/player-health.md`.

## Chute hors de la salle (lecture du code, 2026-10-28)

**Prouvé (code)**, non tracé :
- `76BC` : BAS (`Y >= 225`) sans salle en dessous (`C41D = FF`) : `C421 := 0`, `C420 := 6`, `Y := FA` (hors écran), `C428 := 1E` (30 images) aux stages 2 et 10 (l'eau), avec le son 9 (`509F`) ; sinon `C428 := 1`.
- `767B` et les routines de Simon rendent la main tant que `C420 = 6`.
- `7093` (`C420 = 6`) :
  - phase 0 : `C428` décompte jusqu'à 0, puis `C428 := 5A` (90 images), `C421 := 1`, arrêt de la musique (son 0) et jingle 89 ;
  - phase 1 : à la fin des 90 images, les cœurs `C417` sont ramenés à 5 au plus (`70B4`), puis la remise à zéro de `70BF` met `C413` à 0 (mort, état 6). Cette remise à zéro efface aussi `C416` (arme), `C700` (clés dorées), les bits de `C701` sauf le bit 7, et met `D001` à 0.

**Port** (`gameplay.c` `VK_EVENT_FALL`, `health.c` `vk_health_fall`) :
- Simon est garé en Y FA et ne bouge plus ; l'état 6 arrive 30 + 90 images après la chute dans l'eau (1 + 90 ailleurs) ;
- les cœurs sont ramenés à 5 au plus.

**PROVISOIRE** :
- ni le son 9 ni le jingle 89 ne sont joués ;
- la remise à zéro du reste de `70BF` (minuteries `C431`/`C432`/`C441`/`C442`, `C702`) n'a pas d'équivalent porté.

## Nouvelle vie (état 4, lecture du code)

**Prouvé (code)** :
- `422C` (état 4) appelle `62E6`, qui recharge le flux du stage (`5A49`).
- Toutes les bougies et tous les objets fixes du stage reviennent donc, y compris la clé blanche dans son bloc caché.
- La mort (`70BF`, `70DC`) a déjà remis `C416` (arme) et `C700` (clés dorées) à 0, et gardé seulement le bit 7 de `C701`.
- Les cœurs `C417` ne sont pas touchés, sauf le plafond de 5 après une chute.

**Port** : `vk_items_new_life` et `vk_candles_new_life`, appelés par `respawn()` dans `main.c`, avec le retour du fouet au niveau 0.
