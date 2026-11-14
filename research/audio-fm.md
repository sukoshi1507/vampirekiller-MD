# Musiques sur le YM2612 (mode FM)

Statut : **candidat, à valider à l'oreille** par l'utilisateur. Le mode PSG reste le défaut (`vk_sound_fm = 0`) et le code de ce mode est inchangé. Seules les **musiques** (80 à 8E) passent en FM ; les bruitages restent sur le PSG.

## 1. Principe

- Les 3 voies de musique du pilote MSX (AY A, B, C) jouent sur les canaux 1 à 3 du YM2612 (partie 0). Mêmes notes, mêmes durées, mêmes boucles : seul le timbre change.
- Les percussions de la MSX sont des trames de bruit AY (surtout voie C). Elles restent sur le canal de bruit du PSG, cadencé par le ton 2 (fréquence exacte du bruit AY, comme dans le rendu PSG quand la voie C est libre). Niveau : le bruit PSG au volume maximal vaut environ 0,25 de la crête FM (mesuré dans le cœur Genesis Plus GX : bruit à pleine puissance 0,123, crête FM 0,5), contre 1/3 (0,26 / 0,8) dans le prototype ; le PSG ne peut pas aller plus fort sans baisser le FM, ce qui changerait les niveaux validés.
- Comme le lecteur PSG : flux précalculés à la compilation, joués par le 68000 dans l'interruption verticale, une image à la fois, en phase avec la voie de musique.

## 2. Chaîne d'outils

