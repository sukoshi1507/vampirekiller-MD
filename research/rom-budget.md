# Budget ROM : place dans la collection 4 Mio et gains possibles (etude, rien n'est modifie)

Etat mesure : worktree a92816c (`sgdk/out/rom.out`, `make GDK=...`, SGDK 2.11, GCC `-O3 -flto`).
Outils ajoutes par cette etude : `tools/rom_budget.py` (inventaire + `--check`), `tests/run_rom_budget.sh`
(0,1 s, lit l'ELF existant), `tools/rom_compress_probe.py` (compression mesuree avec les jars SGDK et cout
de decompression 68000 modelise). Aucune donnee du jeu, aucun fichier de `sgdk/src` n'a ete touche.

## 1. La place restante

La collection (`vampirecollection-md`, `Makefile`, `collection/merge_rom.py`) met Castlevania-MD a 0 et Vampire Killer
a `VK_BASE = 0x214000`, sans mapper : Vampire Killer doit finir avant `0x400000`.

| element                                             | octets    |
|-----------------------------------------------------|----------:|
| plafond de Vampire Killer = 0x400000 - 0x214000     | 2 015 232 |
| image Vampire Killer (`objcopy -O binary`), liee a 0 | 1 885 668 |
| idem liee a `VK_BASE` avec `vk.ld` de la collection (mesure, build temporaire) | 1 885 672 |
| **libre**                                           | **129 564 (126,5 Kio)** |

Castlevania-MD finit a 0x211C04 (`out/collection/castlevania.bin`, 2 169 860 o) : 9 212 o de marge sous `VK_BASE`.
Le chiffre « ~30 Kio » de la demande ne se retrouve pas : meme en comptant la ROM remplie a 128 Kio
(`rom.bin` = 0x1E0000), il resterait 49 152 o. Le dernier `vampirekiller.bin` de la collection (17:13, sources plus anciennes) faisait
1 954 540 o (60 692 libres) : les sources de `main` ont depuis perdu ~69 Kio. `tools/rom_budget.py` relit `VK_BASE` dans le Makefile de la collection
(`VK_COLLECTION=...`, ou `VK_BASE=0x...` pour un autre plafond) ; `--check [--margin N]` echoue si la place libre est sous N.

Repartition (nm, `tools/rom_budget.py --top 5`) :

| categorie       | Kio   | %     | gros symboles |
|-----------------|------:|------:|---------------|
| cartes          | 463,2 | 25,2  | vk_maps 214 016, late_maps 64 512, vk_stageN_collision 60 672, g16_*_map 109 120 (155 x 704) |
| code (+petites tables) | 357,4 | 19,4 | vk_show_player 23 702, Enemies_frame 22 100, main 16 316, step_boss 16 132, vk_step_enemies 15 668 |
| tuiles 16 bits  | 267,9 | 14,5  | vk_gfx16_pool 194 272, vk_intro16_sprites 26 868 |
| son FM          | 220,1 | 12,0  | msx_fm_80..8e 195 779 + msx_fm_noise_* 29 233 |
| sprites         | 205,1 | 11,1  | vk_dracula_bust_sets 33 280, jeux de poses Simon (shield, whip, crouch) ~80 Ko |
| tuiles MSX      | 172,3 |  9,4  | vk_tiles 43 008, vk_stageN_tiles 56 832, scene_tiles 34 272, vk_intro_frames 15 360 |
| son PSG         |  69,8 |  3,8  | msx_sound_80..8e 68 271 (musiques du mode PSG) |
| PCM             |  34,2 |  1,9  | wilhelm_pcm 35 072 |
| SGDK            |  31,5 |  1,7  | vsprintf 4 602, drv_pcm 2 866 |
| autres          |  19,8 |  1,1  | vk_palettes 4 864, textes |

