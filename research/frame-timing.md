# Calendrier d'affichage : du calcul d'une passe à l'écran (MSX2 et port MD)

Statut : **prouvé par trace openMSX** pour le MSX (2 exécutions, 2026-10-29) et **mesuré sur le harnais MD** pour le port, avant et après la correction. Correction **portée** : Simon, son fouet et les ennemis sont maintenant affichés deux images après le calcul, comme sur le MSX (`sgdk/src/actors.c`).

## 0. Méthode

- **Trace MSX** : `tools/run_frame_timing_trace.py ROM OUTDIR PLAN` (moteur `tools/vertical_links_trace.tcl` + `tools/frame_timing_trace_extra.tcl`), analyse `tools/analyze_frame_timing.py OUTDIR`. Openmsx 21, machine `C-BIOS_MSX2`. Plan : cour 0 → 1, `gotox 168`, fouet (ESPACE) sur la torche B, ramassage du cœur, deux fouets en l'air de parités différentes, deux sauts, hall, attente d'un contact, objet 05 posé sous Simon. Relevés : par VBLANK (crochet `4028`), `C003`, `C005`, `C006`/`C007`, X/Y, `C415`, `C417`, `C42D`, `C43E`, R#7 et la SAT entière lue en VRAM ; avec leur instant en µs depuis le VBLANK : `4BBB` (lecture des touches), `414D`, `6560`/`658A`, chaque départ de commande VDP (écriture de R#46 par le port 9Bh, avec DX, DY, NX, NY), `CHGCLR` (`0062`), les demandes de son `509F` et les écritures de `C096`, `C415`, `C417`, `C42D`. Sorties brutes hors dépôt (`scratchpad/ft/out1`, `out2` de la session).
- **Conventions MSX** : image N = image balayée après le VBLANK N. Une écriture VRAM faite t µs après le VBLANK N, sur la ligne y (212 lignes affichées sur 262), est dans l'image N si t < (50 + y) × 63,7 µs, sinon dans l'image N + 1 (calcul, non vérifié au pixel : les cas relevés sont loin de la limite, sauf la case du HUD). La SAT lue au VBLANK N + 1 est celle de l'image N. Un son demandé pendant l'image N part au VBLANK N + 1 (`8964` tourne au début de chaque interruption, `403B`, avant le garde `C005`).
- **Port** : `python3 tools/measure_md_frame_timing.py --out DIR` (harnais `md_harness.py`, Genesis Plus GX). Dans ce core, un `retro_run` commence par la V-Int puis balaie l'image : l'image du run r est l'image après la V-Int r ; le tour de boucle de `main.c` qui suit la V-Int r (« itération r ») a lu la manette de cette V-Int (`JOY_update`). Mesuré : un bouton tenu au run r0 change la RAM à l'itération r0 (fouet : `whip.timer` = 14 et son 05 demandés au run r0 ; marche : X + 1 au run r0).
- **Correspondance** : l'itération r du port correspond à la passe qui commence au VBLANK p du MSX. Vérifié sur la RAM : le port et le MSX changent X (marche), `C422`/`whip.timer` et le son du fouet dans la passe qui lit la touche, Y au 4e image (saut, `jump-compare.md`), la silhouette de 09/08 à la passe suivante.

## 1. MSX : où et quand chaque élément est produit

Une passe de jeu (`C003` + 1) dure 2 images : `4028` appelle `8964` (son) à chaque VBLANK, puis, si `C005` est libre, `4B9D` (touches) et `414D` (passe). La passe dépasse le VBLANK suivant (`C005` = 1 au VBLANK du milieu, la passe se termine dans la 2e image) ; les passes de chargement de salle durent 18 à 37 images.

