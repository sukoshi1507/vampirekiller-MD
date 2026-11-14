# Pilote son MSX2 de Vampire Killer : désassemblage, extraction et portage Mega Drive

Statut global : **pilote prouvé** (lecture du code et émulation conforme à la trace openMSX), **données extraites pour tous les sons** (29 effets 01 à 1D, 15 musiques, 2 sons spéciaux), **chaque identifiant relié à son événement et à son appelant** (section 3, appelants contrôlés sur les octets de la ROM par `tests/test_extract_msx_sound.py`), **tous les sons branchés dans le port** (section 7). Aucun nom de son n'est provisoire. Reste non prouvé : la durée exacte de la mort par contact (79 images, section 7). Le garde `A4E8` (`C420 = 6`) est prouvé et porté (section 7). Le rendu par défaut est celui du SN76489 ; un mode FM optionnel des musiques (YM2612, choisi sur l'écran titre) est décrit dans `research/audio-fm.md`.

ROM de référence : SHA-256 `99eabaae…e7e2` (`tools/probe.py`). Adresses CPU Z80. Banque 0 en `4000`, banque 1 en `6000`, banque 2 en `8000`, banque 3 en `A000` (mappage par défaut, rétabli par `5139`). Le pilote est en banque 14 (`8000`) et 15 (`A000`), mappées par `509F` et par le VBLANK `4028`.

## 1. Points d'entrée

| Adresse | Rôle | Statut |
|---|---|---|
| `5076` | Init : `C097 = BC` (mélangeur), `C0A5 = C0A6 = C0A7 = 0`, puis `5085` | prouvé (code) |
| `5085` | Arrêt total (requête `00`) : `C096 = C098 = C0A8 = 0`, les 4 pointeurs `C010..C016 = 89CE` | prouvé (code) |
| `509F` | Requête de son, identifiant dans A. Bascule banques 14/15, puis aiguillage | prouvé (code, trace) |
| `4028` → `8964` | Mise à jour par image (VBLANK), banques 14/15 mappées | prouvé (trace : 58,3 appels par seconde) |
| `0093` (BIOS GIWRTE) | Écriture AY (registre A, valeur E) ; `0096` (GIRDPSG) lit un registre | prouvé |

### 1.1 Aiguillage de `509F`