(`nm` ne distingue pas le code d'une petite table : la categorie « code » contient quelques Kio de tables.)

## 2. Mesures par bloc

Methode : octets reels de l'ELF, compresses par `lz4w.jar p` et `apj.jar pp` (aPLib maximum) ; le decodeur aPLib du modele est verifie octet pour
octet contre l'original, le modele LZ4W relit la longueur decompressee. Cout = cycles 68000 de `lz4w_unpack` / `aplib_unpack` de `tools_a.s`
comptes sur les flux reels (instruction par instruction, sans etats d'attente, +-15 %) ; ligne = 488 cycles, image = 262 lignes (~128 000 cycles).
LZ4W : 7 a 13 cycles par octet (la doc SGDK dit 550-950 Ko/s, soit 8 a 14) ; aPLib : 33 a 165 cycles par octet selon le contenu
(cartes ~66-100, tuiles ~90-150), soit 8 a 12 fois plus lent.

RAM libre : `_bend` = 0xFFC199, il reste 15 975 o jusqu'au haut de la pile (pile SGDK comprise) : un tampon de decompression est limite a ~6-8 Kio.

### Tableau recapitulatif

| bloc (taille)                                     | technique                              | gain mesure | cout | moment | risque |
|---------------------------------------------------|----------------------------------------|------------:|------|--------|--------|
| `msx_fm_*` + `msx_fm_noise_*` (225 012)           | **motifs d'images** : appel (3 o) vers une suite d'images deja ecrite, plus repetition d'images vides (2 o) | -137 000 (fm -115 000, bruit -22 000) estime | ~0 (un test par image dans la V-Int, aucune RAM) | en jeu, V-Int | moyen : format lu par `fm_core`/`psg_core` (tests natifs) |
| idem, LZ4W en flux par blocs de 4 Kio             | decompresser la musique en tampon      | -182 000 (aPLib -196 000) | 4 Kio x ~9 c/o = ~76 lignes par bloc toutes les ~3-6 s ; 2 tampons de 4 Kio | en jeu | eleve : pointes de 76 lignes, RAM |
| `msx_sound_8x` (68 271, mode PSG)                 | motifs d'images (meme code)            | -40 000 estime | ~0 | en jeu | moyen (meme format) |
| `vk_gfx16_pool` (194 272)                         | **coherence de palette par etage** + doublons exacts (generateur seul) | ~ -95 000 estime (voir 2.4) | 0 a l'execution | build | moyen : octets de palette et de tuiles changent, pas l'image |
| idem                                              | miroirs H/V/HV                          | -4 300 (136 tuiles) | 0 mais cartes u8 -> u16 (+109 Ko) | | a ecarter |
| idem                                              | LZ4W / aPLib du pool entier            | -61 000 / -94 500 | tampon 3 a 23 Kio impossible (RAM), 226 / 3 346 lignes par etage | | a ecarter |
| `vk_maps` (214 016, u16 dont l'octet haut est 0)  | u16 -> u8                              | -107 008 | ~0 (boucle d'ecriture de carte, voir 2.3) | dessin de salle | moyen (calibrage du dessin hors gel) |
| idem + LZ4W par salle (704 o)                     | u8 puis LZ4W                           | -175 800 (aPLib -179 500) | 18,8 lignes / salle (max 24,9) ; aPLib 148 | dessin de salle, ecran coupe | moyen |
| `late_maps` 64 512 + `vk_stageN_collision` 60 672 | LZ4W par salle (768 o) en tampon RAM de 768 o | -45 400 et -38 700 | 18,6 / 20,8 lignes par salle (max 26,6 / 27,4) | entree de salle | moyen : lues a chaque image par le jeu |
| `g16_*_map` (109 120, u8, 155 x 704)              | LZ4W par salle                         | -69 500 (aPLib -72 800) | 18,6 lignes / salle (max 25,1) ; aPLib 147 | dessin de salle (g16), ecran coupe | faible a moyen |
| `vk_dracula_bust_sets` (33 280)                   | **65 tuiles uniques sur 1 040** : 65 tuiles + 65 listes de 16 index | -30 200 | ~6 lignes par changement de buste (copie de 16 tuiles, 512 o, vers `row_words`) | salle de Dracula, a chaque changement de couches | faible |
| idem                                              | LZ4W par jeu de 512 o                  | -8 000 | 11,7 lignes (decompression) | | a ecarter |
| `wilhelm_pcm` (35 072)                            | reechantillonnage 11 025 Hz (+ queue silencieuse coupee) | -17 400 ; -22 700 coupe a 1,1 s ; 8 000 Hz coupe : -26 000 | 0 | chute de Simon | faible (timbre) |
| idem                                              | supprimer (et `drv_pcm` 2 866 + `SND_PCM_*` ~550) | -38 500 | 0 | | choix de produit |
| `vk_intro16_sprites` 26 868 + `vk_intro_frames` 15 360 | LZ4W par blocs de 2 Kio / 768 o   | -19 200 / -12 000 | 47 / 14 lignes par bloc, une fois toutes les 16 images | introduction | faible (aucun jeu en cours) |
| `vk_tiles` (43 008, 7 x 6 144)                    | LZ4W / aPLib par jeu                   | -15 200 / -23 100 | 143 / 1 808 lignes par jeu, tampon 6 Kio | dessin de salle (mode MSX) | eleve pour peu |
| `vk_stageN_tiles` (56 832, etages 12-18)          | LZ4W / aPLib par jeu                   | -42 100 / -44 900 | 212 / 1 481 lignes par jeu (max 293 / 2 188), tampon 6-9 Kio | idem | eleve |
| `scene_tiles_*` (34 272) ; `vk_stageN_enemy_tiles` (28 672) | LZ4W / aPLib par bloc        | -14 600 / -20 500 ; -17 000 / -20 700 | 122 / 72 lignes par bloc | scene / entree de salle | moyen |
| poses de Simon (`vk_simon16_*` 40 736, MSX ~40 000, DMA par pose) | LZ4W                  | -30 200 pour les 16 bits | 61 lignes par pose, chargees en jeu | **en jeu, a chaque pose** | a ecarter |
| code : `-Os` global                                | -214 000 (texte 1 885 544 -> 1 671 528) | -- | code plus lent | | eleve (timing) |
| code : `-O2` global                                | -140 000                               | -- | | | eleve |
| code : `-O3 -fno-inline-functions`                 | -84 000                                | -- | | | moyen |
| code : `-Os` sur les seuls fichiers froids         | -13 500                                | -- | | menus, carte, test du son, scenes | faible |
| `vsprintf` (SGDK, 4 602) | remplacer par un formatage de nombres | -4 000 | 0 | | faible |

### 2.1 Musiques (FM 225 Kio, PSG 68 Kio)

Les flux (`msx_fm_md.h`) sont lus vers l'avant a 60 Hz : par image un octet de comptage puis des paires registre/valeur ; la boucle est un decalage d'octets.
Compresses entiers, ils tombent a 11 % (LZ4W) et 6 % (aPLib), les bruits a 5,7 % : ils sont tres repetitifs. Mais une decompression
entiere est impossible en RAM (35 Kio pour le plus gros), et par blocs la repetition lointaine se perd (1 Kio : 34 %, 2 Kio : 25 %, 4 Kio : 19 % en LZ4W).

Le gain sans decompression vient d'un format a motifs. Simulation (script jetable, non commite ; glouton, sources litterales seulement donc profondeur d'appel 1) : decouper
chaque flux en images, remplacer toute suite d'au moins 4 images deja ecrite par un appel de 3 octets (image de depart sur 2 octets, longueur) et toute serie de 3 images identiques
d'un octet (silences) par 2 octets. Resultat (plus petit seuil 3 ; resultats presque identiques de 3 a 12) :

| flux             | avant   | apres   | gain    |
|------------------|--------:|--------:|--------:|
| FM (msx_fm_8x)   | 195 779 |  80 651 | 115 128 |
| bruits (noise)   |  29 233 |   7 551 |  21 682 |
| PSG (msx_sound_8x) | 68 271 |  28 329 |  39 942 |

Dans le lecteur, une suite d'appel de 3 octets coute un test par image (FM : `n` <= 63, les octets 0x40.. sont libres ; PSG et bruit : les bits 6-7 de l'entete sont des drapeaux, il faut un code d'echappement libre, par exemple un comptage de 63 jamais produit, a verifier) et une pile d'un retour ;
aucune RAM, aucune pointe de temps. FM et bruit avancent images pour images ensemble (`vk_fm_core_step`) : le meme codage doit s'appliquer aux deux, avec une image
de boucle qui demarre un segment litteral.

### 2.2 `wilhelm_pcm`

Utilise uniquement par `main.c:921` (cri a la chute de Simon, `SND_PCM_startPlay` ; ajout du portage absent de la ROM MSX) et le pilote Z80 `drv_pcm`
charge au demarrage (`main.c:704`). Le Z80 lit la ROM directement : aucune decompression possible (LZ4W 78 %, aPLib 66 %, de toute facon). Le signal : 8 bits, 22 050 Hz, 1,59 s ;
99 % de l'energie sous 3,5 kHz, 99,9 % sous 5,3 kHz ; les derniers ~30 % de la duree (apres 1,1 s) sont a amplitude <= 3/127 (queue d'extinction). Donc 11 025 Hz (`SOUND_PCM_RATE_11025`) divise la taille par deux
sans perte audible utile ; couper la queue apres 1,1 s en gagne encore 5 Kio. Alignement 256 o a conserver.

