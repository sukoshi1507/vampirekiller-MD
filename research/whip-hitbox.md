# Forme de la lanière du fouet (MSX2, openMSX)

Objectif : mesurer la lanière de Simon pendant l'attaque SPACE pour remplacer les segments prototypes de `vk_whip_bounds` (`sgdk/src/whip.c`). Aucun fichier du portage n'a été modifié.

## Méthode reproductible

- ROM : `Vampire Killer (Japan, Europe) (En).zip` à la racine (SHA-256 vérifié par `tools/probe.py`), machine `C-BIOS_MSX2`.
- Script de capture (hors dépôt) : `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/whip/capture_attack.tcl`
- Lancement : `python3 -I anim-inventory/run_trace.py whip/capture_attack.tcl whip/out 47` (dans le dossier scratchpad).
- Sortie : `.../scratchpad/whip/out/` : `a1_base.bin` (VRAM avant attaque), `a{1..4}_t{01..16}.bin` (VRAM complète 128 Ko à chaque tick 0x4028 pendant 16 ticks), `ticks.csv` (C000, C42E, C42F, C427, C425, C436, C420), `regs.csv` (R0 à R27 par tick).
- Décodage SAT et motifs : scripts `whip/render.py`, `whip/bbox2.py`, `whip/cells.py` (scratchpad).
- Trois attaques capturées (a1 à a4 ; a1 analysée en détail, a2 à a4 non comparées).

## Registres VDP (prouvé, `regs.csv`)

| Registre | Valeur | Lecture |
|---|---|---|
| R0 | 6 | modes bitmap (SCREEN 5 présumé) |
| R1 | 98 (0x62) | bit 1 = sprites 16x16 |
| R5 | 239 | SAT à 0xF600 (bits 1-0 ignorés), R11 = 1 ajoute 0x8000 |
| R6 | 31 | motifs de sprites à 0xF800 |
| R11 | 1 | A15 de la SAT |

La SAT est donc en 0xF600 (32 entrées de 4 octets). L'hypothèse 0xF780 (formule brute de R5) est fausse : elle est vide.

## Correspondance des trames (prouvé, `ticks.csv`)

Séquence C42F par tick de capture : 6 (t01-02), 7 (t03-04), 8 (t05-12), 0 (t13-16). Identique à `research/simon-assets/ATTACK_TRACE_OPENMSX.md`.

Écritures VRAM : la VRAM ne change qu'aux ticks impairs (t02, t04, ... valent 0 octet modifié). Le tick impair est donc la mise à jour réelle, et une image dure deux ticks.

Lanière visible dans la SAT (sprites n = 0x10, 0x14, 0x18, 0x1C) : **t07 à t14**. Les sprites ne sont jamais présents en t01 à t06 ni en t15-16.

Donc la lanière apparaît **deux ticks après** le début de la phase C42F = 8 (t05), et reste **deux ticks après** sa fin (t13-14 avec C42F = 0). Décalage constaté : **prouvé** ; cause : **inconnue** (retard de rafraîchissement du côté du jeu, ou lecture de C42F avant mise à jour).

Corps de Simon : x+0 (sprite x=8) pendant t01-02 ; décalé à x-8 (sprite x=0) en t03-06 ; retour à x+0 à partir de t07. Ce décalage est dans le corps, pas dans la lanière. Il suit la phase 7-8 et ne dépend pas de la lanière.

Trames de la lanière (paires de ticks, mise à jour réelle) :
- P4 (t07-08), P5 (t09-10), P6 (t11-12), P7 (t13-14) : mêmes positions de sprites, motifs différents (animation).

## Boîte englobante (relative à Simon : x = C427 = 16, y = C425 = 176)

Position du sprite : `x_sprite - C427`, et `Y+1 - C425` (le SAT affiche la ligne Y+1).

| Sprite | Position SAT | Position relative |
|---|---|---|
| 0x18/0x1C (Y=0x90) | x=0x18 | x +8 .. +23, y -31 .. -16 |
| 0x10/0x14 (Y=0x90) | x=0x28 | x +24 .. +39, y -31 .. -16 |

Boîte mesurée pour P4 à P7 (pixels opaques, décodage 4 bpp quadrants) : **x [+8, +39], y [-31, -16]**. Identique sur les quatre paires.