| Fichier | Rôle |
|---|---|
| `tools/extract_msx_sound.py` | + `notes` par image : voies de musique (C095 = 0 à 2) qui commencent une note (point d'arrêt `8AF1`) |
| `tools/convert_fm.py` | convertisseur AY → YM2612, repris du prototype validé (`fm.py`, jeu `A_cuivres`) |
| `tools/convert_psg_vgm.py` | + option `noise_only` : flux PSG des percussions seules |
| `tools/build_msx_sound_md.py` | génère `sgdk/src/generated/msx_fm_md.c/.h` (jamais édité à la main) ; `msx_sound_md.c/.h` reste identique octet pour octet |
| `sgdk/src/fm_core.c/.h` | cœur testable sur l'hôte : suit le flux FM en phase avec le cœur PSG |
| `sgdk/src/psg_core_fm.c` | seconde compilation de `psg_core.c` (voir 4) |
| `sgdk/src/psg_player.c` | couche Mega Drive : écriture YM2612, interrupteur |

### Convertisseur (reprise exacte du prototype)

- Voie active dans une image : ton autorisé au mélangeur, volume (R8 à R10 & 15) > 0, période > 1.
- Début de note sur une voie active : key off, puis écriture de la fréquence, du TL des porteuses, key on.
- Fréquence écrite quand la période change : `f = 1 789 772,5 / 16 / période`, `fnum = f × 2^21 / (7 670 453 / 144)`, bloc relevé jusqu'à `fnum < 2048`.
- TL des porteuses = TL de l'instrument + 4 × (15 − volume AY), écrit quand le volume change ; key off quand la voie devient inactive.
- Ordre des écritures par image (celui du tri du prototype) : tous les key off, puis toutes les fréquences (A4 puis A0), tous les TL, tous les key on, chaque groupe par voie. Registres d'opérateur dans l'ordre `OPOFF` (S1, S2, S3, S4 : 0, 8, 4, 12).
- Instruments : mélodie (AY A) `saw_lead`, basse (AY B) `bass_slap`, harmonie (AY C) `saw_soft`, valeurs exactes du dictionnaire `P` du prototype (algorithme 4, retour 5 ou 6, porteuses S2 et S4).
- Deux écarts voulus : (1) le registre 2B (activation du DAC) n'est pas écrit : le canal 6 appartient au pilote PCM (cri de Wilhelm) ; (2) l'image de boucle restitue l'état complet (fréquence et TL de chaque voie active, et key off + key on seulement pour une voie dont l'état de touche diffère entre l'arrivée depuis le début et depuis la fin), comme le flux PSG, pour que la boucle soit juste dans les deux cas.

### Format du flux FM (`vk_fm_music[i]`, musique 80h + i)

Par image à 60 Hz : un octet `n` (0 à 63) puis `n` paires (registre, valeur) de la partie 0. `0xFF` termine ; le flux reprend à l'octet `loop` (`0xFFFF` : pas de boucle). Même nombre d'images et même image de boucle que le flux PSG (testé pour les 15 musiques). `vk_fm_noise[i]` est le flux PSG de la même musique avec les percussions seules (format de `psg_core.h`, mêmes drapeaux NOTE_C et verrou). `vk_fm_init` : registres d'instrument (95 paires), écrits une fois.

Taille ajoutée : 225 396 octets de flux (FM 195 k, bruit 30 k) ; `text` du ELF 1 290 856 → 1 517 160 (+226 304 o), bss +~0,4 Ko ; la ROM passe de 1,25 à 1,5 Mo (alignement de 128 Ko).

## 3. Règles du pilote MSX en FM (`fm_core.c`)

`vk_fm_core_step` exécute une image du cœur PSG (inchangé, avec le flux de percussions) puis avance le flux FM d'une image au même moment, d'après l'état du cœur PSG avant et après l'image :

- fin de musique sans boucle : toutes les voies coupées ; boucle : même image que le flux PSG ;
- priorité des effets : quand un effet est accepté, la voie C (canal 3) est coupée (key off) et ses écritures sont ignorées (état gardé) tant que l'effet joue et jusqu'à la note suivante de cette voie (drapeau NOTE_C, `c_muted` du cœur PSG) : la note écrit alors sa fréquence, son niveau et son key on ;
- pause FD / arrêt du temps FB : toutes les voies coupées, flux gelé ; FE / FC : les notes tenues sont remises en route (key on ; l'enveloppe repart, la fréquence et le niveau sont encore dans la puce) ;
- fondu FF : TL des porteuses + 4 par cran de 3 dB (16 crans), aussi sur les écritures du flux ; à la fin du fondu, voies coupées ;
- arrêt (00) et musique PSG qui remplace une musique FM : key off immédiat des trois voies (`vk_psg_stop`, `fm_cancel`).

## 4. Couche Mega Drive

- **Pilote Z80 de SGDK** : `main.c` charge `SND_PCM_loadDriver` (cri de Wilhelm sur le DAC). Lu dans `drv_pcm.s80` : il écrit le DAC en permanence (échantillon nul quand rien ne joue) en positionnant **une fois** le verrou d'adresse du YM2612 sur 2A puis en écrivant seulement des données. Une écriture du 68000 qui change le verrou enverrait les échantillons suivants dans le mauvais registre. Donc, pour chaque lot d'écritures : `Z80_getAndRequestBus(TRUE)` (laisse le bus s'il est déjà pris par la boucle principale, comme `SND_PCM_startPlay`), `YM2612_writeReg(0, reg, val)` (attente du bit busy lue dans `ym2612.c`), puis **`YM2612_write(0, 0x2A)` pour remettre le verrou**, puis `Z80_releaseBus` si on l'a pris. Le canal 6 et le registre 2B ne sont jamais écrits.
- **Interruption** : `vk_psg_set_fm(1)` installe `fm_vint` au lieu de `psg_vint` (même lecteur avec le flux FM). En mode PSG, `psg_vint` est celui d'avant : le coût de l'interruption, auquel le rejeu 12-18 (images exactes) est sensible (+12 et +32 cycles passent, +96 casse à l'image 4862), n'a pas bougé. Pour cela `fm_core.c` appelle une seconde compilation de `psg_core.c` (`psg_core_fm.c`, `vk_psg_core_fm_frame`, avec `#pragma GCC optimize ("no-ipa-icf")`) : un second appelant de `vk_psg_core_frame` l'aurait sorti du corps de `psg_vint` où le compilateur l'insère.
- **Interrupteur** : `vk_sound_fm` (`psg_player.h`, 0 au démarrage), ligne « SOUND < PSG > » / « SOUND < FM > » de l'écran titre (police SGDK, aucune tuile du budget `MSX_TEXT_TILE_MAX`), LEFT ou RIGHT la change. Le code Konami garde ses LEFT et RIGHT : tant qu'un code est commencé (`vk_konami_pending`), ils ne changent rien. Aucune touche du titre n'a changé de rôle ; LEFT/RIGHT, sans effet avant, règlent le son. Le choix reste pour la session (partie, game over, retour au titre). Le test sonore joue les musiques dans le mode courant.
- Au premier passage en FM, les registres d'instrument sont écrits tout de suite (écran titre), pas à la première note.

## 5. Coût mesuré

ROM de mesure (`EXTRA_FLAGS=-DVK_MEASURE_PSG`, `tools/measure_fm_cost.txt`, cœur Genesis Plus GX NTSC H32) : le pas suivant est refait sur une copie, bus Z80 et attentes du YM2612 compris (les écritures de la mesure vont au registre inutilisé 2F), chronométré au compteur HV (171 pas par ligne de 488,6 cycles). Les 15 musiques par le test sonore, 22 843 pas :

| Paires YM2612 dans le pas | 0 | 2 | 6 | 12 | 18 |
|---|---|---|---|---|---|
| Pas le plus long (lignes) | 8,0 | 12,6 | 14,3 | 21,9 | 26,4 |

- Moyenne : 1 078 pas HV, soit 6,3 lignes (le mode PSG : 3,9).
- **Pire cas observé : 4 520 pas HV, 26,4 lignes (~12 900 cycles)**, 18 paires (trois voix qui commencent une note ensemble : 694 images de flux sur 26 764, soit 2,6 %). Une image peut en ajouter quelques-unes (effet : +1 paire, fondu : +6) : environ 33 lignes au pire théorique, pour 38 lignes de blanc vertical NTSC ; le PSG seul : 15,3 au pire.
- Le coût est dominé par l'attente du bit busy du YM2612 (~530 cycles par paire dans le cœur).
- **Vérifié** : le rejeu 12-18 lancé avec le mode FM (`VK_SOUND_FM=1`, ajoute un appui RIGHT au démarrage) ne perd aucune image sur les 4 542 tours (stages 12 à 13:0, marge médiane 124 lignes, minimum −8 comme en PSG), puis échoue au premier point de contrôle parce que le rejeu n'a pas été enregistré dans ce mode. **Non vérifié** : le reste de la partie en FM, ni sur console réelle (le temps de busy y est celui de la puce, la contention du bus Z80 avec le DAC n'est pas émulée).

## 6. Vérification

- `tests/test_convert_fm.py` : musique 80 rejouée par le cœur (`tests/fm_core_dump.c`), image par image les mêmes écritures que `fm.py` du prototype (`VK_FM_PROTO`, défaut `scratchpad/fm_proto/src` du dépôt principal), même ordre, y compris à travers la boucle ; registres AY extraits égaux à `m80.json` ; même nombre d'images et même boucle que le flux PSG pour les 15 musiques ; pour chaque note tenue de chaque musique, bloc/fnum à 1 % de la période AY.
- `tests/test_fm_core.c` (dans `tests/run_native.sh`) : démarrage, effet sur la voie C et reprise à la note suivante, pause, FB/FC, fondu, fin sans boucle, remplacement par une musique PSG, 15 musiques sans débordement.
- `tests/md_scenarios/zzu_sound_fm.txt` : bascule PSG/FM au titre, Konami intact, musique 8A puis 80 en FM, curseur FM qui avance, effet 05 sur le PSG, pause gelée puis reprise, boucle, remplacement à la mort (89 puis 80).
- `tools/md_harness.py --wav` enregistre l'audio du cœur ; `tools/record_stage1_fm.txt` produit `scratchpad/fm_proto/rom_stage1_fm.wav` (45 s depuis la ROM, mode FM, niveaux bruts non normalisés : crête 0,5 contre 0,8 pour le WAV du prototype).

## 7. Limites

- Candidat : le rendu FM n'a pas été écouté sur ce dépôt ; la fidélité des notes est vérifiée par les tests ci-dessus, le timbre est celui du prototype.
- Au rétablissement après pause, FB ou fondu, une note tenue repart avec son attaque (key on), alors que l'AY ne fait que rétablir ses registres.
- Les sons FB et FD (arrêt du temps, jingle de pause) restent sur le PSG (ton 0).
- Cri de Wilhelm : le DAC est coupé quelques microsecondes à chaque lot d'écritures (bus Z80 demandé) ; non écouté.
- Rejeu 12-18 : valable pour le mode PSG seulement.