### 2.3 Cartes

`vk_maps` : `const u16 vk_maps[152][704]`, mais toutes les valeurs sont < 192 (octet haut toujours 0, verifie par `rom_compress_probe.py --lowbyte`).
Lectures : `room_view.c` (`VDP_setTileMapDataRectEx`, dessin de salle en mode MSX qui est le mode par defaut, `vk_gfx16 = 0`), `item_view.c` (`room_map`, 4 cellules pour la cle de l'etage 2),
`gfx16.c vk_room_cell` (mode MSX seulement). Passer a u8 demande une boucle d'ecriture comme `vk_gfx16_draw_map` : elle existe deja pour les cartes 16 bits et son cout est le meme que celui des
salles 16 bits. La difference de duree du dessin hors gel (reapparition, debut d'etage, saut : calibre sur l'ancienne duree, `research/room-draw-budget.md`) est a mesurer avant de generaliser.
Les cartes 16 bits (`g16_*_map`, u8) different de `vk_maps` sur 23 % des cellules (82 368 / 107 008 egales) : un codage par difference ne fait pas mieux que LZ4W.

`late_maps` / `vk_stageN_collision` (768 o par salle) sont lues par le jeu a chaque image (`vk_late_room_map`, `gameplay.c`) : un tampon RAM de 768 o rempli a l'entree de salle convient (le tampon doit
rester valide tant que l'ancienne salle sert pendant un passage de porte : a verifier dans `room_manager.c`).

### 2.4 `vk_gfx16_pool` (194 272 o, 6 071 tuiles)

Les tuiles sont deja uniques (aucun doublon exact). Mesures :
- miroirs H/V/HV : 5 935 tuiles distinctes (-136, -4 352 o) ; les cartes contiendraient les bascules (u16 au lieu de u8 : +109 Ko) : perdant.
- aPLib 51 %, LZ4W 68 % : le pool serait a decompresser par etage (19 groupes de 3,4 a 22,8 Kio, tampon de 22 Kio impossible), 3 346 lignes par etage en moyenne (8 123 au pire).
- **Les memes dessins existent en plusieurs « teintes d'indices »** : le generateur donne a chaque salle sa propre palette (15 couleurs + noir), les tuiles qui ne different que par
  l'ordre des indices ne sont pas fusionnees. Converties en RGB, il n'y a que 2 914 tuiles exactes distinctes (par etage ; 2 179 en tout, 2 850 a un miroir pres) pour 6 071 dans le pool,
  soit 93 248 o au lieu de 194 272. Faire coherentes les palettes d'un etage (meme couleur = meme indice dans toutes ses salles) exige un coloriage de graphe des couleurs par etage :
  les etages ont 15 a 24 couleurs (hors noir) pour 15 indices ; un coloriage glouton passe sur 9 groupes sur 19 et demande 16-17 indices sur les autres (un coloriage exact ou une fusion de
  couleurs proches devra le resoudre ; les salles non resolues gardent leurs tuiles propres). Gain realiste : ~95 Kio, zero cout d'execution. Le rendu reste identique en RGB, mais la CRAM et les index VRAM changent.

### 2.5 Blocs de la salle de Dracula et de l'introduction

`vk_dracula_bust_sets` : 65 jeux de 16 tuiles (65 combinaisons ordonnees des couches 1..4) ; compresses ensemble 4,7 % (LZ4W), mais seulement 65 tuiles distinctes parmi 1 040 (33 a un miroir pres) :
le code (`dracula_view.c bust`) peut copier les 16 tuiles du jeu dans `row_words` (tampon de 1 Kio existant) puis `VDP_loadTileData` en DMA en file, ~5-7 lignes de plus que le DMA seul, loin des 50 a 120 lignes de la composition d'origine.

`vk_intro16_sprites`, `vk_intro_frames` : lus dans l'ordre du temps par `scene.c` (4 478 sprites 16 bits, 320 images) ; un bloc de 2 Kio decompresse tous les ~16 images pendant l'introduction (aucune logique de jeu, aucune
salle) : 47 lignes (sprites), 14 lignes (images) dans un tour de 250.

### 2.6 Code

Texte 1 885 544 o ; `code` ~366 Kio. Variantes mesurees (build LTO complet dans un dossier temporaire, taille seulement) :

| options                                              | texte     | gain    |
|------------------------------------------------------|----------:|--------:|
| actuel : `-O3 -flto` (SGDK)                          | 1 885 544 | 0       |
| `-O3 -fno-ipa-cp-clone`                              | 1 884 776 | -768    |
| `-O3 -fno-inline-functions`                          | 1 801 576 | -84 000 |
| `-O3 -fno-inline-functions -fno-ipa-cp-clone`        | 1 798 248 | -87 300 |
| `-O3` sans peel/unswitch/inline/cp-clone             | 1 782 120 | -103 400 |
| `-O2`                                                | 1 745 512 | -140 000 |
| `-O2 -fno-inline-functions`                          | 1 719 912 | -165 600 |
| `-Os`                                                | 1 671 528 | -214 000 |

Pourquoi les fonctions sont si grosses : `-O3` ajoute `-finline-functions` (toute fonction `static` appelee de plusieurs endroits est copiee dans chaque appelant : `vk_show_player` inline les sprites de chaque type d'ennemi,
23 702 o en `-O3`, 8 816 en `-Os` ; `Enemies_frame` 22 100 -> 12 434 ; `vk_step_weapons` 11 684 -> 3 864 ; `dracula_step` 11 448 -> 5 096 ; `vk_step_enemies` 15 668 -> 10 370 ; `main` 16 316 -> 11 718), le deroulage/pelage de boucles
et le « unswitch ». Les `.constprop.N` (clones d'appel a argument constant, `-fipa-cp-clone`) ne sont pas en cause : les desactiver ne gagne que 768 o. Au niveau des fichiers (compilation sans LTO,
`.text` seul) : enemies.c 79 700 -> 32 308 (-Os) / 52 098 (-O2), boss.c 37 700 -> 11 742 / 18 078, actors.c 25 356 -> 15 938 / 21 802, candles.c 22 900 -> 7 838 / 10 190, dracula.c 20 150 -> 5 906 / 7 442,
hud.c 13 662 -> 2 842 / 3 486, main.c 13 672 -> 8 660 / 12 824, gameplay.c 11 614 -> 4 440, player.c 10 152 -> 3 858 ; au total 327 428 o en `-O3`, 200 834 en `-O2`, 137 440 en `-Os`.

Ce chiffre est une taille, pas une vitesse. Les fonctions chaudes (tour de jeu, `vk_show_player`, ennemis, boss, `vk_step_*`) sont celles qui gagnent le plus et celles dont le timing compte : marge minimale 6 lignes
sur les salles 12-17, rejeu 12-18 image exacte, dessin de salle calibre (`research/room-draw-budget.md`, `sgdk/src/profile.h`). Tout changement de code doit etre verifie par
`vk_late_turns` / `vk_last_slack` sur les memes scenarios avant d'etre accepte. Les fichiers froids (carte d'etage `stage_map.c` 5 988 -> 1 304, `stage_map_view.c` 4 640 -> 2 070, `scene.c`,
`stageNN_scene.c`, `item_reveal_view.c`, `sound_test.c`, `stage12_secret.c`) donnent -13 500 o en `-Os` sans toucher au temps de jeu (`__attribute__((optimize("Os")))` par fonction ou drapeau par fichier ;
`__attribute__((noinline))` sur les gros corps rares : attaques du boss, fins, boutiques).

