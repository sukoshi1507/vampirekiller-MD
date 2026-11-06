# Hasard : le registre R du Z80

Code : `sgdk/src/enemies_stage7.inc` (`s7Random`, `Enemies_s7R`, `Enemies_randomR`), appels dans `enemies_stage7.inc` (puce) et `enemies.c` (os).
Test natif : `tests/test_stage7.c` (`random_r`). Trace : journal de chaque `LD A,R` (point d'arrêt sur l'instruction suivante : frame, PC, A, R, IX,
stage, salle), harnais openMSX de `stage4/tool` + un script de points d'arrêt non versionné (`scratchpad/stageR`).

## Recensement (`ED 5F` dans la ROM : 15 occurrences)

| Adresse | Usage | Utilisation de A | Tracé | Porté |
|---|---|---|---|---|
| `B216` | init de la puce 0D | +12 = 3Ch - (A >> 3) (A entier, bit 7 compris) | stages 7 et 9 : 20 appels | `enemies_stage7.inc` |
| `B29E` | saut de la puce | A & 3 = 0 : vy `F8E0` au lieu de `FDA0`, si Y >= 50h | stages 7 et 9 : 776 | oui |
| `B4AD` | lancer d'os de l'objet 0B | bit 0 : vy `F800` ou `FB00` | stages 7, 8, 9 : 496 | `enemies.c` |
| `B9B6` | objet 14h (boss stage 9) | table `B9C7`[A & 7] | stage 9 : 137, puis 230 (salle 7, 13 760 images) | `boss.c`, `Enemies_randomR` |
| `BF32` | boule du boss du stage 3 | 40h - (A >> 2) | stage 3 : 2, puis 60 (salle 5, 22 000 images, Simon maintenu en vie) | `boss.c`, `Enemies_randomR` |
| `92E8` | choix aléatoire du marchand / objet mural (paire 3-5, 4-5 ou 3-4 de la table `9300`) | bit 0 (RRA) : 1 = premier de la paire (H), 0 = second (L) | stage 2 salle 0 (voir plus bas) : 137 ; naturel : 10 (stage 1 et stage 4, salle 8) | `wall.c`, `Enemies_randomR` |
| `6854`, `6860` (routine `684F`) | gerbe de 6 particules (objet 2Eh) à la mort du boss du stage 18 (script de salle `65B0`, `CE00` = 6) | X = `CE0F` + (A & F) - 8 ; Y = 98h + (A & F) | stage 18 salle 9 : 12 appels par mort | non porté (stage 18) |
| `9AE5` | tirage du drop d'un ennemi mort (`9AC0`) | A & 3F = 0 : drop 2 ; sinon A & 3 = 0 : la flamme porte `+1F` = 1 | stages 1 à 9 : 208 ; stages 10 à 14 : 42 | tirage de la flamme de mort (`enemies_death.inc`, autre chantier) |
| `AA81` (routine `AA7D`) | minuterie de l'objet 0Eh (`A9B7`) : `+18` = 10h + (A & F) | A & F | 1 appel mal attribué au stage 10 : l'objet 0Eh est créé au stage 11 salle 5 (trace `S11`, `stage10-enemies.md`) | non porté (stage 11) |
| `BBA8` (routine `BB8F`, objet 18h `BAF6`) | lancer : vy = (A or 80h) x 2 + `F880` ou `FA80` | A or 80h : 0 à 7Fh uniforme, bit 7 forcé | non atteint | non porté (stages 10+) |
| 4 octets `ED 5F` aux offsets fichier `1D5E1`, `1D634`, `1D651`, `1D78A` | **pas du code** : banque 14 = pilote sonore, fenêtre `8000` (adresses `95E1`, `9634`, `9651`, `978A`), dans les flux de la musique 80 (`9598`-`9669`) et 81 (`976D`-`97DA`) | - | non exécutables | - |

