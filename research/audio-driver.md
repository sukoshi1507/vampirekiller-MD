# Pilote son MSX2 de Vampire Killer : désassemblage, extraction et portage Mega Drive

Statut global : **pilote prouvé** (lecture du code et émulation conforme à la trace openMSX), **données extraites pour tous les sons** (28 effets, 15 musiques, 2 sons spéciaux), **branchement partiel**. L'identité de plusieurs sons vient seulement du contexte de l'appel ; elle est marquée **déduit** ou **PROVISOIRE**.

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

Pointeurs relus dans la ROM : `research/audio/msx_sounds.json` (`effect_pointers`, `music_pointers`). Appelants : tous les `CALL/JP 509F` des banques 0 à 3 (les banques 4 à 15 n'en ont aucun), plus les gestionnaires de ramassage `8D3E` qui renvoient l'identifiant dans A vers `509F`.

### 3.1 Effets (voie C)

| Id | Flux | Appelant(s) | Contexte | Statut de l'identité | Branché MD |
|---|---|---|---|---|---|
| 01 | `8E2B` | `B744` | une image sur deux tant que `C415` (énergie) < 20 : remplissage de l'énergie | déduit | non (pas de remplissage porté) |
| 02 | `8E58` | `94FC` | code objets banque 2 | PROVISOIRE | non |
| 03 | `8E8A` | `72F5` | toutes les 8 images, animation de Simon (`IX+6`) : pas dans l'escalier | déduit | non |
| 04 | `8E99` | `7470` | lancement d'un projectile depuis Simon (`C425`/`C427`) : arme secondaire | déduit | non (pas d'arme secondaire) |
| 05 | `8EAE` | `7225` | coup de fouet (`C42F = 6`) | **prouvé** (trace, 13/13 images identiques) | oui (`main.c`) |
| 06 | `8ECB` | `7489` | comme 03, autre sens d'escalier | déduit | non |
| 07 | `8EE4` | `6D38` (si `CFF0 ≠ 0`, soit `C428 ≥ 21`), `6F7E`, `A34D` | atterrissage | **prouvé** pour `6D38` (trace `scratchpad/fidelity/out1`) | oui (`main.c`, `VK_EVENT_LAND`) |
| 08 | `8EF3` | `A2F9` (banque 3, ennemi) | ennemi | PROVISOIRE | non |
| 09 | `8F25` | `76E5`, `A386` | chute dans l'eau (stages 2 et 10 ; ennemi sous Y C0) | prouvé (code) | oui |
| 0A | `8F56` | `B9A9` | ennemi (banque 3) | PROVISOIRE | non |
| 0B | `8F75` | `862F` | projectile qui touche un objet | PROVISOIRE | non |
| 0C | `8FB9` | `7FD8`, `7F65`, `7F99`, `7DD4`, `804C`, `80CF`, `8116`, `815E` | contact du fouet (bougie, ennemi, objet) | **prouvé** (trace `hit-sound.md`) | oui (bougies, ennemis) |
| 0D | `8FDD` | `7EBC` | contact d'un ennemi quand `C434 ≠ 0` (minuterie de l'objet 09) | déduit | non |
| 0E | `902E` | `87D8` | coup sur un bloc (mode ≥ 2) | prouvé (code) | oui |
| 0F | `9065` | `8DAF`, `8DB7`, `9344` | cœur ramassé | **prouvé** (trace `hit-sound.md`) | oui |
| 10 | `909A` | `936D`, `8E48` | points (objets 13, 14) | déduit | non |
| 11 | `90C7` | `8F7B` | coffre ouvert avec une clé dorée | prouvé (code) | oui |
| 12 | `90E6` | `8D8A`, `8E30`, `94EF` | objet ramassé (fouet, armes, objets à bits) | prouvé (code) | oui (fouet) |
| 13 | `9137` | `6F9A` | début du recul (dégâts), si `C002` bit 6 | prouvé (code) | oui |
| 14 | `9152` | `8E5D`, `8E68` | clé dorée ou blanche ramassée | prouvé (code) ; clé blanche par la trace `door-room7.md` (`8E60`) | oui |
| 15 | `9179` | `6DC5`, `7774` | porte : `772A` avec `C5AC = 5` (porte du château) ; HAUT devant une porte (`6DAE`) | **prouvé** (trace `castle-entrance.md`) | oui (porte du château) |
| 16 | `9212` | `8DDB` | objet 08 (minuterie `C43A`) | prouvé (code) | non |
| 17 | `9275` | `75ED` | `C43A` atteint 10 : fin de minuterie proche | déduit | non |
| 18 | `92D8` | `7414` | projectile, changement d'état | PROVISOIRE | non |
| 19 | `9470` | `938F`, `959E` | objets, texte (`490A`) | PROVISOIRE | non |
| 1A | `931A` | `776F` | porte de `772A` quand `C5AC ≠ 5` | déduit | oui, PROVISOIRE (porte de la salle 7) |
| 1B | `93CA` | `8DCA` | objet 05 | prouvé (code) | non |
| 1C | `939F` | `7E1C` | défaite d'un boss : jauge ENEMY `C418` à 0 (`7E17`), avec `C445 = 14h`, `7806` et `CE15 = 1` (voir `castle-candles.md` 9.8) | déduit (code) | non (pas de boss aux stages 0-2) |
| 1D | `8E69` | `934E` | objet mural `C5B5`/`C5C5`, résultat `C70C = 4` : cœurs - 5 (`45A0`) ; le pointeur tombe au milieu des données de 02 (`8E58`-`8E89`) | PROVISOIRE ; **non extrait** (hors de la plage 01-1C de l'outil) | non |

### 3.2 Musiques (voies A, B, C)

| Id | Appelant(s) | Contexte | Statut | Branché MD |
|---|---|---|---|---|
| 80 | `796F` (table `7972[D000]`) | stages 0 à 3 (cour, hall, stage 2, stage 3) | **prouvé** (trace : voies A/B identiques sur 7 649 images, boucle comprise) | oui |
| 81 | `7972` | stages 4-6, 11, 12 | prouvé (table) | (stages non portés) |
| 82 | `7972` | stages 7-9 | prouvé (table) | - |
| 83 | `7972` | stages 16, 17 | prouvé (table) | - |
| 84 | `7972` | stages 13-15 | prouvé (table) | - |
| 85 | `7972` | stages 10, 18 | prouvé (table) | - |
| 86 / 87 | `77FD` | entrée dans la salle du boss : 86 si `D000 = 12h` (dernier stage), sinon 87 | déduit | - |
| 88 | `6619` | séquence `CE00` (fin de partie gagnée ?) | PROVISOIRE | - |
| 89 | `70AA` | mort : arrêt (`00`, `70A5`) puis 89 quand `C428` arrive à 0 | prouvé (code) | oui |
| 8A | `4222` | état 3, sous-état 0 : avant le stage (départ) | **prouvé** (trace : 385/385 images identiques) | oui (château) |
| 8B | `42AB` | état 6 sans vie : fin de partie | prouvé (trace `player-health.md`) | oui |
| 8C | `B720`, `B72A` | séquence `CE02`/`CE10` (banque 3) | PROVISOIRE | - |
| 8D | `667F` | séquence `CE36` | PROVISOIRE | - |
| 8E | `66C9` | séquence `CE40` | PROVISOIRE | - |
| 8F | - | pointeurs vers un flux vide | - | - |

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
- Enveloppe (effets 02, 0B, 1A seulement) : enveloppe AY simulée, amplitude moyenne sur l'image. Enveloppe continue plus rapide qu'une image sur une voie sans ton audible : le ton SN prend la fréquence de l'enveloppe. **PROVISOIRE** (non écouté).
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
| Porte de la salle 7 | 1A | `776F` | `main.c` (**PROVISOIRE** : dépend de `C5AC`, non tracé) |
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
| Chute dans un trou ou dans l'eau | cri de Wilhelm (PCM, DAC) | **ajout du port**, absent de la ROM | `main.c` (voir §7.1) |

**Atterrissage d'un saut (6D26-6D38), prouvé le 2026-10-29.** `CFF0` n'est pas un reste des sondes de mur : la descente du saut (`C428 ≥ 9`, `6D13`) écrit `CFF0 := 0` à chaque pas (`6D17`), puis `CFF0 := 1` en `6D6F` quand `C428 - 2 ≥ 13h` (`6D1C`-`6D22`, appel de `6D6D`) ; la sonde de sol `7B88` n'écrit pas `CFF0`. Le son 07 du saut joue donc si l'atterrissage tombe au pas 21 ou plus. La table `6D95` a 19 pas et revient à la hauteur de départ au pas 19 ; au-delà, +6 par pas. Trace (`scratchpad/fidelity/out1`, Simon soulevé de 0, 0, 12, 24 et 40 px au départ du saut, point d'arrêt `6BA0`) : atterrissages au pas 19 sans 07 (deux fois), aux pas 21, 23 et 26 avec `CFF0 = 1` et une requête 07 à `509F` à la même image. Sens de jeu : **un saut qui retombe plus bas que son départ fait le bruit d'atterrissage**, pas un saut sur sol plat. Le port compte `C428` après la table (son saut passe en chute, mode 4, au pas 19) : `19 + ceil(images / 2)` (`gameplay.c`, `jump_step`), test `test_gameplay.c` (départs soulevés de 4, 12, 24, 40 px). PROVISOIRE : la table à 21 pas (`6D94`, `C431` bit 4, non portée) atteindrait le pas 21 même sur sol plat.

Non branchés (événements non portés) : 01, 02, 03/06 (escaliers : à brancher quand le pas d'escalier sera relevé), 04, 08, 0A, 0B, 0D, 10, 16-19, 1B, 1C, 81-88, 8C-8E, FF. Les sons 03 et 06 ne viennent pas des escaliers mais des projectiles des armes secondaires (`72DE`, `7473`), non portées. Tous sont jouables dans le menu de test son (code Konami au titre).

## 8. Fichiers

- Outils : `tools/extract_msx_sound.py`, `tools/convert_psg_vgm.py`, `tools/build_msx_sound_md.py`.
- Généré : `sgdk/src/generated/msx_sound_md.c/.h`, `research/audio/msx_sounds.json`.
- Lecteur : `sgdk/src/psg_core.c/.h`, `sgdk/src/psg_player.c/.h`, `sgdk/src/sound_test.c`.
- Tests : `tests/test_psg_core.c` (dans `tests/run_native.sh`), `tests/test_convert_psg_vgm.py`, `tests/test_extract_msx_sound.py` (ROM requise, `VK_ROM_PATH`), scénarios MD `tests/md_scenarios/m_sound_events.txt`, `o_pause.txt` et `n_room_transition_freeze.txt` (musique pendant le dessin d'une salle).
- Retirés : `generated/sfx_md.*`, `generated/sfx_hit_md.*` (castlevania-md), `generated/music_md.*` (capture candidate) et leurs générateurs.
