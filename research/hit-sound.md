# Son du coup de fouet sur bougie ou objet (MSX2)

Statut : **prouve** pour le numero d'effet joue au contact d'une torche de la cour (effet `0C`), et pour la routine qui le demande (`7FD8`, table `C470`). **Deduit** pour l'identite du son (`SFX_14` de castlevania-md). **Inconnu** pour l'appelant de l'effet `0F` (ramassage probable) et pour la correspondance numerique avec l'index NES.

ROM de reference : `Vampire Killer (Japan, Europe) (En).zip` (racine), SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`, verifie sur la copie utilisee (`scratchpad/hit-sound/game.rom`, copiee depuis `scratchpad/rom/`, meme hash). Adresses CPU Z80.

Script : `tools/hit_sound_trace.tcl` (scenario identique a `tools/candle_drop_trace.tcl` : fouet a vide x=36 (torche A), fouet a vide x=100, fouet sur la torche B x=168, cour salle 1). Lecture seule.

## 1. Resultat

| Question | Reponse | Statut |
|---|---|---|
| Numero d'effet du coup sur objet | **`0C`** (decimal 12), ecrit dans `C096` | **prouve** (trace, 2 runs) |
| Effet du fouet dans le vide | `05`, ecrit dans `C096` | **prouve** |
| Routine qui demande l'effet objet | `7FD6` : `LD A,0Ch` ; `7FD8` : `CALL 509Fh` (retour `7FDB`) | **prouve** (retour lu en pile + octets ROM) |
| Routine de la table des objets | `sub_7FB7` (CPU `7FB7`-`7FD8`), boucle sur la table `C470` (8 entrees de `10` octets) | **prouve** (code) ; table `C470` = objets de salle, **deduit** de `research/castle-candles.md` |
| Effet joue aussi a la destruction ? | Non, aucune ecriture de `C096` quand la flamme disparait | **prouve** pour ce cas (1 cas) |
| Effet joue au ramassage ? | Un effet `0F` est ecrit dans la meme image que le ramassage du coeur, appelant non identifie | **deduit** (coincidence d'une image) |

## 2. Chronologie prouvee (run `run4`, image VBLANK `n`)

| Image | Phase | Evenement | PC ecrivain | Preuve |
|---|---|---|---|---|
| 561 | walk1 | `C096 = 05` (touche SPACE de fin de titre, fouet a vide) | `517D` | trace |
| 1079 | whipE (x=100, vide) | `C096 = 05`, `C016 = 9C` ; fin a 1092 (`C096 = 00`, PC `89D7`) | `517D`, `519E`, `89D7` | trace |
| 1207 | whipB (x=168, torche B) | `C422` 02 vers 03 a 1209 ; `C096 = 05` (1207) puis **`C096 = 0C` (1210)**, `C016 = 9C` | `517D` | trace |
| 1210 | whipB | `C800 = 26` (objet flamme cree) dans la meme image que `C096 = 0C` | `5F82` (C800) | trace |
| 1222 | whipB | `C096 = 00` (fin de l'effet) | `89D7` | trace |
| 1255 | whipB | flamme `C800` 26 vers 00, coeur `C500` 00 vers 83 : **aucune ecriture de `C096`** | `99F8`, `89F0` | trace |
| 1284 | pick | coeur `C500` 84 vers 00 (ramassage, marche a droite) ; **`C096 = 0F`** dans la meme image | `8BEA` (PC rapporte, voir section 6), `517D` | trace |
| 1310 | pick | `C096 = 00` | `89D7` | trace |

Verification de l'aiguillage (cas du fouet a vide) : le fouet `05` seul ; le fouet sur torche ajoute `0C`. Les deux runs `run2` et `run4` donnent la meme sequence (RAM 4000-7FFF et 8000-BFFF capturee a chaque `C096`).

## 3. Mecanisme

### 3.1 Routine de requete d'effet (ROM, CPU)

| Element | Adresse | Lecture (octets ROM) |
|---|---|---|
| Entree : `PUSH HL/DE/BC/AF`, `DI`, bascule des banques 14 (`8000`) et 15 (`A000`) | `509F` | prouve (dump) |
| `A = 0` : arret (`JP Z,51A4`) ; `A >= FB` : cas speciaux (`51AA`) | `50B9`-`50C2` | lu |
| Refus si `C0A8 != 0` (occupe) | `516A`-`516F` | lu |
| Compare la requete `C` a `C096` : `CP C` ; si `C096 >= C` et different, abandon | `5172`-`5179` | lu |
| Accepte : `LD (C096),A` (la requete devient l'effet courant), puis copie de 14 octets | `517C`-`517D` | prouve (trace PC `517D`) |
| Effet : pointeur de voie 4 `C016 := 509E`/`519E` (`LD (C016),HL`) | `519E` | prouve (trace `C016 = 9C`) |

Regle de priorite (lue dans le code, coherente avec la trace) : une requete n'est acceptee que si son numero est superieur ou egal a `C096` courant. `05` puis `0C` : accepte (`0C > 05`). `0C` puis `0F` : accepte. Une requete plus basse pendant un effet est ignoree.

Table des effets : `CPU 8DC9 + 6 x numero` (bank 14, 3 pointeurs de 16 bits, une par voie ; calcul `50E7`-`50FE`).

**Correction (2026-10-28, `research/audio-driver.md`)** : `8DC9 + 6 x (A & 7F)` est la table des **musiques** (A >= 80). Un effet (A < 80) prend un seul pointeur dans `8D8D + 2 x A` (`518B`-`5197`) et joue sur la voie C. Le flux de `0C` est `8FB9`, celui de `05` est `8EAE`, celui de `0F` est `9065`. Les lignes ci-dessous lisent donc les musiques 85, 8C et 8F, pas les effets. Le son du coup est maintenant le vrai `0C` de la ROM.

| Effet | Entree (CPU) | Pointeurs (voies 1, 2, 3) | Statut |
|---|---|---|---|
| `05` (fouet) | `8DE7` | `9EC1`, `9F89`, `A051` | prouve (lecture) |
| `0C` (coup sur objet) | `8E11` | `A5DF`, `A5FD`, `A621` | prouve (lecture) |
| `0F` (inconnu, ramassage probable) | `8E23` | `A822`, `A822`, `A822` | prouve (lecture) |

Les donnees des effets `05` et `0C` sont differentes (octets distincts a partir de la premiere ecriture, par exemple `E7 E9 D6 FB` pour `05` voie 1 contre `E6 E9 D4 FC` pour `0C` voie 1). Le son du coup n'est donc pas le son du fouet.

### 3.2 Routine appelante de l'effet `0C` (torche / bougie)

| Element | Adresse | Statut |
|---|---|---|
| Garde : `C422 == 3` (phase de coup valide), sinon retour | `7FB7`-`7FBE` | prouve (code) ; `C422` passe a 3 en image 1209 |
| Boucle sur 8 entrees de `C470` (pas de `10`), ignore type `00` et `0C` | `7FC2`-`7FD0` | prouve (code) |
| Test de recouvrement `CALL 8450h` ; si touche : `INC (HL)` sur le champ `+3` de l'entree | `7FC8`-`7FD5` | prouve (code) |
| `LD A,0Ch` ; `CALL 509Fh` | `7FD6`-`7FD8` | prouve (octets ROM et adresse de retour `7FDB` lue en pile a l'ecriture de `C096`) |

Autres appelants de `0C` (lus statiquement, **non verifies en jeu**) : `7F65` (boucle `D700`, 8 entrees stride `80`, enemies probablement), `7F99` (listes `C450`/`C460`), `7DD4` (objets `C800` de type `>= 11`), `80CF`/`8116`/`815E` (sauts). Le son `0C` peut donc aussi etre joue sur ennemi ; ce cas n'a pas ete mesure.

Au point d'ecriture `517D`, l'adresse de retour lue en pile est :

| Requete | Adresse de retour (SP+6) | Appelant |
|---|---|---|
| `05` (fouet, image 561, 1079, 1207) | `7228` | appelant du fouet (`CALL 509F` en `7225`) |
| `0C` (image 1210) | `7FDB` | `7FD8` (`sub_7FB7`) |
| `0F` (image 1284) | `8C01` | **non fiable** : `8C01` est au milieu d'une instruction, donc la pile n'est pas celle d'un appel simple. Appelant inconnu. |

### 3.3 Correspondance avec castlevania-md (lecture seule)

- `src/main.c`, `whip_strikes()` (ligne 1308-1315) : `if (hit) sfx_play(SFX_14);  // a hit ($E4CC)`. Autres appels a `SFX_14` : lignes 1366, 2753, 3634, 3715 (coups sur ennemis et boss).
- `inc/music.h` : `SFX_14 = 34` ; `src/music.c` : entree `sfx_14[62]` (commentaire « NES sound $14 »). Le fouet est `SFX_0A` (`SFX_0A = 24`, deja documente dans `research/audio/whip-sfx-castlevania.md`).
- Correspondance **deduite** : l'effet MSX `0C` joue au coup (fonction identique a `SFX_14`). Les numeros ne sont pas les memes que ceux du NES (fouet MSX `05`, NES `0A` ; coup MSX `0C`, NES `14`), et aucun decalage constant n'est etabli. La correspondance des donnees (PSG contre APU) n'a pas ete comparee.