| Élément | Tampon / écriture | Moment dans la passe p (trace) | Image à l'écran |
|---|---|---|---|
| Touches | `4BBB` lit la ligne 8 et le port joystick (`C007` tenues, `C006` nouvelles) | 0,5 à 2,6 ms après le VBLANK p (médiane 0,6 ms), seulement aux débuts de passe | — |
| Sprites (Simon, fouet, ennemis, flamme qui tombe d'une torche) | `D600` (Y par `78CC`, ~13 ms), `D400` (`790C`) ; envoyés par `6560`/`658A` au début de la passe suivante | envoi à 0,8-2,9 ms du VBLANK p + 2 | **p + 2** (et p + 3) |
| Décor en bitmap : torche éteinte, cœur posé ou ramassé, bloc | commandes VDP `HMMM` (D0h) sur la page 0, au moment du calcul | torche : VBLANK p + 1 + 4,9 ms (lignes 144-159) ; cœur ramassé : p + 1 + 1,5 ms | **p + 1** |
| HUD : barre ENEMY/PLAYER, cœurs, case d'objets, clé | commandes VDP (`C0`/`70`/`D0`) sur les lignes 0 à 31 | barre : p + 15,6 ms puis p + 1 + 0,7 à 4,3 ms ; cœurs : p + 1 + 5,7 ms ; case : p + 1 + 3,0 ms | **p + 1** (dernières commandes : p + 2) |
| Animation des flammes (torches, bougies) | `LMMM` (98h) | à chaque passe | p ou p + 1 |
| Bordure (objet 05, `7604`) | `CHGCLR` écrit R#7 tout de suite | p + 8,2 à 9,0 ms | **p à partir de la ligne ~80, p + 1 entière** |
| Son | `509F` (demande), `C096` ; `8964` au VBLANK suivant | fouet 05 : p + 8 à 9 ms ; coup 0C : p + 1 + 3,3 ms ; cœur 0F : p + 1 + 6,1 ms ; 1B (05) : p + 1 + 12,8 ms ; dégât 13 : p + 7,5 ms | **p + 1** (demande dans la 1re image) ou **p + 2** (dans la 2e) |

Entrées mesurées (touche enfoncée au VBLANK k0) : la lecture tombe à k0 si k0 commence une passe, sinon à k0 + 1 ; la première image où la SAT change (entrées triées : `6560` tourne leur ordre à chaque passe, `C00F`) est la lecture + 2 :

| Action | k0 | Lecture (p0) | 1re image changée | Retard depuis k0 |
|---|---|---|---|---|
| Fouet (cour 1) | 1186 | 1187 | 1189 | +3 |
| Marche droite | 1288 | 1289 | 1291 | +3 |
| Fouet en l'air | 1373 | 1373 | 1375 | +2 |
| Fouet en l'air | 1416 | 1417 | 1419 | +3 |
| Saut | 1478 | 1479 | 1485 (Y à p0 + 4) | +7 |
| Saut | 1561 | 1561 | 1567 | +6 |

Fouet sur la torche B (passe p0 = 1187) : son 05 au VBLANK 1188 (p0 + 1), première pose 1189 (p0 + 2), contact dans la passe 1189 : torche effacée dans l'image 1190, son 0C au VBLANK 1191 et pose du contact dans l'image 1191. La torche disparaît donc une image avant que la pose qui la touche soit affichée.

Contact (hall) : `4638` baisse `C415` à la passe 2239 (barre du HUD dans l'image 2240) ; `6F9F` pose `C42D` et demande le son 13h à la passe suivante 2241 (son au VBLANK 2242, Simon caché dans l'image 2243).

## 2. Port : où et quand chaque élément est produit

| Élément | Écriture | Image (itération r) |
|---|---|---|
| Manette | `JOY_update` à chaque V-Int, lue par l'itération qui suit | chaque image (le MSX : une sur deux) |
| Sprites | `VDP_updateSprites(..., DMA_QUEUE)`, envoi à la V-Int r + 1 | r + 1 (avant correction pour tous) |
| Tuiles sous le HUD (support des torches, blocs, message de la ligne 26) | `VDP_setTileMapXY` au CPU, tout de suite | **r** (mesuré : A+B rallume la torche A, cellules dans l'image r, flamme-sprite r + 1) |
| HUD (barre, chiffres) | tuiles au CPU, lignes 0 à 31 déjà balayées | r + 1 (mesuré : contact, barre à r + 1) |
| Couleur de fond (objet 05) | `vk_show_backdrop` juste après la V-Int r + 1 | r + 1 entière |
| Son | `vk_psg_effect`, lecteur en V-Int | V-Int r + 1 |
| Bougies, objets lâchés, case d'objets, clé du HUD, flammes des torches (bitmap sur MSX) | sprites (`candle_view.c`, `room_view.c`) | r + 1 |

## 3. Comparaison et décision

Retards depuis le début de la passe (MSX p) ou l'itération (port r) qui calcule l'élément :

| Élément | MSX | Port avant | Port après |
|---|---|---|---|
| Simon, fouet, ennemis (sprites MSX) | p + 2 | r + 1 | **r + 2** |
| Bougies, objets lâchés, case et clé du HUD, flammes des torches (bitmap MSX, sprites MD) | p + 1 | r + 1 | r + 1 |
| Décor sous le HUD (tuiles MD) | p + 1 (p si dessiné tôt dans la 1re image) | r | r |
| HUD (barre, chiffres) | p + 1 (parfois p + 2) | r + 1 | r + 1 |
| Bordure / fond (05) | p (dès la ligne ~80), p + 1 entière | r + 1 | r + 1 |
| Son | p + 1 ou p + 2 selon le moment de la demande | r + 1 | r + 1 |
| Touche → 1re image de Simon changée | +2 ou +3 (lecture une image sur deux) | +1 | **+2** |

Avant la correction, seuls les sprites MSX étaient en avance d'une image sur tout le reste : Simon caché ou en silhouette une image avant la barre du HUD et le son, pose du fouet affichée en même temps que son son (MSX : une image après), objet 05 qui allume le fond en même temps que les sprites au lieu d'une image avant. Le retard global (touche → image) était aussi plus court d'une à deux images.

**Implémenté** (`actors.c`, `vk_show_player`) : la table envoyée à la V-Int r + 1 porte, pour les sprites 0 à 6 (Simon, fouet) et les ennemis, les entrées construites au tour précédent (`swap_entries`), avec les tuiles du bouclier et la couleur de silhouette (PAL3 1-3) qu'elles demandent, chargées par la même file DMA. Les sprites qui sont du bitmap sur le MSX (bougies, objets, HUD, flammes des torches) restent à une image. `vk_actors_flush` (appelé par `hide_sprites` de `main.c` et par `vk_load_whip_graphics`, à chaque chargement de scène) vide le tampon : après le gel d'un changement de salle, Simon et les ennemis réapparaissent deux images après la première passe, comme quand le MSX vide sa SAT (`47F0`, `63C5`). Coût : 37 entrées de 8 octets copiées par image, aucune image de plus pour la manette au-delà de celle du MSX (+2, contre +2 ou +3 sur le MSX).

Mesures du port après correction (`tools/measure_md_frame_timing.py`) : fouet et marche, 1re image changée à r0 + 2 (avant : r0 + 1) ; son du fouet à la V-Int r0 + 1 ; cellules de la torche à r, flamme à r + 1 ; barre du HUD au contact à r + 1 ; fond de l'objet 05 à la passe + 4 en RAM, image + 1. Scénario `w_simon_blink_phase` : silhouette affichée aux images +4 à +301 (passe paire) et +6 à +299 (impaire) depuis le début de la passe du ramassage, exactement les images de la trace MSX (`castle-candles.md` 9.8 ; avant : une image plus tôt).

## 4. Écarts restants (non corrigés)

- **Tuiles sous le HUD** : le port les écrit au CPU pendant l'itération, avant le faisceau : elles changent à r, le MSX à p + 1. Ex. : le support d'une torche ou un bloc cassé paraît une image trop tôt par rapport à la pose du fouet (MSX : une image avant la pose ; port : deux). Correction possible : passer ces écritures (`room_view.c` `set_courtyard_flame_background`, `candle_view.c` `draw_hidden_block` et les bougies de la salle 0) par la file DMA. Non fait : `candle_view.c` est en cours de modification par ailleurs.
- **Sons** : le port envoie tout son à r + 1 ; le MSX à p + 2 quand la demande tombe dans la 2e image de la passe (coup 0C `7FD8`, ramassage `8D3E`, 1B de 05). Le son du coup part donc une image avant la pose du contact (MSX : ensemble). Écart d'une image (17 ms), non corrigé.
- **Lecture de la manette** : le MSX ne lit les touches qu'au début d'une passe (une image sur deux) ; le port à chaque image et lance le fouet ou la marche à n'importe quelle image. Retard touche → écran : MSX +2 ou +3, port +2.
- **Contact** : le MSX baisse l'énergie (`4638`) une passe avant de poser `C42D` et le son 13h (`6F9F`) ; le port fait les trois dans la même itération. Logique de jeu, hors de ce calendrier.
- **Bordure** : le MSX change R#7 au milieu de l'image p (vers la ligne 80) : la première image est coupée en deux. Le port change tout le fond à r + 1.
- **Ligne de partage** des écritures VRAM du MSX calculée (50 lignes avant la ligne 0, 63,7 µs par ligne), non vérifiée au pixel (pas de capture d'écran sans rendu OpenGL).