## 3. Cout de decompression en lignes d'ecran

Une image = 262 lignes ; le dessin de salle pendant le gel tient en 2 images (n_room_draw_margin exige <= 4), budget 7 : ~1 300 lignes de marge (hors rejeu calibre). La decompression
ne sert que si elle a lieu :

- **ecran coupe, dans le gel** (dessin de salle : `vk_gfx16_draw_map`, tuiles, cartes `g16_*_map`/`vk_maps`, tampon de 704 a 768 o) : 19 a 27 lignes par salle en LZ4W, 150 a 220 en aPLib : tient, avec la reserve que les dessins hors gel
  (reapparition, debut d'etage, saut d'etage, boss) ont un tour de dessin calibre sur l'ancienne duree (zzj_ghost5, zzb_stage8 : le tour de dessin compte pour un pas de jeu).
- **en jeu, sur une image** : LZ4W jusqu'a 20-30 lignes de pointe par bloc est a comparer a la marge minimale de 6 lignes des salles 12-17 : a eviter ; aPLib (80-150 c/o) impossible.
- **introduction** : aucune contrainte de jeu, 14 a 47 lignes.
- **a l'octet pret, sans decompression** (motifs de musique, doublons de tuiles, u8, palettes) : 0 a 7 lignes.

## 4. Recommandation, gains sûrs d'abord

