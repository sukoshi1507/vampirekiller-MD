# Boîtes de contact des ennemis (stages 7 à 9) : armes lancées, fouet, contact de Simon

Statut : **tracé** (openMSX 22, ROM d'origine). Complète `weapon-targets.md` (bords hauts E 147 / 148) et `stage8-enemies.md`, `stage9-enemies.md`.
Code : `sgdk/src/enemies.c` (`romContact`, `Enemies_whipHitStage4`, `Enemies_shotHit`), `sgdk/src/weapons.c` (`vk_shot_touches`, `vk_shot_touches_slot`),
`sgdk/src/actors.c` (`shot_touch`). Tests : `tests/test_weapons.c` (section « Boxes of the objects of stages 7-9 »), scénario `tests/md_scenarios/zzl_enemy_boxes.txt`.
Boîtes de boss et de bougie : `boss-boxes.md`, `secondary-weapons.md` (même routine `84F0` pour les bougies).

## Méthode : re-exécution de la routine de la ROM

Un point d'arrêt à l'entrée de la routine (`8500` armes, `84A7` fouet, `847A` contact) attend un appel pour un objet réel de `C800` ou `D700` (HL, DE et IX
lus : classe, X, Y). Il sauve les registres et les octets d'entrée, remplace l'entrée (arme : type `+1`, Y `+4`, X `+5` de l'emplacement ; Simon : `C427`, `C425`,
`C420`, `C42C`, `C416`) par chaque valeur d'une grille centrée sur DE, **remet PC à l'entrée** (interruptions coupées pendant le balayage), lit la retenue aux sorties
(`850D` et `8523`, `84B9`, `84C0` et `84D8`, `8487` et `84A6`) et restaure tout à la fin : la ROM calcule, aucune formule du port n'intervient. De 7 000 à
130 000 évaluations par objet et par routine ; tous les rectangles sont pleins. Ensuite, la chaîne complète (`801E` : lancer réel par ESPACE, objet posé par
`poke`, boules et autres objets retirés, appels de `8500` journalisés avec le résultat) vérifie la formule sur 3 975 appels : 0 écart.
Outils non versionnés : `scratchpad/eb/` (`tool/sw.tcl` balayages, `tool/tr.tcl` essais en jeu, `an.py`, `val.py`, plans `p_*.txt`).

## Formules prouvées (identiques au code désassemblé)

D, E = X, Y de l'objet (IX+5, IX+3) ; H, L de la classe. Fenêtres données en différences, Y moins E (ou X moins D).

| Test | Horizontal | Vertical |
| --- | --- | --- |
| Arme lancée, emplacement C450 (`8500`) | `|dX| < L + 6` | `0 <= Y arme - E + H < H + 6` (dague, type 2) ou `H + 12` (hache, croix, eau) |
| Fouet (`84A7`) | pointe = X ± 20 (± 24 chaîne `C416 = 1`) ; `|pointe - D| < L + 12` (`L + 16` chaîne) ; une retenue sur la pointe (bord d'écran) annule | `(Y - B - E + H) & FF < H + 4`, B = 18 debout, 10 accroupi (`C420 = 2`) |
| Contact (`847A`) | `|dX| < L + 5` | `(Y' - 2 - E + H) & FF < H + 1A` debout ; `H + 12` accroupi ; Y' = Y - 8 en saut (`C420 = 1`), `H + 12` aussi |

## Boîtes par classe (4 bords, trace)

Fenêtres inclusives, objet en (D, E). Armes : X arme - D et Y arme - E. Fouet : X Simon - D (cuir, regard à droite `C42C = 0` ; à gauche : symétrique) et Y Simon - E.
Contact : X Simon - D et Y Simon - E.

| Classe (objets) | H, L | Dague gauche / droite / haut / bas | Hache et croix : haut / bas | C460 seul : gauche / droite |
| --- | --- | --- | --- | --- |
| 1 : 0A statue, 0B | 18h, 5 | -10 / +10 / -24 / +5 | -24 / +11 | -16 / +16 |
| 2 : 08, 0C, 0D | 10h, 8 | -13 / +13 / -16 / +5 | -16 / +11 | -19 / +19 |
| boule de la statue (D700 type 3) | 6, 3 | -8 / +8 / -6 / +5 | -6 / +11 | -14 / +14 |
| os de 0B (D700 type 4) | 0Ch, 6 | -11 / +11 / -12 / +5 | -12 / +11 | -17 / +17 |

| Classe | Fouet cuir : pointe - D | Fouet chaîne | Fouet Y - E debout | Fouet Y - E accroupi |
| --- | --- | --- | --- | --- |
| 1 | -16 à +16 (Simon - D : -36..-4) | -20 à +20 | -6..21 | -14..13 |
| 2 | -19 à +19 (-39..-1) | -23 à +23 | 2..21 | -6..13 |
| boule | -14 à +14 | -18 à +18 | 12..21 | 4..13 |
| os | -17 à +17 (-37..-3) | -21 à +21 | 6..21 | -2..13 |

| Classe | Contact : Simon - D | Y - E debout | Y - E en saut | Y - E accroupi |
| --- | --- | --- | --- | --- |
| 1 | -9..9 | -22..27 | -14..27 | -22..19 |
| 2 | -12..12 | -14..27 | -6..27 | -14..19 |
| boule | -7..7 | -4..27 | 4..27 | -4..19 |
| os | -10..10 | -10..27 | -2..27 | -10..19 |

Objets balayés : 0A (stage 8 salle 6, stage 9 salle 1), 0B (stage 8 salle 2, stage 9 salle 5), 0C (stage 8 salle 6), 0D (stage 9 salle 5), 08 (stage 9 salle 1), boule et os.
L'eau bénite n'a pas été balayée (vol en arc) : même `8500` (B = 12 pour tout type différent de 2), donc mêmes fenêtres que la hache.

## Bords hauts de la statue (E 147 / 148) : élucidé

Chaîne complète, statue 0A épinglée en (144, E), PV tenus à 8, boule et leaper retirés, dague lancée par ESPACE à Y 128 (Simon en (60, 144)) : touche à E = 146, 147, 148,
149 à 152, **ne touche pas à 153** (le bord haut est donc bien 152 = 128 + 24, formule de `8500`). Avec le leaper `0C` laissé dans la salle, l'essai à 147
ne touche pas : la dague s'arrête sur le leaper (son d'impact `804C` avec IX = `C880`, type 0C) avant d'atteindre la statue. La limite 147 / 148 de `weapon-targets.md` était donc
une interception par un autre objet, pas une boîte plus basse. Bords bas confirmés sur 3 191 appels (hache, croix, dague) : dague touche jusqu'à Y - E = +4 (limite +5 par la formule), hache et croix jusqu'à +11 ;
hache et croix touchent à E = 117 et pas à 116 (Y 128).

## Écart trouvé et corrigé : le second emplacement d'arme (C460)

`84F0` appelle `8500` pour C450 puis C460 **sans recharger HL** : le premier appel laisse L + 6 (toujours) et H + B (B = 6 si l'emplacement 0 est une dague, 12 sinon,
emplacement vide compris) quand son test de X est passé. Le second emplacement teste donc `|dX| < L + 12` et, derrière une arme de C450 qui passe en X sans toucher en Y, une
fenêtre verticale qui commence B plus haut. Observé : journal des appels de `8500` (L = 0Bh et H = 1Eh aux entrées de C460), balayages (statue : C450 ±10, C460 ±16 ; bougie : ±13 contre ±19),
et en chaîne complète avec deux dagues (X 130, dX -14 : C450 manque à l'image 1278, la dague de C460 touche à 1290 ; la première touche à X 135). Le port testait les deux
emplacements avec les mêmes H et L. Corrigé : `vk_shot_touches_slot` (`weapons.c`) reproduit le chaînage, utilisé pour les ennemis, les boss (`actors.c`, `shot_touch`), les bougies et les
objets muraux. Aucun test existant n'a changé de sens hormis le cas « dague en C460 à dX -11 », qui ne touchait pas par construction et touche maintenant (limite ±16).

## Objet 08 isolé

Épinglé en (130, 105) (stage 9 salle 1, autres objets retirés, Simon seul) : `+D` (points de vie) = 1, `+E` = 07 (drapeaux de touche, contact, fouet). Dague, hache, croix et fouet le
tuent au premier coup (score `C405-C407` 000000 -> 000200 : 200 points) ; la dague disparaît, la hache et la croix continuent. Le bloc devient ensuite un objet de flamme de mort (1E, ou 26
selon la passe). Contact (Simon immobile à X = 130, Y = 112) : énergie 48 -> 44 (4 points), `C420` = 5 (recul), l'objet 08 survit au contact. Boîte (10h, 8) pour le contact, l'arme et le
fouet. Le port avait déjà ces valeurs (PV 1, 200 points, dégâts 4, classe 2) : seul le test natif les consigne (`tests/test_weapons.c`).

## Port : état

Les formules du port étaient exactes pour les classes 1, 2, boule et os dans les trois tests (contact, fouet, premier emplacement) : le test natif balaie les quatre bords de chaque
classe, contact debout, saut et accroupi, fouet cuir et chaîne à droite et à gauche, dague / hache en C450 et en C460. Le scénario `zzl_enemy_boxes.txt` (statue 0A de la salle 6 du stage 8,
41 assertions) vérifie à 1 px près : bords X et Y de la dague, de la hache, de la croix, de C460 (seul et derrière une dague), du fouet (X et Y) et du contact (X, Y debout, Y accroupi).

## Non tracé

- Eau bénite : fenêtre non balayée (arc de vol), ROM lue seulement ; elle occupe C460 et subit donc le chaînage ci-dessus.
- Contact en saut : fenêtres tracées (`C420` = 1) mais pas de scénario MD au pixel près (Simon en l'air ne tient pas une position posée) ; couvert par le test natif.
- Ordre exact des objets quand deux touchent la même passe : un seul par passe (`801E`), le premier de `C800`, déjà dans le port.