| A | Chemin | Effet |
|---|---|---|
| `00` | `51A4` → `5085` | arrêt total |
| `01`..`7F` | `516A` | **effet** : refusé si `C0A8 ≠ 0` (`516B`) ou si `C096 > A` (`5172`-`5179`). Sinon `C096 = A`, bloc `C058` remis à zéro, pointeur `8D8D + 2A` → `C064`, voie 4 `C016 = 8C9C` |
| `80`..`FA` | `50C6` | **musique** : blocs `C01C`, `C030`, `C044` remis à zéro, 3 pointeurs `8DC9 + 6(A & 7F)` ; `C010/C012/C014 = 8A64/8A6B/8A72` ; `C096 = 0` et `C016 = 509E` (RET : l'effet en cours est abandonné) ; `C0A5 = C0A6 = C0A8 = 0` ; `C0A7 = 07` | prouvé |
| `FB` / `FC` | `5276` / `52C9` | son de minuterie sur la voie A (flux `948B`, routine `8C95`, bloc `C06C`) ; les voies de musique sont gelées (`C098` bit 1). `FC` restaure |
| `FD` / `FE` | `51C4` / `522D` | pause : sauvegarde des registres, mélangeur `BF`, jingle `9463` sur la voie A (bloc `C080`) ; `FE` restaure |
| `FF` | `51BC` | fondu de la musique : `C0A5 = 3A` ; `897A` baisse `C0A6` d'un cran toutes les 3A images, arrêt à `F0` (16 crans) |

### 1.2 Mise à jour `8964`

1. Écrit `C097` dans R7 (mélangeur).
2. Si `C098` bit 0 (pause) : seule la voie `C01A` tourne. Si bit 1 (`FB`) : voie `C018` sur AY A, puis l'effet.
3. Fondu (`C0A5`/`C0A6`).
4. Voies de musique 1, 2, 3 (`C010`, `C012`, `C014`) sur AY A, B, C (`C094` = 0, 1, 2 ; `C095` = numéro de voie), puis la voie d'effet `C016` **toujours sur AY C** (`89BF` : `C094 = 2`, `C095 = 3`).

**Priorité de voie (prouvé, code et trace)** : quand `C096 ≠ 0`, la voie de musique 3 (AY C) reçoit le bit 5 de `IX+2` (`8A85`) : elle continue d'avancer mais n'écrit plus rien. À la fin de l'effet, `89CE` met `C096 = 0`, écrit le volume 0 sur C et gare la voie d'effet sur `89F1` (RET). Le bit 5 n'est effacé qu'à la **prochaine note** de la voie 3 (`8AF1`-`8B00`) : la musique reprend la voie C à sa note suivante, pas au milieu d'une note. Vu dans la trace : la voie C diffère après chaque fouet jusqu'à la note suivante.

## 2. Format des données

### 2.1 Voie de musique (`8A76`, bloc de 20 octets `IX`)

`IX+0/1` pointeur, `+2` drapeaux (bit 0 mode direct, bit 2 table d'instruments `AAF1`, bit 5 muet, bit 6 désaccord +2, bit 7 instrument fini), `+3` tempo, `+4` volume, `+5/+6` décroissance, `+7` octave, `+9` durée restante, `+A` volume courant, `+B` seuil, `+C/+D` pointeur d'instrument, `+E/+F` vitesse d'instrument, `+10/+11` compteurs de boucle, `+12/+13` retour de sous-programme.

| Octet | Sens |
|---|---|
| `00`-`BF` | note : quartet haut = note (table de périodes `8B81`, décalée de `+7` octaves), quartet bas + 1 = durée × tempo. En mode instrument (bit 0 nul), la note choisit un instrument (`AAD9[note]`) : batterie et bruit |
| `C0`-`CF` | silence, durée = (n + 1) × tempo |
| `Dn` | tempo = n |
| `En` | `E0`-`E5` octave (6 - n) ; `E6` verrou des effets `C0A8 = 1` ; `E7` désaccord ; `EA lo hi` saut ; `EB` fin du verrou ; `ED lo hi` sous-programme ; `EE` retour ; autres : drapeaux `IX+2` bits 0-2 |
| `Fn` | `F0`-`FD` volume n+1 et octet de décroissance ; `FE n lo hi` boucle n fois ; `FF` fin de voie (efface son bit dans `C0A7`, `8C80`) |

### 2.2 Flux d'effet et d'instrument (`8CA7`)

Un pas toutes les `IX+F` images : `FF` fin ; `FE n lo hi` boucle ; `1n` période de bruit R6 = 2n, puis octet suivant ; `2x v` mélangeur (bits 0-1 : ton et bruit), bit 3 = volume par enveloppe, `v` = vitesse, et si `x ≥ 8` deux octets R12, R11 ; sinon octet `VP` + `pp` : volume V (ou forme d'enveloppe R13 si le mode enveloppe est actif), période de ton `Ppp`.

## 3. Table des identifiants

Pointeurs relus dans la ROM : `research/audio/msx_sounds.json` (`effect_pointers`, `music_pointers`). Appelants : tous les `LD A,id` suivis de `CALL/JP 509F` ou de `CALL A4E8` (enveloppe de la banque 3 : `509F` sauf si `C420 = 6`) dans les banques 0 à 3 (les banques 4 à 15 n'en ont aucun), plus les gestionnaires de ramassage `8D30` (la routine empile `509F` en `8D2F`, le gestionnaire revient avec l'identifiant dans A). **La liste complète des appelants est contrôlée sur les octets de la ROM** : `tools/build_msx_sound_md.py` (table `SOUNDS`, source des noms du menu de test) et `tests/test_extract_msx_sound.py` (`SoundEventTests` balaie la ROM et échoue si un appelant manque ou n'existe pas). Les identifiants 0 (arrêt), FC, FE et FF ne sont pas des sons : arrêt `43BE`, `6613`, `70A5`, `B725` ; FC `75FF` ; FE `43E6` ; fondu FF `6786`, `7763`. L'identifiant 1D est atteint par le dernier pointeur de la table (`8DC7`, flux `8E69`).

Colonne « Preuve » : **trace** = observé sur openMSX (document cité) ; **code** = condition lue dans le désassemblage Z80 de l'appelant (l'identifiant et l'adresse sont contrôlés par le test ci-dessus, le contexte par la lecture du code, qui est déterministe : pas de trace de la requête elle-même) ; **Z80** = rejoué sur la ROM par un test du dépôt.

### 3.1 Effets (voie C)

Adresses = instruction `CALL`/`JP` (appelants `call`, `wrap`) ou `LD A,id` des gestionnaires de ramassage (`ret`). Port : tous les effets sont joués, aux événements de la colonne ; corrections de cette passe signalées par « corrigé ».

| Id | Flux | Nom (menu) | Événement | Appelant(s) | Preuve | Port |
|---|---|---|---|---|---|---|
| 01 | `8E2B` | SOIN ENERGIE | stage fini après un boss : l'énergie `C415` remonte pas à pas jusqu'à 20h | `B744` (routine `B736`, script `B6AB`) | trace (`stage6-boss.md` : 12 pas de 20 à 32) | oui (`actors.c`, `VK_BOSS_EV_HEAL`) |
| 02 | `8E58` | REFUS MARCHAND | le marchand : réponse non, cœurs insuffisants ou délai écoulé | `94FE` (`94FC`) | code | oui (`wall.c`) |
| 03 | `8E8A` | CROIX TOURNE | la croix lancée (type 4, `72DE`) tourne, tous les 8 passages (`C003 & 7`) | `72F5` | code (+ trajectoire tracée, `secondary-weapons.md`) | oui (`weapons.c`) |
| 04 | `8E99` | DAGUE LANCEE | la dague (type 2, `7436`) quitte Simon | `7470` | code (+ lancer tracé, `secondary-weapons.md`) | oui |
| 05 | `8EAE` | COUP DE FOUET | début du coup de fouet (`C42F = 6`), fouets seulement (`C436 < 2`) | `7225` | trace (13/13 images identiques) | oui (`main.c`) |
| 06 | `8ECB` | HACHE TOURNE | la hache lancée (type 3, `7473`) tourne, tous les 8 passages | `7489` | code (+ trajectoire tracée) | oui |
| 07 | `8EE4` | ATTERRISSAGE | Simon retombe (saut plus bas que son départ `6D38` ; chute `6F7E`) ; un homme-poisson se pose (`A310`) | `6D38`, `6F7E`, `A34F` (`A34D`) | trace (`6D38`, `scratchpad/fidelity/out1`) ; code (`6F7E`, `A34D`) | oui ; **corrigé** : homme-poisson du stage 2 (`fishStep`) |
| 08 | `8EF3` | POISSON SAUTE | un homme-poisson (objets 02/03, init `A2E0`) jaillit de l'eau à sa création ; pas pour les objets 21h (init `A2C7`) | `A2FB` (`A2F9`) | code (+ Z80 : `tests/test_fish3.c`) | oui ; **corrigé** : stage 2 |
| 09 | `8F25` | PLONGEON EAU | Simon tombe dans le vide des stages 2 et 10 (`76BC`, `D000 = 2` ou `10`) ; un homme-poisson retombe dans l'eau (`A386`) | `76E5`, `A388` (`A386`) | code | oui ; **corrigé** : stage 2 |
| 0A | `8F56` | TIR BOSS 9 | le boss du stage 9 (objet 14h) lance sa boule (`B985` -> `9F6D`) | `B9AB` (`B9A9`) | code (+ `stage9-boss.md` : « effet 0A (`A4E8`) ») | oui ; **corrigé** : les autres boss tiraient un 13 parasite |
| 0B | `8F75` | PROJECTILE PARE | le bouclier (objet 04, `C701` bit 5) arrête un projectile ennemi (`D700`, `8610`) | `8631` | code | oui (`enemies.c`, `actors.c`) |
| 0C | `8FB9` | COUP TOUCHE | le fouet ou une arme lancée touche une bougie, un ennemi, un boss ou un mur | `7DD6`, `7F67`, `7F9B`, `7FD8`, `804E`, `80D1`, `8118`, `8160` | trace (`hit-sound.md`) | oui |
| 0D | `8FDD` | TUE AU CONTACT | Simon touche un ennemi pendant le minuteur `C434` (objet 09) : l'ennemi est détruit, 81AB et 9A3E | `7EBE` (routine `7E67`) | code (+ `castle-candles.md` 9.5) | oui (`enemies.c`, `contactSound`) |
| 0E | `902E` | BLOC CASSE | la bougie touchée est un bloc (mode 2 ou 3) | `87D8` | code | oui (`main.c`) |
| 0F | `9065` | COEUR CLE ROUGE | petit ou gros cœur ramassé (objets 01, 02) ; clé rouge (objet 12 : `C700 = 3`) ; le mur donne 5 cœurs | `8DB4` (`8DAF`, `8DB7`), `8E90` (`8E79`), `9346` | trace (cœur, `hit-sound.md`) ; code (clé, mur) | oui |
| 10 | `909A` | BONUS POINTS | objet 13h ou 14h ramassé (5000, 1000 points) ; le vieil homme du mur disparaît (5000 points) | `8E48`, `936F` | code | oui |
| 11 | `90C7` | COFFRE OUVERT | un coffre s'ouvre avec une clé dorée | `8F7D` (`8F7B`) | code | oui |
| 12 | `90E6` | OBJET PRIS | tout autre objet ramassé (fouet, armes, bits d'inventaire, énergie, minuteurs) ; objet acheté au marchand | `8D8A`, `8E30`, `94F1` (`94EF`) | code | oui |
| 13 | `9137` | SIMON BLESSE | début du recul après un contact (`C002` bit 6) | `6F9A` | code | oui |
| 14 | `9152` | CLE PRISE | clé dorée (objet 17) ou clé blanche (objet 18) ramassée | `8E5D`, `8E68` | code ; clé blanche : trace (`door-room7.md`) | oui |
| 15 | `9179` | PORTE PASSAGE | porte du château de la cour (`C5AC = 5`) ; entrée d'un passage secret (accroupi + HAUT devant `C5B1`) | `7774` (`7772`), `6DC5` (`6DC3`) | trace (`castle-entrance.md`) ; code (passage, `secret-passages.md`) | oui (`main.c`, `VK_SECRET_EFFECT`) |
| 16 | `9212` | INVULNERABLE | objet 08 ramassé : `C43A` = 96h (F0h), aucun contact | `8DDB` | code | oui |
| 17 | `9275` | FIN INVULNERABLE | `C43A` retombe à 10h : l'invulnérabilité va finir | `75EF` (`75ED`) | code | oui (`enemies.c`, `guardWarning`) |
| 18 | `92D8` | EAU BENITE SOL | l'eau bénite lancée (type 5, `73A4`) touche le sol, les flammes commencent | `7416` (`7414`) | code (+ tracé : `secondary-weapons.md`) | oui |
| 19 | `9470` | MARCHAND CARTE | la fenêtre du marchand s'ouvre (`938F`) ; la carte du stage s'ouvre sur F2 (`959E`) | `9391`, `95A0` | code | oui (`wall.c`, `main.c`) |
| 1A | `931A` | PORTE STAGE | une porte intérieure franchie (`C5AC` 1 → 2) ; précédé de `FF` si `7777[D000] ≠ 0` (stages 3, 6, 9, 10, 12, 15, 17, 18) | `776F` (`776D`) | Z80 (`tools/verify_rom_markers.py`, 18 portes) | oui |
| 1B | `93CA` | ENNEMIS DETRUITS | objet 05 ramassé : tous les ennemis et projectiles détruits (`7806`), éclair | `8DCA` | code | oui |
| 1C | `939F` | MORT DU BOSS | la jauge ENEMY `C418` tombe à 0 (`7E17`) | `7E1E` (`7E1C`) | trace (stages 6, 9 : `stage9-boss.md`, `stage6-boss.md` « effets 0C et 1C ») | oui (`actors.c`, `dracula.c`) |
| 1D | `8E69` | COEURS PERDUS | résultat du mur : cœurs - 5 (`934E`, `45A0`) | `9350` (`934E`) | code | oui ; **corrigé** : flux extrait (l'identifiant était ignoré : hors table) |

Le flux de 1D commence au milieu des données de 02 (`8E58`-`8E89`), à `8E69` : le pilote le joue tel quel (46 images, montée de `R10`/`R4`).

### 3.2 Musiques (voies A, B, C) et sons spéciaux

| Id | Nom (menu) | Événement | Appelant(s) | Preuve | Port |
|---|---|---|---|---|---|
| 80 | STAGES 0-3 | musique de stage `7972[D000]`, `D000` = 0 à 3 (cour, hall, stages 2 et 3) | `796F` | trace (voies A/B identiques sur 7 649 images, boucle comprise) | oui |
| 81 | STAGES 4-6 11 12 | `D000` = 4, 5, 6, 11, 12 | `796F` | code (table `7972` relue par le test) | oui |
| 82 | STAGES 7-9 | `D000` = 7, 8, 9 | `796F` | code | oui |
| 83 | STAGES 16-17 | `D000` = 16, 17 | `796F` | code | oui |
| 84 | STAGES 13-15 | `D000` = 13, 14, 15 | `796F` | code | oui |
| 85 | STAGES 10 18 | `D000` = 10, 18 | `796F` | code | oui |
| 86 | ENTREE DRACULA | entrée de la salle de Dracula (`D000 = 12h`, `77D1`) | `77FD` | code (+ `stage18-boss.md`) | oui |
| 87 | ENTREE BOSS | entrée de la salle d'un autre boss (`77D1`) | `77F7` (saut `JR` en `77F9`) | trace (`stage12-boss.md`, `stage3-boss.md`) | oui |
| 88 | DRACULA FORME 2 | la première forme de Dracula a disparu, la seconde commence (`CE01 = 3`, script `65B0`) | `661B` (`6619`) | code (Z80 : `tests/run_dracula_trace.sh`, requêtes `509F` comparées) | oui (`dracula.c`) |
| 89 | MORT SIMON | mort de Simon, après l'arrêt 00 (`70A5`) | `70AC` | code | oui |
| 8A | DEPART | début de partie (état 3, sous-état 0) | `4224` (`4222`) | trace (385/385 images) | oui |
| 8B | GAME OVER | plus de vie (état 6) | `42AD` (`42AB`) | trace (`player-health.md`) | oui |
| 8C | VICTOIRE BOSS | l'orbe du boss est prise : stage terminé (`B720` à l'entrée de `B6AB`, `B72A`) | `B722`, `B72C` | trace (`stage6-boss.md`, `stage9-boss.md`) | oui |
| 8D | DRACULA VAINCU | la seconde forme de Dracula est tuée (`CE01 = 6`, `667F`) | `6681` (`667F`) | code (Z80 : `run_dracula_trace.sh`) | oui |
| 8E | GENERIQUE | début du générique de fin (`CE40`, `66C9`) | `66CB` (`66C9`) | code (Z80 : `run_dracula_trace.sh`) | oui |
| 8F | - | pointeurs vers un flux vide, jamais demandé | - | - | - |
| FB | ARRET DU TEMPS | l'objet 0A utilisé (BAS en saut, cœurs - 5) : `C43B` = 5Ah (96h) ; voies de musique gelées | `7191` (`718F`) | code ; `castle-candles.md` 9.9 | oui (`vk_psg_timer`) |
| FD | PAUSE | touche de pause en jeu (`5C3A`) | `5C59` (`5C57`) | code (jingle `9463` extrait ; FD/FE rejoués par le scénario `o_pause`) | oui |

Règle de reprise (prouvé, code `794F`) : en état 4, la musique du stage n'est relancée qu'au premier stage (`D000 = 0`), après une vie perdue (`C40D = 1`) ou après un stage de boss (`7777[D000-1] ≠ 0`). Cour → hall → stage 2 : la musique 80 continue sans redémarrer. `7763` : fondu `FF` avant la porte d'un stage de boss (`7777[D000] ≠ 0`). L'écran titre (états 0 à 2) est muet : aucune requête, aucune écriture de son dans la trace `long1` avant l'image 598.

Musiques qui verrouillent les effets (`C0A8`, commande `E6`) : 89, 8B, 8C, 8D, 8E, du début à la fin. Le verrou reste posé après la fin, jusqu'à la requête suivante de musique ou d'arrêt (`5131`, `508C`).

## 4. Extraction par émulation

`tools/extract_msx_sound.py` : cœur Z80 (`z80`, comme `castle_room_objects.py`), mappeur Konami, pièges BIOS sur `0093` (journal des écritures AY) et `0096`. Pour chaque son : `5076`, une image à vide, requête `509F`, puis `8964` une fois par image. Fin d'un effet : `C096 = 0`. Fin d'une musique : `C0A7 = 0`. Boucle : l'état `C010..C0BF` détermine tout le futur ; le premier état répété donne la boucle exacte (frames `0..i` gardées, reprise en `seen + 1`). Les marques `note_c` (`8AF1` avec `C095 = 2`) et `lock` (`C0A8`) sont relevées par image.

Vérification contre la trace openMSX `research/audio/long1-bios-giwrte.csv.gz` (registres 14-15, manette, exclus) — tests `tests/test_extract_msx_sound.py` :

| Son | Fenêtre de trace | Résultat |
|---|---|---|
| 8A | images 598 à 982 | 385/385 images identiques, tous registres |
| 80 | images 1096 à 8744 | voies A et B identiques sur 7 649 images (3 passages de la boucle 596 → 2387) ; voie C : différences seulement aux fouets (toutes les 2 s puis 5 s, comme les entrées), expliquées par la priorité |
| 05 | images 1197 à 1209 | 13/13 images identiques pour R4, R5, R6, R10 |

## 5. Conversion AY-3-8910 → SN76489

`tools/convert_psg_vgm.py` (`FrameConverter`) :

- Ton : `N = P` (AY 1 789 772,5 Hz / 16, SN 3 579 545 Hz / 32 : même fréquence). `P > 1023` : remonté d'une octave jusqu'à tenir (la note garde son nom) ; compté `tone_folded` (basses des musiques 81 à 88, 8E). **Approximation**.
- Volume : 3 dB par pas AY (table AY8910 d'openMSX), 2 dB par pas SN : `att = arrondi(1,5 × (15 - v))`, 15 = muet.
- Bruit : la voie AY la plus forte avec le bruit actif pilote le bruit SN (blanc). Si le ton 2 SN est libre (voie C sans ton audible), le bruit est cadencé par le ton 2 avec `N2 = R6` : **fréquence exacte** du bruit AY. Sinon taux fixe le plus proche (`noise_fixed_rate`, rare).
- Enveloppe (effets 02, 0B, 1A seulement) : enveloppe AY simulée, amplitude moyenne sur l'image. Enveloppe continue plus rapide qu'une image sur une voie sans ton audible (ton coupé ou période < 16) : le ton SN prend la fréquence de l'enveloppe. **Prouvé en fréquence** : l'enveloppe AY avance d'un pas toutes les 16 x EP horloges (EP = R11 + 256 R12 ; datasheet AY-3-8910 : f = horloge / (256 x EP) pour un cycle de 16 pas ; le test simule le même modèle à horloge / 8, un pas toutes les 2 x EP échantillons), 16 pas par cycle pour une dent de scie (formes 8 et 12), 32 pour un triangle (10 et 14), donc f = 1 789 773 / (256 x EP) (dent de scie) ; le ton SN de période N vaut 3 579 545 / (32 x N), d'où N = arrondi(3 579 545 x cycle / 32). L'écart se limite à l'arrondi de N. **Mesuré dans openMSX** (`tools/measure_ay_envelope.py` : registres écrits sur la voie C avec un ton de période 1, audio enregistré, pic de FFT) : dent de scie EP = 1 : 6 991,3 Hz (SN : N = 16, 6 991,3 Hz) ; triangle EP = 1 : 3 495,7 Hz (N = 32, 3 495,6 Hz) ; dent de scie EP = 5 : 1 398,2 Hz (N = 80, 1 398,3 Hz) ; EP = 39 : 179,1 Hz (N = 624, 179,3 Hz) ; écart SN / openMSX inférieur à 0,08 %. Tests : `tests/test_convert_psg_vgm.py` (`test_envelope_buzz_pitch_is_exact` simule l'enveloppe pour les quatre formes et six valeurs d'EP et exige l'écart de 0,5 / N au plus ; `EnvelopeInOpenMsxTests` rejoue la mesure openMSX si `openmsx` est installé). **Sons concernés dans toute la ROM : l'effet 0B seul, images 25 et 26** (EP = 1, forme 12, ton de période 1, soit 112 kHz : inaudible ; `test_fast_envelope_buzz_is_only_in_sound_0b` balaie les 46 flux). Les formes 02 (EP = 19, triangle à 184 Hz, ton de période 112), 1A (EP = 39, dent de scie à 179 Hz, ton de période 16) et l'image 0 de 0B (EP = 5, dent de scie à 1 398 Hz, ton de période 496) ont un ton audible : le ton SN garde la période du ton, l'enveloppe ne donne que l'amplitude moyenne. Ce qui reste approximatif est le timbre (un carré au lieu d'une dent de scie, modulation d'amplitude de 02 et 1A perdue), pas la hauteur.
- Effets : voie C et bruit seulement. FB et FD : voie A. Musiques : 3 voies et bruit.

`tools/build_msx_sound_md.py` produit `sgdk/src/generated/msx_sound_md.c/.h` (71 428 octets de flux, reproductible : même SHA-256 à chaque génération) et `research/audio/msx_sounds.json`. Format par image : en-tête (bits 0-5 nombre d'octets, bit 6 note sur la voie C, bit 7 verrou), octets PSG ; `FF` termine, reprise à l'octet `loop` ou arrêt.

## 6. Lecteur Mega Drive

`sgdk/src/psg_core.c` (sans SGDK, testé par `tests/test_psg_core.c`) et `psg_player.c` (écriture au port PSG en interruption verticale, cadence PAL 6 images pour 5 VBLANK) :

- deux voix : musique et effet ; l'effet prend le ton 2 et le bruit, la musique garde les tons 0 et 1 ;
- la musique continue d'avancer sur les voies prises (état mémorisé) ; après l'effet, le ton 2 et le bruit restent muets jusqu'à la première image marquée « note sur C », puis l'état mémorisé est réécrit (`8B00`) ;
- priorité `5172` (effet accepté si aucun effet ou id ≥ courant), verrou `C0A8`, musique qui abandonne l'effet (`5118`), arrêt `5085`, fondu `FF` (1,5 cran SN toutes les 3A images, arrêt après 16).

Cadence et interruption : comme le pilote MSX, appelé par le VBLANK `4028` (H.TIMI) et non par la boucle de jeu, le lecteur avance dans l'interruption verticale (`SYS_setVIntCallback`, installé par `vk_psg_init` au démarrage dans `main.c`), une fois par VBLANK, avec la cadence PAL ci-dessus. Le callback VBlank de SGDK (`SYS_setVBlankCallback`) ne convient pas : il est appelé depuis `SYS_doVBlankProcess`, donc par la boucle principale, et s'arrêterait comme elle pendant un long traitement. La musique continue ainsi pendant le dessin d'une salle (environ 6 images de 68000 sans tour de boucle) : **prouvé** sur le harnais (`n_room_transition_freeze`, `vk_psg.music.cursor` change à chaque image des 8 dernières du gel du hall ; avec l'ancien appel dans la boucle, il restait figé 5 images). Points de détail :

- les requêtes (`vk_psg_music`, `effect`, `stop`, `fade`, `pause`, `resume`, `play_stream`) viennent de la boucle principale et modifient `vk_psg` entre `SYS_disableInts` et `SYS_enableInts` ; `vk_psg_stop` écrit aussi le silence au port dans la même section, pour qu'aucune écriture de l'interruption ne s'intercale dans une paire verrou/donnée SN ;
- la fréquence PAL/NTSC est lue une fois à l'initialisation : l'interruption ne lit pas le port d'état du VDP, ce qui pourrait gêner une commande VDP en cours dans la boucle ;
- le port PSG (`C00011`) est sur le bus 68000, hors du bus Z80 : aucune demande de bus Z80, et le pilote PCM (`SND_PCM_loadDriver`, cri de Wilhelm sur le DAC du YM2612) n'écrit pas au PSG (vérifié dans `drv_pcm.s80` de SGDK) ; les deux ne se gênent pas ;
- le temps du lecteur est pris au début de chaque image : un long dessin commencé dans le même tour de boucle finit un peu plus tard (scénario `h_castle_entrance` : le mode INTERIOR, posé à la fin du dessin du hall, arrive une image après la fin logique des 36 images noires).
- **coût mesuré** (2026-10-30, ROM de mesure `EXTRA_FLAGS=-DVK_MEASURE_PSG`, script `tools/measure_psg_cost.txt` sur le harnais Genesis Plus GX, NTSC, H32) : `vk_psg_measure` refait le pas suivant (`vk_psg_core_frame` et les écritures au port, vers une variable) sur une copie de `vk_psg`, depuis la boucle principale pendant l'affichage, chronométré par le compteur HV (`C00008`, 171 pas par ligne de 488,6 cycles 68000). Titre, intro, cour (musique 80h, fouets, cœur, saut) puis hall, 1566 pas : **moyenne 665 pas, soit 3,9 lignes (≈ 1 900 cycles)** ; 443 pas au-delà de 5 lignes (28 %) ; **maximum 2615 pas, 15,3 lignes (≈ 7 500 cycles)**, sur un pas de 14 octets écrits (au plus 14 octets par pas observés, pour 160 prévus par `VK_PSG_OUT_MAX`). L'entrée d'interruption de SGDK n'est pas comptée. Conséquence : le lecteur tourne au début de la V-Int, avant le vidage de la file DMA (`SYS_doVBlankProcess`, dans la boucle principale) ; le blanc vertical NTSC dure 38 lignes, le lecteur en prend 4 en moyenne et 15 au pire, ce qui laisse 23 lignes au moins à la file DMA (table des sprites, tuiles du décor, couleurs). Pas de débordement observé dans les scénarios ; à surveiller si la file grossit. La mesure du coût ne change pas la ROM normale (code sous `#ifdef VK_MEASURE_PSG`).

Écart connu : sur MSX, le bruit d'une musique porté par les voies A ou B (musiques 8B et 8E) continuerait pendant l'effet ; ici le bruit SN est pris par l'effet.

## 7. Branchement (port)

| Événement MD | Son | Adresse MSX | Fichier |
|---|---|---|---|
| START au titre (château) | musique 8A | `4222` | `main.c` |
| Début de partie (cour) | musique `7972[D000]` = 80 | `794F` | `main.c` (`new_game`) |
| Vie perdue, reprise | musique du stage | `4242`, `C40D = 1` | `main.c` (`respawn`) |
| Retour au titre | arrêt | titre muet | `scene.c` (`vk_show_title`) |
| Fouet | 05 | `7225` | `main.c` |
| Fouet sur bougie / torche | 0C, puis 0E si bloc (mode ≥ 2) | `7FD8`, `87D8` | `main.c` (`step_candles`) |
| Fouet sur ennemi | 0C | `7F65`, `7DD4` | `actors.c` |
| Cœur, fouet amélioré, clé dorée, coffre | 0F, 12, 14, 11 | `8DAF`, `8D8A`, `8E5D`, `8F7B` | `main.c` |
| Clé blanche | 14 | `8E68` | `main.c` |
| Porte intérieure d'un stage | 1A, après `FF` selon `7777[D000]` | `776F`, `7763` | `main.c`, `items.c` (`vk_items_door_fades`) ; exécuté pour les 18 portes |
| Porte du château | 15 | `7774` | `main.c` |
| Contact ennemi (recul) | 13 | `6F9A` | `main.c` |
| Chute dans l'eau (stage 2) | 09 | `76E5` | `main.c` |
| Mort | arrêt puis 89 : chute 30 images (eau) ou 1 image ; contact **79 images** (PROVISOIRE, mesuré `player-health.md` : 40,617 s → 41,94 s) | `70A0`-`70AC` | `main.c` |
| Fin de partie | 8B | `42AB` | `main.c` |
| Atterrissage d'une chute commencée au sol (mode 0 → 4) | 07 | `6F7E` | `main.c` (`VK_EVENT_LAND`) |
| Atterrissage d'un saut plus bas que son départ (`C428 ≥ 21`) | 07 | `6D38` | `gameplay.c` (`jump_step`) → `VK_EVENT_LAND` → `main.c` |
| START en partie (pause) | FD : silence, jingle `9463` seul, musique et effet figés | `51C4` | `main.c` (`pause_start`), `psg_core.c` |
| START pendant la pause | FE : voies de musique restaurées, la musique reprend où elle était | `522D` | `main.c` (`pause_end`) |
| Arrêt du temps (objet 0A, BAS pendant un saut) | FB : voies de musique coupées et gelées, boucle `948B` sur la voie A, effets sur C | `718F` → `5276` | `actors.c` (`vk_step_time_stop`), `psg_core.c` (`vk_psg_core_timer`) |
| Fin de l'arrêt du temps (`C43B` à 0) | FC : voies de musique rendues | `75FF` → `52C9` | `actors.c` (`timeStopEnd`), `psg_core.c` (`vk_psg_core_timer_end`) ; voir `castle-candles.md` 9.9 |
| Marchand : fenêtre, refus, achat, cœurs du mur | 19, 02, 12, 0F, 1D, 10 | `938F`, `94FC`, `94EF`, `9344`, `934E`, `936D` | `wall.c` (`SOUND_*`), `main.c` (`step_shop`, `step_walls`) |
| Carte du stage (F2) | 19 | `959E` | `main.c` |
| Passage secret | 15 | `6DC5` | `secret.c`, `main.c` (`secret_controls`) |
| Armes lancées | 04 dague, 06 hache, 03 croix, 18 eau bénite au sol | `7470`, `7489`, `72F5`, `7416` | `weapons.c` |
| Bouclier (objet 04) | 0B | `862F` | `enemies.c` (`Enemies_shield`), `actors.c` (`vk_step_projectile_shield`) |
| Objets 08, 09, 05 | 16 et 17 (fin du minuteur), 0D, 1B | `8DDB`, `75ED`, `7EBC`, `8DCA` | `candles.c` (`vk_candles_sound`), `enemies.c` (`contactSound`, `guardWarning`) |
| Hommes-poissons | 08, 07, 09 | `A2F9`, `A34D`, `A386` | `enemies.c` (`fishSpawn`, `fishStep`), `enemies_fish3.inc` |
| Boss : entrée, tir du stage 9, mort, orbe | 87 (86 Dracula), 0A, 1C, 00 puis 8C, 01 | `77F7`, `B9A9`, `7E1C`, `B725`/`B72A`, `B744` | `main.c`, `actors.c` |
| Dracula | 88, 8D, 8E, FF (fondu des crédits) | `6619`, `667F`, `66C9` | `dracula.c` |
| Chute dans un trou ou dans l'eau | cri de Wilhelm (PCM, DAC) | **ajout du port**, absent de la ROM | `main.c` (voir §7.1) |

**Atterrissage d'un saut (6D26-6D38), prouvé le 2026-10-29.** `CFF0` n'est pas un reste des sondes de mur : la descente du saut (`C428 ≥ 9`, `6D13`) écrit `CFF0 := 0` à chaque pas (`6D17`), puis `CFF0 := 1` en `6D6F` quand `C428 - 2 ≥ 13h` (`6D1C`-`6D22`, appel de `6D6D`) ; la sonde de sol `7B88` n'écrit pas `CFF0`. Le son 07 du saut joue donc si l'atterrissage tombe au pas 21 ou plus. La table `6D95` a 19 pas et revient à la hauteur de départ au pas 19 ; au-delà, +6 par pas. Trace (`scratchpad/fidelity/out1`, Simon soulevé de 0, 0, 12, 24 et 40 px au départ du saut, point d'arrêt `6BA0`) : atterrissages au pas 19 sans 07 (deux fois), aux pas 21, 23 et 26 avec `CFF0 = 1` et une requête 07 à `509F` à la même image. Sens de jeu : **un saut qui retombe plus bas que son départ fait le bruit d'atterrissage**, pas un saut sur sol plat. Le port compte `C428` après la table (son saut passe en chute, mode 4, au pas 19) : `19 + ceil(images / 2)` (`gameplay.c`, `jump_step`), test `test_gameplay.c` (départs soulevés de 4, 12, 24, 40 px). PROVISOIRE : la table à 21 pas (`6D94`, `C431` bit 4, non portée) atteindrait le pas 21 même sur sol plat.

Audit des déclencheurs (2026-11-14) : chaque identifiant de la section 3 est joué par le port à l'événement de la table ; la liste « non branchés » des versions précédentes est périmée (armes lancées 03, 04, 06, 18, bouclier 0B, objets 08/09/05 : 16, 17, 0D, 1B, marchand 02, 19, 1D, boss et Dracula 86 à 8E, 1C, arrêt du temps FB/FC, passage secret 15 étaient déjà branchés). Corrections de cette passe : (1) **1D** (cœurs - 5 du mur) était demandé par `wall.c` mais l'identifiant dépassait la table : flux extrait, joué ; (2) **homme-poisson du stage 2** (`fishStep`, type 02, même routine `A310` que le type 03 du stage 10) : 08 à la création, 07 à l'atterrissage, 09 au plongeon, qui manquaient ; (3) **tir des boss** (`VK_BOSS_EV_FIRE`) : le port jouait 13 pour tous les boss sauf le stage 9, alors qu'aucune requête n'existe dans `BF44`/`9F61`/`9F6D` (les boss des stages 3, 6, 12, 15 tirent sans son) ; seul le 0A du stage 9 est conservé. Les 29 effets et les musiques sont jouables dans le menu de test (code Konami au titre, libellés = `SOUNDS` de `tools/build_msx_sound_md.py`).

Garde de `A4E8` (prouvé, openMSX 2026-11-14 + code) : l'enveloppe de la banque 3 est `A4E8 LD C,A / A4E9 LD A,(C420) / A4EC CP 6 / A4EE RET Z / A4EF LD A,C / A4F0 JP 509F`. Un balayage de la ROM (`CD/C3/C4/CC/D4/CA/C2 E8 A4`) ne trouve que quatre appels, tous `CALL A4E8` précédés de `LD A,id` : `A2FB` (08), `A34F` (07), `A388` (09) (homme-poisson, init `A2E0` et routine `A310`) et `B9AB` (0A, boule du boss 9). Tous les autres sons (Simon : `6D38`, `6F7E`, `76E5` ; armes, coups, objets...) passent par `509F` directement et **ne sont pas gardés**. `C420 = 6` est écrit en `7070` (fin du recul mortel) et en `76C6` (chute mortelle) : le garde couvre les deux. Trace (stage 2 salle 4 forcée, hommes-poissons actifs, Simon immobile, `tools`-style crochets sur `A4E8`, `A4EF`, `A4F0`, `509F`) : avec `C420 = 0` ou `5`, chaque entrée en `A4E8` atteint `A4F0` puis `509F` avec le même A (08 aux images 30, 46, 246 ; 07 à 100, 102, 152 ; 09 à 162, 234) ; avec `C420` forcé à 6 à chaque VBLANK (images 300-599), les deux entrées (07 image 316, 09 image 378) retournent en `A4EE` sans jamais atteindre `A4F0` ni `509F`. **Porté** : `vk_simon_pose6()` (`main.c` : `health.fell` ou `vk_game.recoil == VK_RECOIL_DEAD`) coupe les effets des hommes-poissons (`fishSounds`) et la boule du boss 9 (`actors.c`) ; scénarios `zb_a4e8_guard` (stage 2) et `zzd_stage9_boss` (0A). Non vérifié : la chute mortelle réelle n'a pas été rejouée avec un homme-poisson actif (le `C420 = 6` de la trace est forcé, mais le code de `A4EC` ne lit que cet octet) ; pas de boss 9 sous openMSX avec ce garde.

## 8. Fichiers

- Outils : `tools/extract_msx_sound.py`, `tools/convert_psg_vgm.py`, `tools/build_msx_sound_md.py` (table `SOUNDS` : noms, événements, appelants), `tools/measure_ay_envelope.py` (hauteur de l'enveloppe AY mesurée dans openMSX).
- Généré : `sgdk/src/generated/msx_sound_md.c/.h`, `research/audio/msx_sounds.json`.
- Lecteur : `sgdk/src/psg_core.c/.h`, `sgdk/src/psg_player.c/.h`, `sgdk/src/sound_test.c`.
- Mode FM : `sgdk/src/fm_core.c/.h`, `sgdk/src/psg_core_fm.c`, `tools/convert_fm.py`, `sgdk/src/generated/msx_fm_md.c/.h` (voir `research/audio-fm.md`).
- Tests : `tests/test_psg_core.c` (dans `tests/run_native.sh`), `tests/test_convert_psg_vgm.py`, `tests/test_extract_msx_sound.py` (ROM requise, `VK_ROM_PATH` : extraction, appelants de chaque son, enveloppe), scénarios MD `tests/md_scenarios/m_sound_events.txt`, `zm_title_sound_test.txt` (menu, effet 1D), `l_stage2_room4_fishman.txt` (08, 09), `zzd_stage9_boss.txt` (0A), `zr_stage3_boss.txt` (tir sans effet), `o_pause.txt` et `n_room_transition_freeze.txt` (musique pendant le dessin d'une salle).
- Retirés : `generated/sfx_md.*`, `generated/sfx_hit_md.*` (castlevania-md), `generated/music_md.*` (capture candidate) et leurs générateurs.