Les 15 occurrences : 2 (`6854`, `6860`) + `92E8` + `9AE5` + `AA81` + `B216` + `B29E` + `B4AD` + `B9B6` + `BBA8` + `BF32` + 4 en banque 14 (données).
Les 11 sites de code ont tous un point d'arrêt (vérifié : les deux octets avant l'adresse tracée sont `ED 5F`, car une banque autre que celle du code peut être mappée
à la même adresse CPU pendant les chargements : sans cette garde, `BBAA` donnait 61 faux appels à l'image 763). Seul `BBA8` n'a jamais été atteint.

## Distribution mesurée

Plans : chaque salle de chaque stage 1 à 9 forcée (`D000`, `D002`, `D001`, `C000` = 4), Simon immobile, 700 à 1500 images ; puis salles à squelette,
puce et boss 6000 images.
- Bit 7 de A : **jamais** positif (1 565 appels) : A < 80h. Le `LD R,A` de la ROM ne le positionne pas. Le port ne l'a donc jamais.
- Les 7 autres bits sont uniformes (histogrammes par tranche de 16 valeurs entre 12 et 24 % par case pour 776 appels ; B2A0 : A & 3 = 0 dans 204 appels sur 776
  = 26,3 %, attendu 25 % ; B4AD : bit 0 à 1 dans 268 appels sur 496 = 54 %, écart non significatif ; B9B8 : table `B9C7` toutes les 8 entrées vues).
- On ne peut pas reproduire la suite exacte (R compte les instructions exécutées, dépendantes de tout ce qui tourne dans la passe). On vérifie donc la
  distribution, pas la séquence.

## Ancien générateur du port

`s7seed * 5 + 0x3B` sur 8 bits, sorti `s ^ (s >> 3)` : période de 256 au plus, bit 0 alterné quasi régulier, bit 7 parfois à 1. Le bit 7 était masqué
pour la puce (`& 0F`) et sans effet ailleurs, mais les bits 0 et 1 (os et saut) n'étaient pas uniformes.

## Port

`s7Random` : LCG 16 bits de période pleine (`x * 25173 + 13849`), on retourne les bits 8 à 14. Sur une période (65536 tirages) chaque valeur de 0 à 7Fh sort
exactement 512 fois (test natif : valeurs, A & 3 = 0 pour 25 %, bit 0 pour 50 %, A >> 3 uniforme sur 16). Un seul flux pour tous les usages, comme le registre R unique.
`Enemies_s7R` (0 à 255 en test, -1 pour le générateur) reste le point d'entrée des tests et scénarios.

## Sites repris (boss, marchand), tracés

Outil : journal de chaque `LD A,R` par point d'arrêt sur l'instruction suivante (`scratchpad/hasardR`, non versionné) ; forçage du stage et de la salle par `poke`
de `D000`, `D001` puis `C000` = 4, Simon maintenu en vie (`C415` = 80h toutes les 100 images).

| Site | Appels | Résultat |
|---|---|---|
| `B9B6` (stage 9, salle 7, 13 760 images) | 230 | A < 80h (bit 7 jamais) ; A & 7 = 0 à 7 : 42, 26, 22, 34, 26, 25, 27, 28 (khi2 = 9,8 pour 7 degrés de liberté, seuil 14,1 à 5 %) |
| `BF32` (stage 3, salle 5, 22 000 images) | 60 | A < 80h (maximum 125) ; A >> 4 = 0 à 7 : 9, 2, 9, 3, 8, 8, 9, 12 ; moyenne 70,9 (attendu 63,5, écart 1,5 sigma) |
| `92E8` (stage 2 salle 0, objet mural direct de (160, 32) ; la ligne `+5` et le compteur `DE41` posés par `poke` avant chaque coup `+3` = 1 pour atteindre les résultats 7, 8, 9 de la table `9300`) | 137 (+ 10 en jeu normal) | A < 80h ; bit 0 à 1 : 75 sur 137 (55 %, attendu 50 %, 0,8 sigma) |
| `92E8`, résultat `C70C` relu en `92FF` | 121 | bit 0 = 1 : 3 (paire 3-5 ou 3-4) ou 4 (paire 4-5) ; bit 0 = 0 : 5 (3-5, 4-5) ou 4 (3-4) : **0 exception**, le premier de la paire (H) sort quand le bit 0 est à 1 |