| # | action | gain | cout / risque |
|---|--------|-----:|---------------|
| 1 | PCM : 11 025 Hz, queue coupee (`wilhelm.json` / `wilhelm.pcm` regeneres) | -17 400 a -22 700 | 0 ; seul le timbre change |
| 2 | Salle de Dracula : 65 tuiles + listes d'index (`build_dracula_background.py`, `dracula_view.c`) | -30 200 | +5 a 7 lignes par changement de buste ; scenarios Dracula a rejouer |
| 3 | Format de musique a motifs (appel + repetition d'images vides) pour FM, bruit et PSG | -137 000 (+40 000 PSG) | ~0 ; `fm_core`/`psg_core`, tests natifs, aucun changement sonore (decodage identique) |
| 4 | Generateur : coherence des palettes par etage et pool fusionne (`build_gfx16.py`) | ~ -95 000 | 0 a l'execution ; image identique, CRAM/VRAM differents, scenarios a reprendre |
| 5 | `vk_maps` u8 (+ `vsprintf` remplace, `-Os` sur les fichiers froids) | -107 000 ; -4 000 ; -13 500 | boucle d'ecriture a chronometrer ; le reste sans effet de jeu |
| 6 | Introduction : LZ4W par blocs (sprites, images) | -31 000 | 14 a 47 lignes pendant l'intro |
| 7 | Cartes de collision/16 bits en LZ4W par salle (`late_maps`, `vk_stageN_collision`, `g16_*_map`) | -84 000 ; -69 500 | 19 a 27 lignes au chargement de salle, tampon de 768 o ; a valider salle par salle sur le rejeu 12-18 |
| 8 | Code `-O2` sur tout (ou `-O3 -fno-inline-functions`) | -140 000 (-84 000) | seulement apres mesure du timing (rejeu 12-18, marge 6 lignes) |
| 9 | Supprimer le cri Wilhelm et le pilote PCM | -38 500 | decision de produit |

Les etapes 1 a 5 donnent ~ -300 000 o (sans les mesures de temps, tous ces gains sont a cout de jeu nul) et laissent ~ 430 Kio libres dans la collection contre 126,5 Kio aujourd'hui ;
les etapes 6 a 8 ne servent qu'en dernier recours. A ne pas faire : miroirs du pool 16 bits, compression des poses de Simon (chargees a chaque pose), aPLib en jeu.

## 5. Reproduire

```
python3 tools/rom_budget.py --top 5                  # inventaire ; --check [--margin N]
tests/run_rom_budget.sh                              # test (ne reconstruit que si sgdk/out/rom.out manque)
python3 tools/rom_compress_probe.py --unit 1408 vk_maps              # LZ4W/aPLib + cycles
python3 tools/rom_compress_probe.py --lowbyte --unit 1408 vk_maps    # u16 dont l'octet haut est 0 -> u8
python3 tools/rom_compress_probe.py --per-symbol 'vk_stage??_tiles'
```