## 4. Destruction et ramassage

| Evenement | Effet joue | Statut |
|---|---|---|
| Contact du fouet et creation de la flamme | `0C` (image 1210) | prouve |
| Fin de la flamme (`C800` 26 vers 00) et apparition du coeur | aucun (aucune ecriture de `C096` entre 1222 et 1284) | prouve pour ce cas |
| Ramassage du coeur (`C500` 84 vers 00, image 1284) | `0F` dans la meme image | deduit (coincidence) ; appelant `0F` inconnu |
| Destruction de la bougie elle-meme | Pas de son distinct vu : la torche est detruite au contact (flamme), le son est `0C` | deduit |

## 5. Inconnues

1. Appelant de l'effet `0F` (le coup de pile lu n'est pas fiable). Pas de `CALL 509Fh` avec `LD A,0Fh` dans la banque 14 ; la seule occurrence `3E 0F CD 9F 50` est a l offset fichier `0x5344` (banque 2 ; CPU `9346` si la banque 2 etait mappee en `8000`, ce qui n est pas le cas a l image 1284). A verifier : requete par pointeur, `JP`, ou autre chemin.
2. Que `0F` soit bien le son de ramassage : une seule occurrence, coincidence d'image. Un second ramassage (coeur ou autre objet) est a tester.
3. Le son `0C` sur ennemi (`7F65`, `7F99`) et sur les objets `C800` : statique seulement.
4. Torche A (x=36) : le fouet n'a pas ete declenche dans ce run (`whipA` sans `C42F = 06`, `C422` bloque a 02). Pas de reference « fouet sur torche A sans drop » dans ce run. Le test de `research/candles-and-drops.md` (x=36) reste a verifier avec cette methode.
5. Correspondance exacte `0C` avec `SFX_14` (donnees et index) : non etablie.
6. PC du `C500` au ramassage (`8BEA`, rapporte par le watchpoint) ne correspond pas a une instruction d'ecriture de `C500` lisible : les ecritures faites via `IX` peuvent etre rapportees a une instruction voisine. Non tranche.
7. Mapping memoire : `4000-7FFF` = ROM `0000-3FFF` (banques 0 et 1) et `8000-BFFF` = ROM `1C000-1FFFF` (banques 14 et 15) a l'image 1210 et 1284, verifie par comparaison de 16 Ko. La banque `6000` est basculee brievement (`5368` ecrit `0B`, `533D` remet `01`) : les routines en `6000-7FFF` ne sont lisibles qu'entre ces ecritures.