Corps de Simon pour comparaison : sprites Y=0x90 et 0xA0, x=8, donc x [-8, +7] et y [-31, -15] (haut et bas).

## Forme (déduit, décodage incertain)

Cellules de 8 px contenant des pixels opaques (union P4-P7), coordonnées du coin haut-gauche, relatives à Simon :

`(+8,-32) (+8,-24) (+8,-16) (+24,-32) (+24,-24) (+24,-16) (+32,-32) (+32,-24)`

Répartition par colonne : des pixels de x +8 à +15 (2 à 4 par colonne), puis x +24 à +39 (7 à 15 par colonne). La lanière est donc surtout une masse de +24 à +39, avec une traîne à gauche de +8.

Le décodage des motifs est **déduit** : l'ordre 4 bpp quadrants (UL, LL, UR, LR, blocs de 32 octets) donne une cohérence de voisinage supérieure à l'ordre ligne par ligne sur la plupart des motifs. Aucune image de référence n'a validé cet ordre. Une erreur d'ordre changerait la forme exacte, pas la boîte. Hypothèse à confirmer.

## Symétrie gauche / droite

**Inconnu.** Aucune entrée ne fait marcher Simon ni ne le retourne (voir `research/input-mapping-openmsx.md`). La direction n'a pas pu être changée. Le miroir n'est donc pas mesuré. Le commentaire de `tools/build_simon_attack_md.py` (« mirror only for left-facing ») reste une hypothèse.

## Comparaison avec le portage actuel (`sgdk/src/whip.c`)

- `vk_whip_bounds` (état actuel) : debout, boîte inclusive x+8..x+32 et y-22..y-15, soit la lanière visible mesurée en jeu (`research/whip-hitbox-ingame.md`). Accroupi : estimation, non mesurée. Ce ne sont pas les valeurs de la trace MSX ci-dessous.
- Mesure MSX (cette trace) : boîte y -31 à -16, x +8 à +39 en debout. Elle est plus grande que la lanière visible en jeu et ne sert pas de boîte de collision.
- Le texte de `ATTACK_TRACE_OPENMSX.md` disait « x-8 à x+39 ». La mesure ne montre **aucun pixel à x-8** (le plus à gauche est +8). À corriger dans la note.
- Accroupi : non mesuré (aucun accroupissement capturé).

## Proposition de table pour `whip.c` (non appliquée)

Cellules 8 px relatives à Simon, à utiliser en debout, phases 3 à 14 (voir fenêtre ci-dessous) :

```c
/* Lanière MSX2 mesurée (openMSX, P4-P7). Relatif à Simon (x,y). Cellule 8x8.
   Déduit du décodage SAT ; forme exacte à confirmer par image de référence. */
typedef struct { signed char dx, dy; } VkWhipCell;
static const VkWhipCell vk_whip_cells_standing[] = {
    { 8,-32}, { 8,-24}, { 8,-16},
    {24,-32}, {24,-24}, {24,-16},
    {32,-32}, {32,-24},
};
#define VK_WHIP_BOX_X0  8
#define VK_WHIP_BOX_X1 39
#define VK_WHIP_BOX_Y0 -31
#define VK_WHIP_BOX_Y1 -16
```

Changements nécessaires (historique, partiellement appliqués) :
- `vk_whip_bounds` : la boîte de la trace (x +8 à +39, y-31..-16) n'a pas été reprise. Le portage utilise la lanière mesurée en jeu (x+8..x+32, y-22..y-15, inclusive). Accroupi : non mesuré.
- Fenêtre de collision : la lanière est visible de la phase timer 3 à 10 dans le jeu MD (`timer ≥ 8 : 16 px` à remplacer). Au MSX : sprite visible de t07 à t14, soit 8 ticks ; pour le portage, la fenêtre utile était donc le timer 3 à 10, en gardant le décalage de deux ticks. **Remplacé** (2026-10-29) : le MSX ne teste qu'une passe, celle de `C422` = 3 ; le port teste au timer 11 (section « Fouet en saut »).
- Le décalage de deux ticks est **prouvé** dans les données, mais sa cause ne l'est pas. À confirmer avant de l'appliquer au timer MD.

## Limites

- Une seule trame de référence (a1) analysée en détail. a2 à a4 sont capturées mais non comparées.
- Décodage 4 bpp non validé par image de référence.
- Pas de mesure accroupie, ni gauche.
- La VRAM de `anim-inventory/vram1/vram.bin` (80 trames) est au repos dans la cour, sans attaque. Elle ne sert pas à la lanière.