Le port tire donc ces trois sites de `Enemies_randomR()` (un seul flux pour tous les usages, comme le registre R unique) :
- `boss.c` `rand_wait` (`B9B6`) : `table[R & 7]` ; `BF32` : `0x40 - (R >> 2)` ;
- `wall.c` `pick_result` (`92E8`) : `R & 1` ? premier : second de la paire. Les anciens substituts (`vk_boss_R`, `rng`/`tick` du boss, compteur d'images `rnd` du marchand)
  sont supprimés. Le champ `rnd` de `VKWall` est remplacé par un octet `pad` : sans lui, `shop` et `slot` passent à une adresse impaire et le 68000
  plante (le compilateur lit `slot & 1` par un `andl` à l'adresse `slot - 3`) ; les scénarios adressent aussi `vk_walls+320`.
- Tests : `Enemies_s7R` force la valeur (`tests/test_stage9.c`, 0 à 255 pour les 8 cas de la table ; `tests/test_boss.c`, R = 7Ch donne 21h passes ; `tests/test_wall.c`,
  R = 55h donne 3 et 2Ah donne 4), flux libre : 65 536 tirages donnent chaque entrée de la table exactement 8 192 fois, 64 coups du marchand donnent les deux résultats.
  `tests/run_native.sh` lie `enemies.c` avec `test_boss`, `test_stage9` et `test_wall`.

## Sites hors périmètre (stages 10 et suivants, ou drop)

Stages 10 à 18 forcés (toutes les salles 0 à 9, 2 500 images, Simon à l'entrée, fouet répété, boss ramené à `C418` = 4) puis stages 1 à 9 (salles 0 à 8, 4 000 images, même plan) :

- **`9AE5`** (`9AC0`, drop d'un ennemi mort sans `+1F`, `CE00` = 0 ; `CF40` compte les morts, tous les 32 : drop 13h) : 250 appels (208 en stages 1 à 9, 42 en stages 10 à 14), A < 80h ;
  A & 3F = 0 : 2 fois (attendu 3,9) ; sur les autres, A & 3 = 0 : 64 sur 248 (25,8 %, attendu 24,6 %). C'est la distribution de `Enemies_randomR()` ; l'objet laissé par la flamme
  de mort est le chantier de `enemies_death.inc`.
- **`6854` et `6860`** (boss du stage 18, `65B0`, `CE00` = 6 ; objet 11h créé par `65D3`, mort : `65F3` crée 6 objets 2Eh) : 12 appels par mort (13 morts tracées, `R_X` 190 à 216). Le second appel suit le
  premier de 8 incréments de R exactement (instructions `AND`, `SUB`, `LD`, `ADD` entre les deux) : (A2 - A1) mod 128 = 8 dans les 78 paires (R n'a que 7 bits qui comptent) : les nibbles sont couplés,
  (A2 & F) = ((A1 + 8) & F), donc Y - 98h = (X - CE0F + 16) & F. Premiers nibbles : bit 7 jamais ; 6 particules par mort ne suffisent pas à mesurer l'uniformité (les 14 morts partent du même état, les suites se recoupent).
- **`AA81`** (`AA7D`, objet 0Eh `A9B7`, états 0 à 2 : minuterie `+18` = 10h + (A & F)) : 1 appel en 15 000 images (stage 10 salle 0, A = 110) ; pas de distribution mesurable.
- **`BBA8`** (objet 18h `BAF6`, `BB8F`) : jamais atteint (stages 10 à 18, 90 salles, 2 500 images ; le boss du stage 15, script `BBF3`, crée l'objet 16h `BC6D`, sans appel en 9 000 images).
  Sans valeur tracée, rien n'est porté. D'après le code, A or 80h : le bit 7 est forcé, la valeur 80h à FFh est uniforme si A l'est.
- Les quatre `ED 5F` de la banque 14 sont des octets de données musicales (pas de point d'arrêt possible : jamais exécutés).