## 6. Methode et fichiers

- Trace : `tools/hit_sound_trace.tcl` (watchpoints `C096`, `C016`, `C0A7`, `C422`, `C42F`, `C800`, `C500`, mapper `4000-7FFF` ; adresse de retour a chaque `C096` ; dump 16 Ko de `4000` et `8000` a chaque `C096`). Sorties dans `scratchpad/hit-sound/run1` (`events.log`, `state.csv`), `run2` et `run3` (dumps), `run4` (retours).
- Lancement : `OPENMSX_USER_DATA=... SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy VK_OUT=<dossier> openmsx -machine C-BIOS_MSX2 -cart game.rom -script tools/hit_sound_trace.tcl -control stdio`. Duree d'un run : environ 2 minutes.
- Le PC rapporte par openMSX (`CPU regs` 20/21) est exact pour les instructions `LD (nn),A` et `LD (nn),HL` testees (`517D`, `519E`, `89D7`), pas pour les ecritures par `IX` (voir inconnue 6).
- Donnees non commitees : `scratchpad/hit-sound/`.
- Scripts de l'enquete precedente : `tools/candle_drop_trace.tcl`, `tools/candle_drop_repeat.tcl`, `tools/trace_psg_openmsx.tcl`.
- Notes liees : `research/candles-and-drops.md`, `research/sound-triggers.md`, `research/audio/whip-sfx-castlevania.md`, `research/castle-candles.md`.