## Fouet en saut : coordonnées et moment du test (2026-10-29)

Statut : **prouvé** par le code et par trace openMSX (3 exécutions), **corrigé** dans le port.

### Méthode

- Trace : `python3 tools/run_whip_jump_trace.py ROM OUTDIR PLAN` (moteur `tools/vertical_links_trace.tcl` + `tools/whip_jump_trace_extra.tcl`), openMSX 21, `C-BIOS_MSX2`. Par VBLANK : `C003`, `C005`, `C420`, `C422`, `C429`, `C42C`, `C42E`, `C42F`, `C427`, `C425`, le tampon SAT `D600`, la SAT en VRAM et les bougies `C470`. Par événement (µs depuis le VBLANK, PC) : `84D1` (lecture de `C425` par la boîte `84A7`), `84E7` (lecture de `C427` par `84D9`), `7DAD`, `7F49`, `7FB7`, `6560`, écritures de `C425`, `C427`, `C422`, `C42F`. Action de plan `whipat Y up|down` : appui ESPACE au VBLANK qui ouvre une passe, en saut, quand `C425` = Y.
- Plans : `run1` (cour 0 : coup au sol, coup en saut vertical à la montée et à la descente), `run2` (saut à droite et à gauche avec élan, coup accroupi), `run3` (Simon à x 44 sous la torche A, saut vertical, coup lancé à Y 164 en montant puis à Y 159 en descendant). Sorties hors dépôt : `scratchpad/whip-jump/run{1,2,3}/`.

### Code (banque 1 en 6000, banque 2 en 8000)

- Chronologie de `C422` (routine de Simon) : `7133` met 1 quand la passe lit ESPACE ; dans la même passe `721D` pose `C42F` = 6 et `723E` passe `C422` à 2. Passe suivante : `7243` `C42F` = 7, `723E` `C422` = 3. Passe d'après : `7250`/`725D` `C42F` = 8, `C429` = 4, `C422` = 4 ; puis `7262` décompte `C429` sur 4 passes et `726F` remet `C422` à 0.
- Tests du fouet, appelés en `7D80`-`7D86` **après** la routine de Simon dans la même passe, et seulement si `C416` < 2 : `7DAD` (objets `C800`), `7F49` (objets `D700`), `7FB7` (bougies `C470`). Tous commencent par `LD A,(C422) ; CP 3 ; RET NZ`. `C422` = 3 ne dure qu'une passe (posé au milieu de la passe 2, remplacé par 4 avant les tests de la passe 3) : **un seul test par coup**, dans la 2e passe (celle de `C42F` = 7).
- Boîte `84A7` (appelée par `8380`/`827C` pour `C800`, `8450` pour les bougies) : X du fouet = `C427` ± 20 (± 24 si `C416` = 1) selon `C42C` (`84D9`, lecture `84E7`), hauteur `C425 - B` avec `B` = 10 si `C420` = 2 (accroupi), 18 sinon (`84C1`-`84D1`). Les positions sont **lues au moment du test**, rien n'est mémorisé au lancement. Le saut (`C420` = 1) prend `B` = 18.
- Si `C416` ≥ 2 (`7D7E`), `7DA0` remplace les trois tests : pas de test de fouet du tout.

### Trace

| Coup | Passe de lancement (`C422` 1 → 2) | Passe du test (`C422` = 3, `84E7`/`84D1`) | Passe suivante |
|---|---|---|---|
| Sol, cour (run1) | 779 : X 16, Y 176 | 781-782 : `C425` 176, `C427` 16 | 783 : `C422` = 4, aucun test |
| Saut vertical, montée (run1) | 853 : Y 154 | 855-856 : `C425` **150** | 857 : Y 147, aucun test |
| Saut à droite avec élan (run2) | 797 : X 34, Y 145 | 799-800 : `C427` **36**, `C425` **144** | 801 : X 38 |
| Saut à gauche (run2) | 1039 : X 62, Y 147 | 1041-1042 : `C427` 60, `C425` 145, `C42C` 1 | — |
| Accroupi (run2) | 1181 : `C420` 2 | 1183-1184 : `C420` 2 | — |
| Torche A, montée (run3) | 835 : Y 164 | 837-838 : `C425` **159**, `84D1` lu : 159 - 18 - 144 < 0, torche intacte | — |
| Torche A, descente (run3) | 863 : Y 159 | 865-866 : `C425` **164** : torche éteinte (`C470` = 0 à 867) | — |

