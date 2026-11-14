# Routes des stages 10 et 11 : fiche testeur

Etat : les deux routes sont jouees **de l'entree a la porte de sortie**, a la manette, sans aucune pose de position, sur la ROM MD (harnais `tools/md_harness.py`) :
`tests/md_scenarios/zzq_stage10_route.txt` (stage 10, salles 0-3-4-5-0-5-8, porte, entree du stage 11) et `zzq_stage11_route.txt` (stage 11, salles 0 a 5, porte, entree du stage 12).
Les deux scenarios entrent par le vrai selecteur de stage (pause + Konami + B + A). **Ils sont joues en mode invulnerable** (`Enemies_god` = 1 apres le Konami) : un `tap A` de plus
le coupe (`zzm_enter10.inc`), mais la salle 0 du stage 10 (chauve-souris, une passe toutes les ~160 images) et la salle 0 du stage 11 (2 ennemis lents) touchent Simon pendant les marches
chronometrees et le repoussent hors de la route : les variantes sans invulnerabilite sont decrites plus bas.

## Les deux « blocages » des premieres notes : verdict

Les deux murs sont des **blocs de briques 4x4 de bougie (mode 3)** que le jeu d'origine fait casser au fouet. Ce n'etait pas un ecart du portage (aucun code C modifie).
Preuves sur openMSX (ROM d'origine, entrees reelles, Simon pose dans la salle par `D000`, `D002` = 3, `D001`, `C000` = 4 puis aucune pose de position ; position lue en `C427`/`C425`, mode en `C420`, carte en `D100`) :

- **Stage 10, salle 3** (bloc cache `17` en (96, 112), bougie de mode 3, voir `stage10-level.md` ; la carte ROM `vk_stage10_collision[3]` ne le contient pas, le jeu l'ecrit dans la carte RAM `D100` au chargement
  de la salle) : sans coup de fouet, Simon est porte par la plateforme jusqu'a **X 88** (images 887 a 897), puis la plateforme continue sans lui : mode 4 (chute) a l'image 907, Y 168 puis 250 (eau) comme sur le MD.
  Avec `S` (fouet) maintenu 20 images quand Simon est en X 86-88 sur la plateforme : les 4x4 tuiles du bloc (rangees 12 a 15, colonnes 12 a 15 de `D100`, tuiles `01 02 / 0A 0B`) sont **remplacees par le decor d'origine** (`2F 2C 2F 2A ...`),
  Simon passe (X 90, 92, ... 114 avec la plateforme, Y 144 constant). Ce n'etait donc ni un reste perime en RAM ni une mauvaise table : c'est un vrai mur cassable. Sur le MD (`zzq_stage10_route.txt`) : `hold B 3` a X 86, meme resultat.
- **Stage 11, salle 1** (bloc (96, 80), bougie `01 50 60 00 03 18`, cle blanche `84 50 60 00 18`) : depuis le gradin (Y 144) Simon marche jusqu'a **X 88** et s'arrete (images 941 a 1019+). Sauter (UP) en X 80 a 88 monte a Y 112
  (sommet = 32 px) mais **ne se deplace pas** en X (mur) puis retombe : identique au MD. Le saut *avec coup de fouet au sommet* (UP 3 images en X 88, puis `S` 12 images apres, Simon en Y ~115 au moment du coup) casse le bloc
  (rangees 8 a 11 de `D100` redevenues decor) et Simon, encore en l'air, passe en X 92 et se pose en **Y 112** sur le palier. Sur le MD : `C+DROITE` 1 image, `DROITE` 16 images, `B` 3 images (fenetre : `DROITE` 12, 16 ou 20 images avant le coup reussissent, 8 et 24 ratent). Le coup de fouet debout ou a la volee depuis X 88 sur le sol ne touche pas le bloc (Y 144), et le test `zzn_stage11` qui pose Simon en (92, 112) contournait cette difficulte.
- L'hypothese « meme endroit dans deux salles » : les deux murs sont de la meme famille (bloc de bougie mode 3 devant lequel Simon se bloque), pas une table perimee d'une autre salle.

Le fouet est donc **le moyen de passage** de ces deux stages (blocs 4x4 en travers du chemin). Les memes blocs reviennent aux salles 3 de la stage 11 et peut-etre ailleurs (voir les listes de `candles_md.c`).

## Stage 10 (9 salles : 0-1-2-3-4-5 en cercle, 1 monte vers 6, 5 monte vers 8, 6 et 7 se bouclent)

Route complete (Simon arrive en (32, 144), tourne a droite). X = coordonnee de Simon, `P` = X de la plateforme mobile de la salle (lue dans `vk_platforms+2`). Les durees sont des images apres l'arrivee dans la salle.
Entree dans le stage avec les vies/objets du selecteur ; le scenario commence par la cle absente (items 0).

1. **Salle 0** : marcher a droite jusqu'a X 96, attendre la plateforme (X 97, va et vient entre 96 et 191), monter dessus, ne plus bouger, descendre a droite quand elle est a son maximum (X 180-190), marcher a droite : salle 1.
2. **Salle 1** : sol a gauche jusqu'a X 38 : sauter au bord (C+DROITE depuis X 32-34) ; marcher jusqu'au bloc (X 120), sauter dessus (arrivee Y 112, X 124) ; au bout du bloc (X 152) sauter a droite sur le rebord haut (Y 80, X 156) ; marcher a droite, sauter au bord (X 226) : salle 2.
   (La cle doree (144, 96) se ramasse en route ; coffres : voir plus bas.)
3. **Salle 2** : attendre 390 images ; sauter sur la plateforme (P ~75, arrivee Y 168), avancer de 12 px, puis **s'accroupir (BAS) 263 images** pour passer sous la stalactite ; quand la plateforme est a son maximum (P ~190), se relever et sauter a droite : rebord droit (Y 144, X ~234), marcher : salle 3.
4. **Salle 3** : attendre 606 images (la plateforme doit passer par X ~35 en venant de la droite), marcher a droite 40 images pour monter dessus (Simon X 44-46). La plateforme porte Simon jusqu'a X 86 : **a X 86, appuyer sur le fouet (B), 3 images** : le bloc (96, 112) se casse,
   Simon continue sur la plateforme sans s'arreter (X 100, 105 ...). Quand la plateforme est a son maximum (P 185, Simon X ~192), marcher a droite : salle 4 (10, 144).
5. **Salle 4** : marcher a droite jusqu'au bout du rebord (X 122), attendre 90 images, **sauter sur la plateforme** (P ~149 en revenant vers la gauche ; arrivee Y 168, X ~169), se **tenir accroupi (BAS)** : la stalactite (X 176) arrete Simon debout ; accroupi la plateforme l'emporte (X 188).
   Quand P >= 189 (Simon X ~216), (rester accroupi 4 images,) sauter a droite : rebord droit, salle 5 (10, 144). (Le coffre (64, 128) de la salle 4 est sur le rebord de gauche.)
6. **Salle 5** : sauter le trou au bord (X 30-32) : sol X 64-127 (Y 144) ; marcher jusqu'a X 120, sauter sur le bloc flottant (Y 112, X 152) ; de la, sauter a droite sur le rebord haut (Y 80, X 194) ; marcher a droite jusqu'au bord de salle : **salle 0 par le haut** (arrivee X ~24, Y 80, rebord haut gauche).
7. **Salle 0 (haut)** : marcher a droite jusqu'a X ~48-64 : la **cle blanche** (48, 48) est prise (items bit 0). Revenir a gauche : salle 5 en haut (X 246, Y 80).
8. **Salle 5 (haut)** : marcher a gauche jusqu'a X 186, **HAUT** au pied de l'escalier (tuile 13, en haut a gauche) : montee vers la **salle 8** (arrivee X 175, Y 224), garder HAUT jusqu'au sol (Y 176, X 127).
9. **Salle 8** : marcher a droite : la porte (236, 128) s'ouvre quand Simon est contre (X 228), **cle consommee** (items 0), puis ~168 images plus tard : **stage 11, salle 0 en (16, 176)**.

Non joue : salles 6 et 7 (en haut de la salle 1 ; elles se bouclent), coffres 1A (salle 0 (224, 128)), 11 (salle 4 (64, 128)), 1C et 13 (salle 5 (96, 64) et (112, 128)), cle doree de la salle 5 (224, 128), bloc cassable (32, 48) de la salle 1 et (32, 144) de la salle 8, ennemis (aucun n'a ete combattu).

## Stage 11 (6 salles en ligne : 0-1-2-3-4-5, porte salle 5 en (236, 128))

Salle 1 : cle blanche cachee dans le bloc (96, 80) ; cle doree (128, 64) sur le dessus du bloc (128, 112) (non prise par le scenario). Route complete (Simon (16, 176) a l'entree) :

1. **Salle 0** : marcher a droite 532 images (2 ennemis lents), sauter (C+DROITE 1 image) et marcher 22 images : salle 1 (X ~10, Y 176), arrivee sur le premier gradin (Y 144, X >= 56).
2. **Salle 1** : marcher a droite jusqu'au pied du mur (X 88, Y 144) ; **sauter (C+DROITE 1 image), garder DROITE 16 images, puis fouet (B, 3 images)** : le bloc se casse (la cle blanche tombe et est prise : items bit 0), Simon se pose en (92, 112). Marcher a droite jusqu'au
   bloc (128, 112) (X 120), sauter dessus (sommet Y 80, la cle doree est la), marcher a droite et descendre : salle 2 (X 22, Y 112).
3. **Salle 2** : marcher a droite (descente jusqu'au Y 144), sauter depuis X >= 84 sur le palier (Y 112, X ~130), descendre au sol (Y 176) et marcher a droite : salle 3 (X 56, Y 176).
4. **Salle 3** : meme mur qu'en salle 1 : sauter la marche (X 58, Y 144), marcher jusqu'a X 88, **saut + DROITE 16 images + fouet** (arrivee (92, 112)), puis marcher a droite (X 120, 146 (Y 80), 184 (Y 144), 210 (Y 112)) ; sauts en avant (C+DROITE) pour passer les blocs : salle 4.
5. **Salle 4** : arrivee sur le rebord haut (X 14, Y 80) ; enchainer sauts et marches vers la droite (sol Y 176 en bas, X 78 puis 142 (Y 112) puis 206) : salle 5 (10, 144).
6. **Salle 5** : marcher a droite jusqu'a la porte (X 228, Y 176) : la porte s'ouvre, cle blanche consommee (items 0), **stage 12 salle 0** ~170 images apres.

Le scenario (`zzq_stage11_route.txt`) fait les salles 3 a 5 par une serie de 10 sauts en avant (C+DROITE 1 image, DROITE 45 images, DROITE 20 images entre deux sauts) apres le coup de fouet de la salle 3 : les positions de chaque saut sont dans le scenario
(les attentes `until game.player.x >= 88` etc. sont la reference). Un humain peut faire plus simple (sauter a chaque marche), mais cette serie est reproductible.

## Sans invulnerabilite (scenarios `zzq_stage10_route_nogod.txt` et `zzq_stage11_route_nogod.txt`)

Le portage est joue a la manette avec `Enemies_god` = 0 (`zzm_enter10/11.inc` coupent le mode invulnerable par un `tap A` de plus apres le Konami). Les attentes chronometrees sont remplacees par des
attentes conditionnelles qui fouettent ce qui s'approche (`zzq_guard.inc`, `zzq_walk.inc`, `zzq_walkl.inc`, `zzq_clear.inc`, `zzq_guard_platform.inc`, `zzq_guard_pmax.inc`). Chaque scenario lance 3 fois
donne exactement la meme trace image par image (les ennemis ne dependent que de la minuterie de la salle et des positions de Simon). Energie maximale = 32 ; contact avec un homme-poisson ou une puce : -4 / -2 selon l'ennemi.

**Stage 10 : de l'entree a la porte, vivant, 31 / 32 d'energie** (un seul point perdu, une boule de feu en salle 8). Conseils pour un humain :
- Salle 0 (ennemi volant, type 2, une passe toutes les ~160 images, alternativement de droite et de gauche) : le fouet porte ~31 px devant Simon et ne touche l'ennemi qu'au milieu de sa vague (y <= ~122) :
  l'attendre au bord du sol (X 98), le fouetter quand il est a ~35 px. Celui qui vient de derriere ne se fouette pas depuis le sol (il est trop bas) : sauter sur la plateforme quand elle est a X <= 166 (saut a droite en gardant DROITE),
  puis se retourner et le fouetter depuis la plateforme (il est alors haut). Accroupi ne protege pas. Un coup recu au bord du sol par derriere jette Simon vers la plateforme (il peut tomber a l'eau si elle n'est pas la).
- Salles 1 a 5 : hommes-poissons (type 17, **2 coups de fouet**). Ils sautent de l'eau, marchent sur les rebords hauts et **tombent aux memes endroits** (salle 1 : X 108 et X 157 ; salle 3 : X 85 ; salle 5 : X 125 et X 157) :
  ne pas s'arreter sous ces points. Attendre a ~20 px d'un point de chute, fouetter ceux qui viennent a pied. Ne sauter sur un bloc (salles 1 et 5) que quand la salle est vide d'hommes-poissons
  (le scenario attend `aucun type 17 vivant`, puis monte sur le bloc et y attend la vague suivante avant de sauter plus haut). Les sorties d'eau reviennent toutes les ~200 images.
- Salle 3 : marcher vers l'avant sur la plateforme (jusqu'a ~18 px de son bord gauche) avant de casser le bloc, sinon l'homme-poisson qui tombe en X 85 retombe sur Simon arrete au bloc.
- Salle 4 : ne pas attendre au bout du rebord (X 122) quand l'homme-poisson sort de l'eau en X 112/128 ; sauter sur la plateforme quand elle descend (X ~150) plutot que quand elle remonte.
- Salle 8 : deux hommes-poissons fixes au sol (X ~51 et ~205) crachent des boules de feu (-1) a hauteur de la poitrine ; les fouetter (elles disparaissent) et tuer les deux hommes-poissons qui s'avancent avant de marcher vers la porte.

**Stage 11 : salles 0 a 2 passees sans aucun degat (32 / 32 a l'arrivee en salle 3). Salles 3 a 5 : NON passees sans degat.** Conseils verifies :
- Salle 0 : le premier ennemi (X 229) se fouette de face ; le second (de gauche) est plus lent que Simon : continuer a marcher.
- Salle 1 : saut + fouet casse le mur ; **ne pas rester immobile apres le saut** : l'oiseau/puce posee en (144, 80) plonge sur l'endroit ou Simon se pose (-2). Enchainer en marchant vers le bloc (128, 112).
- Salles 2 a 4 : toutes les ~200 images (tant que Simon est en X < 192) un aigle entre en (E0, 30/40), vole vers la gauche a y 48-64 et porte une puce (type 14, 1 point de vie) qui **plonge en spirale sur la position de Simon** quand l'aigle est a moins de 60 px.
  La puce ne se fouette pas pendant la descente (elle arrive en diagonale, trop haut, puis au contact) ; elle se fouette quand elle a atterri et saute vers Simon. L'aigle (8 points de vie) fait -4 s'il touche Simon sur un rebord haut (bloc y 80 de la salle 3).
  Il y a aussi une puce posee en (112, 112) dans la salle 2 (palier) qui bondit des que Simon est a moins de 60 px. En salle 2 attendre a gauche, fouetter les deux puces, puis traverser ~90 images plus tard est sans degat.
- Salles 3 et 4 : aucun des rythmes essayes (attente 0 a 190 images avant de traverser, par pas de 10, puis un joueur pilote par la position) n'evite la plongee d'une puce ; chaque coup recu repousse Simon en arriere et la
  vague suivante arrive avant qu'il ait refait le chemin. Non resolu : la route complete du stage 11 sans invulnerabilite reste a faire (voir "suspect" ci-dessous).

**Points suspects** (le point (1) a depuis ete compare au MSX : conforme, voir la section suivante ; (2) et (3) non compares) : (1) en stage 11 la puce suit Simon en plongee et ne peut pas etre fouettee a temps depuis le sol : degat quasi inevitable a chaque aigle sur la route (si le MSX permet de l'eviter, voir enemies_stage7.inc `fleaStep` : etat 0, |X Simon - X puce| < 3C) ;
(2) stage 10 salle 0, l'ennemi volant arrivant de derriere ne peut pas etre fouette depuis le sol (hauteur de la vague) ; (3) au bord du sol de la salle 0, un coup par derriere jette Simon dans le vide si la plateforme n'est pas dessous.
Ces comportements viennent de traductions ligne a ligne du Z80 (boites de contact `romContact`) et n'ont pas ete rejoues sur openMSX.

## Aigle et puce de la salle 3 : MSX contre portage (2026-11-14)

Question : les salles 3 et 4 du stage 11 sans mode invulnerable (une vague d'aigle + puce toutes les ~200 images, la puce plonge sur Simon, l'aigle prend 4 d'energie sur le bloc haut) sont-elles aussi dures sur MSX ?
**Verdict : le portage est conforme, aucun ecart trouve, aucun code modifie.** Ce que l'on subit dans le portage est ce que subit Simon sur le MSX.

Methode. MSX : `tools/run_eagle_flea_trace.py` (`tools/eagle_flea_trace.tcl`, openMSX, ROM d'origine) force D000 = 11, D001 = 3, D002, C000 = 4 comme `species_trace.tcl`, cloue Simon par poke a chaque image
(C427, C425), journalise a chaque VBLANK Simon, C415 (energie), C42D (invulnerabilite), C420, CF0C/CF0D/CF10, R et les blocs C800 (type, etat, Y, X, vitesses, pose, +C, PV).
Portage : meme salle par le menu de pause (3 « salle suivante »), `Enemies_god` remis a 0 (**le menu de pause le met a 1** : c'est pourquoi un essai a la manette via le menu ne montre jamais de degat), Simon cloue par poke a chaque image. Journal image par image des objets et de l'energie.
Les origines d'image different (MSX : premiere image apres le rechargement de la salle ; MD : premiere image jouable) : l'aigle apparait a l'image 64 (MSX) et 71 (MD), un decalage constant de 7 images, on compare donc par rapport a la creation.
Positions : A = Simon (40, 176) au sol, B = (140, 80) sur le bloc haut, C = (175, 64), H = Simon en (140, 80) avec 1000 images, plus un fouet toutes les 10 images.

| Grandeur | MSX (trace) | Portage | Verdict |
| --- | --- | --- | --- |
| Periode des vagues (position A, 2600 images) | creations aux images 64, 264, 464, 664 (puis plus rien : voir plus bas) | 71, 271, 471, 671 | identique, 200 images |
| Cote d'arrivee, X a la creation | droite, X 221 (E0h - 3), puce en (225, 60) | idem | identique |
| Y de l'aigle | 48 puis 64 en alternance (CF0D impair = 48) | 48, 64, 48, 64 | identique |
| Trajectoire de l'aigle, 170 images apres la creation (etat, X, Y) | vol a -2,5 px par passe (1,25 px/image) en Y constant ; arret a X 114 (Simon 40 : ecart < 56), 8 passes, repart | 170 images sur 170 egales (decalage 0) | identique |
| Lacher de la puce | etat 0 -> 1 quand `|X Simon - X puce| < 3Ch` (X puce 98, ecart 58), 2 passes en etat 1 puis etat 3 en plongee a 110 images | etat 1 a 104, etat 2 a 106, etat 3 a 110 (X 98 / 98 / 95, Y 60 / 56 / 53) | identique image pour image |
| Plongee, atterrissage, rebonds | vy +A0 par passe jusqu'a 2047 (8 px/passe), atterrissage etat 2 pendant 2 passes, puis saut vers Simon vx ±220h, vy -260h, ou (1 fois sur 4, si Y >= 50h) -720h au hasard R | meme code (`fleaStep`, 10 920 passes egales a la ROM) | identique ; seul le choix saut bas / haut depend de R (generateur 7 bits uniforme du portage) |
| Boite de contact puce (classe 2) | |dX| < 13, Y Simon - Y puce de -14 a +27 debout (-6..27 en saut, -14..19 accroupi) | `romContact` 8/10h | identique (coup MSX a (50, 173) pour Simon (60, 176), coup MD a (56, 153) : dans les deux cas dans la boite) |
| Boite de contact aigle (classe 4) | |dX| < 17, Y Simon - Y aigle de -22 a +27 | `romContact` 0Ch/18h | identique : a Y 48 il touche des pieds de Simon a Y 26..75, a Y 64 de 42 a 91 |
| Premier coup de la puce, Simon en A | 156 images apres la creation de l'aigle (image 220) | 156 images (227 - 71) | identique |
| Premier coup de l'aigle, Simon en C (175, 64) | 40 images apres la creation (104 - 64), energie 32 -> 28 | 40 images (111 - 71), 32 -> 28 | identique |
| Degats | puce 2, aigle 4 | puce 2, aigle 4 | identique |
| Invulnerabilite apres un coup | C42D = 5Ah au coup, -1 toutes les 2 images : 180 images ; 89 deux images apres le coup | 90 au coup (89 deux images apres), 180 images | identique (une image de decalage de phase) |
| Recul | C420 = 5 ; Simon recule de 1 px/image dans le sens de la vitesse X de l'ennemi (C43C = bit 7 de +A), arc de ~16 px de haut ; (40, 176) -> (18, 160) en 24 images | (40, 176) -> (18, 160) en 24 images, meme arc image par image | identique (sens de la puce : gauche si elle va vers la gauche) |
| Hits d'un Simon immobile en A (energie remise a 32 sous 8) | 13 coups de puce en 2600 images, espaces de 180 a 208 images (des que l'invulnerabilite finit) | 13 coups, espaces de 180 a 196 images | identique |
| Simon en A, fouet toutes les 10 images, 1200 images | 5 coups aux images 221, 418, 622, 818, 1022 | 227, 423, 627, 823, 1027 (+5 / +6 constants) | identique |
| Fouet sur la puce en vol | tuee en etat 3 (X Simon + 11 a + 32, Y 164 a 174, pour Simon en (40, 176)), devient une flamme (objet 1Eh) | tuee en etat 3 aux memes endroits (X +11 a +32, Y 164 a 172) | identique |

Boite du fouet sur la puce (classe 2, `research/enemy-boxes.md`) : pointe a X Simon ± 20, la puce doit etre a 1..39 px devant Simon et, debout, a 2..21 px au-dessus de ses pieds (Y puce de Y Simon - 21 a Y Simon - 2).
Pendant la plongee la puce traverse cette fenetre de Y en 2 ou 3 passes ; elle y reste plus longtemps posee (etat 2, 2 passes) ou en rebond bas. Elle se fouette donc **en vol comme au sol**, sur MSX comme sur le portage : ce qui donne l'impression contraire est le timing, pas une regle.
L'aigle a 8 PV (boite 0Ch x 18h) ; le fouet sur l'aigle n'a **pas** ete mesure (la boite de fouet de sa classe n'est pas dans `research/enemy-boxes.md`).

Autres faits mesures, identiques des deux cotes :
- l'aigle ne descend jamais (Y constant 48 ou 64 jusqu'a la sortie en X 9) : au sol (Y 144 ou 176) et sur la marche Y 112 il ne touche jamais Simon ; seul le bloc haut (Y 80) est expose, et seulement aux vagues paires (Y 64) ; les vagues impaires (Y 48) ne le touchent pas, sauf un saut depuis le bloc ;
- les vagues recommencent a 200 images ; `9CA9` remet les compteurs a la charge de la salle : la premiere vague arrive ~64 images apres l'entree (aigle a Y 48), la deuxieme (Y 64) a ~264 ;
- **aucune nouvelle vague tant que Simon est en X >= C0h** au moment de la creation (CF0C = 1, nouvel essai 8 images plus tard) : de la, le mur de la salle 3 franchi, on n'a plus d'aigle neuf ;
- l'aigle et la puce sont independants apres la creation : la puce plonge des que Simon est a moins de 3Ch (60 px) d'elle, vivant ou non l'aigle (code `fleaStep`, verifie par vecteurs ; **tuer l'aigle d'abord n'est pas trace**) ;
- les puces non tuees restent (jusqu'a 4 en position A apres 4 vagues) et sautent vers Simon sans fin ; quand les blocs d'objets sont pleins la creation de l'aigle echoue et le canal est repousse (CF0C = 1) : c'est la, et non un blocage, que s'arretent les vagues de la trace A apres l'image 664 ;
- le coup d'une puce peut pousser Simon hors de la salle (position A : X 18 puis 246 par le bord gauche de l'ecran), toujours dans les deux emulateurs.

Comment un humain passe ces salles (deduit de ces mesures, **pas rejoue a la manette sur le MSX**) :
1. Ne pas attendre : une vague arrive toutes les 200 images, chaque coup coute 2 (puce) ou 4 (aigle) puis 180 images d'invulnerabilite, et un Simon immobile en position A a pris un coup de puce par vague dans les deux emulateurs.
2. Fouetter les puces plutot que les esquiver : face au cote d'ou elles viennent, fouet toutes les ~10 images quand une puce est a moins de 40 px devant Simon et a moins de 20 px au-dessus de ses pieds. Une puce morte ne revient pas (200 points). Ne pas laisser les puces s'accumuler (4 puces = plus de vague, mais 4 sauteuses sur Simon).
3. Ne pas rester sur le bloc haut (Y 80) quand l'aigle passe a Y 64 : traverser le bloc pendant la vague a Y 48 (la premiere, ~64 images apres l'entree dans la salle, puis toutes les deux vagues), ou en dehors de la fenetre d'une vague (l'aigle met ~110 images pour aller de X 221 a X 98 (Simon 40) et ~190 pour sortir).
4. Une fois en X >= C0h (salle 3 apres le dernier bloc, salle 4), on ne recoit plus d'aigle neuf ; il ne reste que les puces deja la.

Ce qui reste a verifier (non fait) : un passage complet de la salle 3 puis de la salle 4 a la manette avec ces regles, sur le MSX comme sur le portage.

## Ce qui n'est PAS verifie

- Stage 11 salles 3 a 5 : aucun passage sans invulnerabilite reussi jusqu'a la porte (stage 10 : verifie, voir plus haut). La mecanique aigle + puce de la salle 3 est comparee au MSX dans la section precedente (conforme) ; les autres ennemis ne l'ont pas ete.
- Sur MSX : seuls les blocs de la salle 3 du stage 10 et de la salle 1 du stage 11 ont ete rejoues (positions, `D100`). Les salles 2 a 5 du stage 11 et les salles 4, 5, 8 du stage 10 n'ont ete jouees que sur le portage ; la cle blanche (48, 48) de la salle 0 du stage 10 a ete prise sur le MD.
- Le mur de la salle 3 du stage 11 est suppose etre le meme mecanisme (bloc (96, 80) de la liste de bougies) ; verifie sur le portage seulement.
- Les coffres, les salles 6 et 7 du stage 10, la cle doree du stage 11 : non joues.