## 7. Donnees PSG Mega Drive pour le coup (0C) : repli SFX_14

Statut : **genere, non ecoute**. Decodage de la table MSX : **impossible a ce stade**.

- Table d'effets MSX relue (banques 8 KiB : banque 14 a `8000` = fichier `1C000`, banque 15 a `A000` = fichier `1E000`). Les pointeurs `8E11` (0C) et `8E23` (0F) sont confirmes. Les flux `A5DF`/`A5FD`/`A621` (0C) et `A822` (0F, les trois voies pointent sur le meme flux) sont lisibles, mais leur format (commandes `E0`-`FF`, notes) est celui du pilote MSX, dont l'interpreteur n'est pas identifie dans la note : le decodage n'a pas ete tente. Sans emulateur ni trace du pilote, il n'y a pas de rendu de reference.
- Repli retenu pour 0C : `SFX_14` de castlevania-md (« a hit », `main.c`). C'est un son de bruit seul (`$400C`, `$400E`, `$400F`), donc le modele de `tools/build_sfx_md.py` s'applique sans changement. Difference de format : le NES ecrit des registres APU (paires registre/valeur, `FE n` = attente, `FF` = fin d'image) ; le MSX ecrit un flux de commandes du pilote MSX. Le son n'est donc pas identique au coup MSX, seulement de meme fonction.
- Generateur : `tools/build_sfx_hit_md.py` (reutilise les fonctions de `tools/build_sfx_md.py`, qui n'est pas modifie). Sortie : `sgdk/src/generated/sfx_hit_md.c` et `.h`, symbole `vk_sfx_hit`, 73 octets, 22 images a 60 Hz. Meme structure que `vk_sfx_whip` (compte par image, puis octets PSG, `FF` de fin).
- Verification : `cc -std=c99 -Wall -Wextra -Werror` passe avec un `genesis.h` minimal pour `sfx_hit_md.c` et `sfx_md.c`. Avec les vrais en-tetes SGDK, le compilateur hote echoue sur les conflits de builtins de `memset`/`strlen` (`SGDK/inc/memory.h`, `string.h`), y compris pour `sfx_md.c` deja present : probleme de l'environnement, non du fichier genere. `sfx_md.c` regenere est identique au fichier committe.
- Effet `0F` (ramassage) : **non genere**. Le flux MSX n'est pas decode et le son NES equivalent n'est pas identifie (l'appelant de `0F` est inconnu, voir section 5). Le son de tresor `SFX_17` (`main.c`, ligne 1939) est un candidat faible, non verifie.
- Branchement : non fait. Le parent doit appeler `vk_psg_play(vk_sfx_hit)` au coup sur torche ou bougie, en incluant `generated/sfx_hit_md.h`.
