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
- Clignotement de Simon : porté par `C42D` et le bit 1 de `C003` (`78CC`), prouvé par trace (voir « Cadence » plus bas).

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

### 3.6 Écran de fin de partie (port, 2026-10-29)

`42E3` (état 7) : `47B9` efface l'écran, texte « GAME  OVER » (`4D3A`), HUD redessiné par `4513`, attente `C004 = 0x78` images. Le port (`show_game_over` dans `main.c`) cache les sprites, efface `BG_A` (lignes 4 à 27) et `BG_B`, écrit « GAME  OVER » en (88, 88) avec la police MSX (`msx_text.c`) ; seule `step_health()` tourne pendant cet état (scénario `d`). « F5 _ CONTINUE » (`4300`) n'apparaît que si `E600 ≠ 0`, posé par `5C92` quand une seconde cartouche Konami est présente : sans objet sur Mega Drive. PROVISOIRE : l'icône de clé dorée du HUD n'est pas réaffichée.

### 3.7 Pause (port, 2026-10-29)

START en partie (Simon vivant ou mourant) : son FD (`51C4`), texte « PAUSE » en (108, 88) sur `BG_B`, jeu figé, seul START est lu ; un second START joue FE (`522D`) et reprend. A+START retourne au titre (raccourci de debug). Scénario `o_pause`.

### 3.8 HUD : ligne du haut et score (lecture du code, port 2026-10-29)

`4513` redessine tout le HUD : textes fixes de `4C00` par `4ACB` (police MSX, page 1, codes = ASCII − 0x10, tiret = code 0x30, cœur = code 0x50) : « SCORE- » (8, 0), « PLAYER » (8, 12), « ENEMY » (8, 20), cœur et tiret (176, 0), « P- » (212, 0), « STAGE- » (108, 0). Valeurs en BCD par `4578` (deux chiffres par octet, 8 px par chiffre) : score `C407..C405` en (56, 0) (`4531`), stage `C411` en (156, 0) (`453B`), cœurs `C417` en (192, 0) (`4566`), vies `C410` en (228, 0) (`456E`). Barres : PLAYER `2 × C415` px en (60, 14), 4 px de haut, couleur 11h (`45D1`) ; ENEMY d'après `C418` (`45E5`).

