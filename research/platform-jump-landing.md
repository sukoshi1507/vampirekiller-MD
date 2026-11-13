# Atterrissage d'un saut sur une plateforme mobile : le pixel d'écart d'une image

Point ouvert de `research/stage5-enemies.md` (« le port répartit le +3 de la passe sur deux images (+2 / +1) quand la MSX le fait en une »). Fermé le 2026-11-14.

## Trace MSX (openMSX, `tools/platform_jump_trace.tcl`, stage 10 salle 4, plateforme 1)

Simon posé sur la plateforme (Y 168), saut (touche haut 4 images), retour sur la plateforme. Hook `4028` (une fois par image), adresses : `C420` état, `C425` Y, `C427` X, `C428` phase du saut, `C439` plateforme dessous, `C59A` X plateforme, `C59B` pas.

- Plateforme vers la droite, X de Simon 138, saut vertical avec marche à droite tenue : atterrissage image n = 44 (`C420` 1 -> 0, Y 168, X 166) ; n = 45 : X 166, plateforme 159 ; **n = 46 : X 169** (+3 : marche 2 + transport 1 en une image) ; la plateforme avance aux images impaires (45, 47, ...), Simon aux images paires.
- Plateforme vers la gauche, saut vertical sans touche (case 0, X 198) : atterrissage n = 44 (X 198), plateforme 173 -> 172 en n = 45, **X 197 en n = 46**, puis -1 une image sur deux (48, 50...).

Ordre : à chaque passe de 2 images, `90B8` avance la plateforme à la première image ; la sonde `8524`, le transport `6BAF` et la marche de Simon (`6B39`) se font à la seconde. Le transport n'a donc jamais lieu à l'image du pas de la plateforme, mais à l'image suivante, la même que la marche.

## Cause dans le port

`vk_step_platforms` (`actors.c`) transportait Simon à l'image du pas de la plateforme (`platform_pass`), alors que la marche du joueur tombe sur l'image suivante (horloge `p->clock` paire après le tick, calée à l'atterrissage sur la grille du saut). Résultat : transport +1 en L + 1, marche +2 en L + 2, au lieu de +3 en L + 2 ; sans marche, le transport arrivait en L + 1 au lieu de L + 2 (mesuré avant correction : x = x0 + 1 en L + 1).

## Correction

Le transport est mémorisé au pas de la plateforme (`platform_carry`) et appliqué à l'image de la grille de Simon (`p->clock` impaire avant le tick, donc paire après), le même appel que la marche. Le port suit désormais la trace : atterrissage en L, X fixe en L + 1, transport en L + 2 (et +3 avec marche).

## Vérifications

- Scénario `tests/md_scenarios/zzm_stage10_platform_jump.txt` (6 assertions, échoue avant la correction : transport en L + 1).
- Chutes et sauts sur la plateforme (`tools/platform_fall_trace.tcl` rejoué, comparé par `tools/platform_fall_compare.py`, 132 cas) : 0 écart de pixel après la correction, mêmes décalages de phase qu'avant.
- Limite : sauts avec marche contre un mur ou la pointe de stalactite (X 168, salle 4) : le transport est refusé comme sur la MSX (`7BA9` / `7C05`), le scénario n'utilise donc que le saut vertical.