Dans chaque passe, l'écriture de `C425` par le saut (`6D5D`, ~8 ms) précède le lancement (`7133`) ou le passage à 3 (`723E`), et les tests suivent (~15 ms ou au début de l'image suivante). Le test utilise donc la position de Simon **après le pas de saut de la 2e passe** : en montée elle est plus haute que celle du lancement, en descente plus basse, et X a avancé de 2 px par passe avec l'élan.

**Affichage** : dans `D600`, les sprites de la lanière (motifs 10h-1Ch) sont recalculés à chaque passe à la position de Simon de cette passe : Y = `C425` - 32 (ligne affichée - 31), X = `C427` + 8 et + 24 (run2, images 802 à 808, pose `C42F` 8 : Simon de X 38, Y 144 à X 44, Y 147, lanière toujours à +8/+24 et -31). Ils sont envoyés à la VRAM au début de la passe suivante (`6560`), donc affichés deux images après le calcul, comme le corps (`frame-timing.md`). Rien n'est figé au lancement : ni le corps, ni la lanière, ni la boîte.

### Port : écart et correction

- **Avant** : `vk_whip_tick` (`sgdk/src/whip.c`) figeait `whip.x`/`whip.y` à l'appui (timer 14), et les tests duraient plusieurs images : boîte de lanière (`Enemies_whipHit`, cible) aux timers 10 à 3, bougies et ennemis à boîte ROM aux timers 7 à 3 (« phase 2 », marquée PROVISOIRE). En saut, la boîte restait à la hauteur du lancement pendant que Simon montait ou descendait (mesuré au harnais : `whip.y` 159 constant, Simon de 159 à 144). Conséquence : la torche A de `run3` était éteinte par le coup de la montée et ratée par celui de la descente, l'inverse du MSX. L'affichage, lui, suivait déjà Simon (sprites 5 et 6 dessinés depuis `player`).
- **Après** : `whip.x`/`whip.y` suivent Simon à chaque image (direction et posture restent celles du lancement, `7201`) ; `vk_whip_active` n'est vrai qu'au timer 11 (`VK_WHIP_STRIKE_TIMER`, `whip.h`), seule image de test pour les bougies (`candles.c`), les ennemis (`actors.c`) et la cible (`targets.c`). Timers 12 et 11 = la 2e passe (pose `C42F` 7) ; le 11 est la dernière image de cette passe et porte le pas de saut correspondant quelle que soit la parité de l'appui (le port lit la manette à chaque image, le MSX une sur deux : un appui à une image impaire du saut équivaut à une lecture MSX à la passe suivante).
- Vérification : scénario `tests/md_scenarios/za_whip_jump_y.txt` (le plan `run3` au harnais : coup lancé à 164 en montant, test à 159, torche intacte ; coup lancé à 159 en descendant, test à 164, torche éteinte au timer 11 ; sprites du coup à la position de Simon de l'image précédente). Sur la ROM d'avant la correction, le scénario échoue (boîte à 164, torche éteinte à la montée). Tests natifs : `tests/test_whip.c` (origine qui suit Simon, une seule image de test), `tests/test_candles.c` (les deux coups de `run3`). Scénarios retouchés : `g_room2_panther` (appui quand la panthère est à X 144), `k_stage2_room2_zombie` (appui à X 174), car le test unique arrive 3 images après l'appui au lieu de 7 à 11.

### Incertitudes

- Posture : le MSX relit `C420` au test (10 si accroupi) ; le port garde la posture du lancement (`whip.crouched`). Un changement de posture pendant un coup n'a pas été tracé (accroupi : `C420` reste 2 pendant tout le coup dans `run2`).
- `C416` ≥ 2 (objets 2 à 4 : dague, hache, croix) supprime tout test de fouet sur le MSX (`7D7E`) ; le port branche le niveau 2 de fouet sur `C416` = 2 et le fait toucher. Non corrigé ici.
- Les boîtes elles-mêmes (lanière x+8..x+32 pour `Enemies_whipHit` et la cible, `84A7` pour les bougies et les ennemis ROM) ne sont pas modifiées.