- `44C6` (nouvelle partie) remet `C405..` à 0, puis `C410 = 3`, `C411 = 0`, `C412 = 1` : la cour affiche STAGE-00. La sortie de stage (`434F`) incrémente `C410` et `C411` (puis l'état 4 décrémente `C410`).
- Score : seul appelant de l'addition `44EE` (E, D, C = octets BCD ajoutés à `C405`, `C406`, `C407` ; 999999 en cas de retenue ; rien si `C002` bit 7) : `81AB`, à la mort d'un ennemi (`7DF9` fouet, `7E21`, `7EB6`, `8072` arme). Points = premier octet de la table `81CE` indexée par le type − 1, en centaines (type 1 : 100, 2 : 200, 4 : 100, 5 : 100 ; types 11h et 17h : + 30 000 et + 50 000).
- Port : `hud.c` (`vk_hud_show`, `vk_hud_status`, `vk_hud_add_score`), composition des caractères à n'importe quel X par `msx_text.c`. Le HUD entier est dessiné avec la salle ; la réapparition met les vies à jour avant de dessiner la salle (état 4, `4237`). Scénarios `b` (100 points) et `d` (P-01 à la réapparition).
- Barre PLAYER : `hud.c` gardait une reconstruction des 27 tuiles pixel par pixel à chaque changement d'énergie, plus longue qu'une image de 68000 : à chaque contact, le jeu perdait une image (ennemis figés une image). Les tuiles ROM et les masques des pixels de remplissage sont maintenant calculés une fois (`prepare_bar`), une mise à jour ne fait que des ET par ligne. Vérifié par le scénario `c` (parité des ennemis à l'image qui suit le contact).

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
- L'icône de la clé dorée reste affichée jusqu'au HUD redessiné par `4513` en `4237` (état 4) ; à la fin de partie, `4513` (`42F8`) ne la redessine pas puisque `C700 = 0` (lecture du code, `castle-candles.md` 9.7). Porté : `vk_candles_death` à l'état 6, `vk_candles_hud` à la nouvelle vie et au GAME OVER.
- Les cœurs `C417` ne sont pas touchés, sauf le plafond de 5 après une chute.

**Port** : `vk_items_new_life` et `vk_candles_new_life`, appelés par `respawn()` dans `main.c`, avec le retour du fouet au niveau 0.

## Recul au coup reçu : machine d'états (lecture du code, 2026-10-28)

**Prouvé (code, banque 1 en 6000)**, non encore porté :
- Contact (`7EC1`, `7F1B`) : `C420 := 5`, `C423 := 0`, `C422 := 0` (fouet coupé), `C43C := 1` si le bit 7 de `IX+10` de l'ennemi est à 0, sinon 0.
- `C420 = 5` : aiguillage sur `C423` (table `6F8B`) : 0 → `6F93`, 1 → `6FD4`, 2 → `7017`, 3 → `701D`.
- Phase 0 (`6F93`) : son 0x13 (si `C002` bit 6), `C42D := 5A`. Sur un escalier (`C42B ≠ 0`) avec de l'énergie : `C420 := 3`, `C42F := 2`, pas de recul. Sinon `C42C := C43C`, `C423 := C43C + 1`, `C42A := 0`, pose `C42E/C42F := 07/03` (+10/+15 tournée à gauche, `765F`).
- Phase 1 (`6FD4`) : `6C2F`, 2 px vers la gauche (`C42C := 1` ; 2,5 px si `C431` bit 3), arrêt au mur `7C05` ou à x < 16 sans salle à gauche. Phase 2 (`7017`) : `6C53`, même chose vers la droite (`C42C := 0`, mur `7BA9`, x ≥ F0 sans salle à droite).
- Arc (`6D4D`) : `C42A += 1`, Y += table `707D[min(C42A, 21) - 1]` = `FD FD FE FE FE FF FF FF FF 00 00 00 00 01 01 01 01 02 02 02 03` (montée de 16 px, puis 3 px par image au-delà).
- À partir de `C42A ≥ 11`, sol testé (`7B88`) : à l'atterrissage, Y &= F8, `C423 := 3`, `C42A := 4` (16 si `C415 = 0`), pose +1 (08/04).
- Phase 3 (`701D`) : `C42A` décompte ; à 0, `C420`, `C421`, `C423`, `C422`, `C42A`, `C428 := 0` : retour au contrôle.
- Simon ne répond à aucune touche pendant `C420 = 5`.

**Porté** (`gameplay.c` `vk_game_hit` / `vk_recoil_tick`, appelé par `actors.c` au contact ; `enemies.c` garde seulement le drapeau `recoil` = `C420 = 5` qui coupe le contact) :
- une passe toutes les 2 images, comme l'état 5 du MSX (tick 30 Hz `C003`) : la passe de pose, puis l'arc de `707D` à 2 px par passe, le sol testé à partir du 11e pas, l'atterrissage avec Y aligné sur 8, puis 4 passes au sol (16 à énergie nulle) ;
- touches ignorées pendant tout le recul : pas de saut, pas de marche, pas de fouet (`main.c`), le fouet en cours est coupé (`C422 := 0`) ;
- poses de la ROM (`tools/extract_player.py`, poses 8 à 11) : 07/03 (poussé à gauche, Simon tourné à droite), 17/18 (poussé à droite), puis 08/04 et 18/19 au sol. Ces poses montent un pixel plus haut que les autres et sont dessinées en y − 32.
- Vérifié : la chute de l'arc retombe au sol après 22 pas, soit 44 px, comme la trace (X 136 → 92) ; tests `test_gameplay.c` et scénario `c_enemy_contact`.

**Coup reçu sur un escalier (prouvé, trace du 2026-10-29)** : `scratchpad/fidelity/out3` (hall salle 1, volée A, Simon arrêté sur les marches, points d'arrêt `6FAF` et `6FBC`, `C420`/`C42B`/`C42E`/`C42F` par image). Cinq coups avec de l'énergie (images 2106, 2362, 2564, 2820, 3028, `C42B` = 8 ou 16) passent par `6FAF` : `C420 = 5` pendant une passe (2 images, 2105-2106), puis `C420 = 3` ; X, Y et `C42B` inchangés, `C42D = 5A`, son 13 ; la montée reprend ensuite normalement. Un coup à énergie nulle sur l'escalier (3852, `C415` 2 → 0) passe par `6FBC` : recul ordinaire, Simon quitte l'escalier (64,176) et l'arc le pose au sol en (8,192).

**Porté** (`vk_game_hit`, état `VK_RECOIL_STAIRS`) : sur un escalier avec de l'énergie, 2 images sans contrôle ni pas d'escalier (et sans contact, `vk_game_recoiling`), Simon reste sur sa marche ; à énergie nulle, `on_stairs := 0` et recul ordinaire. Test `test_gameplay.c` (volée A de la salle 1).

**Pose au coup sur l'escalier (prouvé par trace, porté le 2026-10-29)** : `6FB4` met `C42F := 2` (+15 à gauche, `765F`) sans toucher `C42E`. Le port affiche maintenant les poses d'escalier de la ROM (`research/stairs.md` §2.2) : `04/01` devient `04/02`, les autres poses ont déjà le haut 2. Test `test_gameplay.c`. Trace `scratchpad/trace3/out3` (coups forcés par `C420 := 5` au VBLANK, `tools/run_stair_recoil_trace.py`, `VK_TRIG`) : montée à droite, `C42B` = 2, `04/01` → `04/02` (image 2005) ; descente à gauche, `C42B` = 18, `14/16` → `14/17` (2231) ; montée à gauche (volée C), `C42B` = 3, `14/16` → `14/17` (2725). `C42E`, X, Y et `C42B` inchangés ; au pas suivant la pose d'escalier est réécrite. Le coup tombant au milieu d'une cellule, le pas en cours reprend après la passe du coup jusqu'au bord de cellule (`C435`, voir `research/stairs.md` §2.2).

**Fin du recul et mort (lecture du code, port 2026-10-29)** : à la fin de la phase 3 (`702A`), `C42C ^= 1` (`704B`, sauf `C5AC` = 2, 3 ou 5) : Simon se retrouve tourné vers le coup ; le port inversait l'orientation, c'est corrigé. Si `C415 = 0` (`7053`) : X < 10h avance de 8 px, `C42D := 0`, `C428 := 1`, `C420 := 6`, pose `C42E/C42F := 09/05` (+10/+15 tourné à gauche, `765F`) : les restes de Simon, deux demi-sprites côte à côte (32 × 16 px, `tools/extract_player.py`, `vk_player_death_tiles`). L'état 6 (`7093`) attend `C428` puis lance l'arrêt et le jingle 89 (`70A0`), soit environ 79 images après le contact (recul de 39 passes), ce qui recoupe la mesure de 79 images. Port : état `VK_RECOIL_DEAD`, pose `VK_PLAYER_DEATH_POSE` jusqu'à la réapparition ; test `test_gameplay.c`, scénario `d` (pose à 74-84 images du contact).

**Cadence (prouvé, trace)** : dans la trace du coup à énergie nulle (`out3`, images 3851-3909), l'état 5 fait une passe toutes les 2 images : pose à 3851, premier pas à 3853 (visible à 3855), X 64 → 8 et Y par la table `707D` en 27 passes, atterrissage à 3909. Le port suit cette cadence depuis le 2026-10-29 (`recoil_wait`, tests `test_gameplay.c` et scénario `c`). Clignotement pendant `C42D` (`78CC`) : Simon caché (Y `E0`) quand le bit 1 de `C003` est mis, 4 images sur 8. **Phase prouvée (trace openMSX du 2026-10-29, `tools/run_simon_blink_trace.py`, détails dans `castle-candles.md` 9.8)** : `C42D` = 5Ah posé par `6F9F` puis baissé par `75CC` dans la passe du coup, `78CC` le teste ensuite : Simon est caché dès la passe du coup si le bit 1 de `C003` y est mis, puis pendant les passes 0 à 88 (`C42D` après baisse non nul) selon ce bit ; l'image affiche les tampons de la passe précédente (coup à l'image 1788, passe `C003` = 67 : caché aux images 1790-1791, montré 1792-1795...). Le port suit le bit 1 de `tick` (`Enemies_simonHidden`), figé à chaque passe.

**Bord de salle et pas de 2,5 px (lecture du code, porté le 2026-10-29 ; sortie de salle prouvée par trace)** :
- `6C34`/`6C57` : la borne X < 16 / X ≥ F0 ne vaut que sans salle de ce côté (`C41E`/`C41F` = FF). Avec une salle, rien ne borne X, et le test de sortie `767B` (appelé par `6B24` après l'aiguillage de `C420`, retour seulement si `C420 = 6`) tourne aussi pendant le recul : X < 8 donne la salle de gauche à X F6, X ≥ F8 celle de droite à X 0A. Port : `vk_recoil_exit` (`gameplay.c`), le recul continue dans la nouvelle salle après le gel de l'état 9. Test `test_gameplay.c` (salle 1 → salle 0, atterrissage à F6 − 30), scénario `r_recoil_room_exit` (hall 0 → 3). Les scénarios `c` et `d` placent Simon en X 136 (runs 3 et 4) pour rester dans la salle 0. Le cas tracé du coup mortel sur l'escalier (64,176) → (8,192) est reproduit exactement (sans sortie).
- **Prouvé (trace `scratchpad/trace3/out1`, 2026-10-29, `tools/run_stair_recoil_trace.py`, plan : cour à droite jusqu'au hall, puis Simon immobile en X 16 de la salle 0)** : contact naturel d'un ennemi venu de la droite à l'image 1786 (`7EC3` `C420 := 5`, `7ED6` `C43C := 0`), pose `07/03` à 1788 (`6FC3` `C423 := 1`, `6FC7` `C42A := 0`), puis un pas toutes les 2 images (`6C98` : X 16 → 14 → 12 → 10 → 8 → 6, `6D4D` : `C42A` 1 à 5, Y 192 → 182). Dans la passe où X devient 6 (1798), `7709` écrit X := F6 ; `C000 := 9` à 1799 (`41B6`), `D001 := 3` à 1800 (`B987`), état 9 pendant 22 images, `C000 := 5` à 1821. `C420` = 5, `C423` = 1, `C42A` = 5, `C42C` = 1, `C42D` (gelé à 84) et Y 180 traversent le chargement de salle sans écriture. Le recul continue dans la salle 3 (1822 : X F4, `C42A` 6, Y suivant la table `707D`) jusqu'à l'atterrissage à `C42A` = 21 en (212,192) = (F6 − 2 × 17, 192) à 1855, 4 passes au sol puis retour au contrôle à 1863 (`C42C` 1 → 0, Simon tourné vers le coup). Le port (`vk_recoil_exit`, scénario `r_recoil_room_exit` : sortie 11 images après le contact, X F6, atterrissage en F6 − 34, Y 192) est conforme.
- `6C44`/`6C67` : pas `BC` = FE00 / 0200 en 8.8 sur `C426-C427`, FD80 / 0280 (2,5 px) si `C431` bit 3. `C431` bits 2, 3, 4 sont mis par les objets 0B, 0C, 0D (`8DF5`-`8E03`, effet 12), remis à 0 par la mort (`70E8`). Port : `VKCandles.c431` (objet 0C du coffre de la salle 4 du hall), copié dans `VKGame.items` ; fraction `VKGame.x_frac` (PROVISOIRE : `C426` n'est mis à jour que par le recul, la marche du MSX l'écrit aussi). Bits 2 et 4 sans effet porté.
- la plateforme mobile (`C439`) n'est pas portée ;
- le son 0x13 du coup n'est pas joué par ce module (branchement du son à part).
