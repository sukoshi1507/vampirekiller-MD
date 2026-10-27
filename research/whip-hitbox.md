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
- Fenêtre de collision : la lanière est visible de la phase timer 3 à 10 dans le jeu MD (`timer ≥ 8 : 16 px` à remplacer). Au MSX : sprite visible de t07 à t14, soit 8 ticks ; pour le portage, la fenêtre utile est donc le timer 3 à 10, en gardant le décalage de deux ticks.
- Le décalage de deux ticks est **prouvé** dans les données, mais sa cause ne l'est pas. À confirmer avant de l'appliquer au timer MD.

## Limites

- Une seule trame de référence (a1) analysée en détail. a2 à a4 sont capturées mais non comparées.
- Décodage 4 bpp non validé par image de référence.
- Pas de mesure accroupie, ni gauche.
- La VRAM de `anim-inventory/vram1/vram.bin` (80 trames) est au repos dans la cour, sans attaque. Elle ne sert pas à la lanière.
