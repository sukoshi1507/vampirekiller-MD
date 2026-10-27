# Fouets : variantes dans la ROM MSX2 et séquence accroupie

Objectif : savoir combien de fouets (apparences) le jeu MSX2 contient, comment on en change, et quelle est la séquence accroupie complète à porter. Aucun fichier de `validation/` n'a été modifié. Rien n'est committé ici hors de ce document et du commit de `sgdk/src/generated/simon_crouch_*`.

## Méthode

- ROM : `Vampire Killer (Japan, Europe) (En).zip` à la racine (SHA-256 `99eabaae…65e7e2` vérifié par `tools/probe.py`), machine `C-BIOS_MSX2`, openMSX 21.0 compilé dans le scratchpad.
- Lancement : `run_trace.py SCRIPT.tcl OUTDIR SECONDES` (scratchpad `anim-inventory/`). Le script Tcl échantillonne à chaque VBLANK (point d'entrée `0x4028`) : `C42E`, `C42F` (index bas et haut de la composition de Simon), `C427`/`C425` (x, y), `C416`/`C436`, `C096`/`C016` (effet sonore), `C006`/`C007` (entrées).
- Entrées : `keymatrixdown 8 MASQUE`, masque de bits sur la ligne 8. SPACE = 1, LEFT = 16, UP = 32, DOWN = 64, RIGHT = 128. Vérifié : LEFT fait reculer Simon (`C427` 16 → 14), RIGHT le fait avancer (14 → 74), DOWN seul donne `C42E` = 6.
- Lecture statique : désassemblage (`z80dis` du venv du scratchpad, `anim-inventory/dis.py`). Adresses CPU : fichier = CPU − 0x4000 pour la page 0x4000-0x7FFF utilisée par ce script. Les adresses hors de cette page n'ont pas de banque établie.
- Scripts et sorties (hors dépôt) : `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/` :
  - `variants/out/ticks.csv` (gauche, droite, saut) ;
  - `variants/level416/ticks.csv` (niveaux forcés par écriture de `C416`) ;
  - `variants/level/ticks.csv` (écriture directe de `C436`, annulée, voir plus bas) ;
  - `crouch/out/ticks.csv` (attaque accroupie, 4 coups), `crouch/probe/probe.csv` (DOWN seul, puis DOWN+SPACE) ;
  - `whip/out/` (attaque debout, 4 coups, VRAM complète par tick).

## Réponse courte

Le jeu a **trois fouets** (trois triplets de compositions), chacun en version droite et gauche (miroir). Le niveau est porté par `C416` (copiée dans `C436`). Mesures du 27 octobre (deux runs par niveau, forçage de `C416`) : niveaux 0 et 1 ont la même rafale sonore, des durées de 12 ticks, et une lanière qui couvre x +8..+32 (niveau 0) ou x +8..+39 (niveau 1). Niveaux 2 et 3 : 6 et 8 ticks, sans rafale sonore du niveau 0, et une lanière qui balaie vers l'avant. **Le jeu normal atteint le niveau 1** (écriture de `C416` = 1 à 75 s, observée). Niveaux 2 et 3 : non observés en jeu normal. Le portage actuel ne porte que le niveau 0.

## Preuves

### 1. Trois triplets de compositions dans la ROM (prouvé, table A2D1 / A281)

Les compositions haute (`C42F`, table `A2D1`) et basse (`C42E`, table `A281`) sont décodées par `tools/decode_extended_simon.py:components`. Les étendues ci-dessous sont celles des pixels opaques (union des couches, origine de composition).

| Triplet | Haut, droite | Étendue x (couches) | Haut, gauche (+15) | Vérifié |
|---|---|---|---|---|
| A | 6, 7, 8 | 6 : −24..−1 ; 7 : −24..−1 ; 8 : −8..32 | 21, 22, 23 | encre identique 274/184/274 |
| B | 9, 10, 11 | 9 : −24..−1 ; 10 : −24..−1 ; 11 : −8..39 | 24, 25, 26 | encre identique 292/206/295 |
| C | 12, 13, 14 | 12 : −16..−1 ; 13 : −9..−1 ; 14 : −8..12 | 27, 28, 29 | encre identique 188/136/209 |

- Le miroir gauche est un décalage d'index : **+15 sur le haut**, **+10 sur le bas** (`C42E` 0 → 10). Une routine en `0x7660–0x7676` ajoute ces valeurs (lecture de `research/simon-assets/ANIMATION_SEQUENCES.md`), et la mesure le confirme (ci-dessous).
- Les index bas 1 et 2 sont d'autres poses du corps (marche), pas des fouets.
- Un décalage de +8 px entre origine de composition et Simon est supposé (la lanière mesurée en jeu, `research/whip-hitbox.md`, va jusqu'à +39). Non vérifié.

### 2. Sélection du triplet par `C436` (prouvé statiquement, testé en émulateur)

- `0x7123–0x7126` : `LD A,(C416) ; LD (C436),A`. `C436` est donc une copie de `C416`.
- `0x7200–0x7230` : au début du coup, `C436` (lu par `LD A,(C436)` puis `CP 2`) choisit l'index haut : **0 → 6 (triplet A), 1 → 9 (triplet B), ≥ 2 → 12 (triplet C)**. Les écritures `LD (C42F),A` sont en `0x721D` (valeur 6 ou 9) et `0x722F` (valeur 12).
- `0x70E5` : `C416` est remis à 0 (dans une routine d'initialisation, probablement entrée de salle ou de partie). Deux autres écritures de `C416` sont aux adresses CPU `0x8D76` (valeur lue moins 25, observée à 1 en jeu normal) et `0x8E94` (valeur 0). Voir « Mesures par niveau ».
- Test : écrire `C436` directement (`debug write memory`) est annulé au tick suivant par la copie depuis `C416` : **non concluant**. Écrire `C416` (`variants/level416`) donne le résultat attendu, avec le triplet choisi :

| `C416` forcé | Séquence `C42F` (ticks VBLANK, deux par image) | `C42E` | Son au début (`C096`) |
|---|---|---|---|
| 0 | 6,6,7,7,8×8 (12 ticks) | 0 | 5 (`C016` = 0x9C) |
| 1 | 9,9,10,10,11×8 (12 ticks) | 0 | 5 (`C016` = 0x9C) |
| 2 | 12,12,13,13,14,14 (6 ticks), puis 0 | 0 | aucun début détecté (`C096` = 0), puis 4 |
| 3 | 12,12,13,13,13,14,14,14 (8 ticks), puis 0 | 0 | aucun début détecté |

Les niveaux 2 et 3 n'ont été mesurés qu'une fois chacun. Les durées diffèrent (6 et 8 ticks), ce qui n'est pas expliqué.

### 3. Gauche et droite, air (prouvé, une occurrence chacun sauf mention)

- Marche à gauche puis SPACE : `C42E` = 10, `C42F` = 21, 22, 23 (miroir de 6, 7, 8). Preuve du décalage +15/+10.
- Marche à droite puis SPACE : 0 et 6, 7, 8.
- UP+SPACE (saut, `C425` 176 → 150) : `C42F` = 6, 7, 8, `C42E` = 0. Le saut utilise donc le même fouet, qui contredit la note `sound-triggers.md` (« puis 22 ») : cette note concernait sans doute un coup gauche.

### 4. Accroupi (prouvé, 4 coups + 1 contrôle)

- DOWN seul : `C42E` = 6, `C42F` = 0 (accroupi, sans coup). `DOWN+SPACE` : `C42E` reste à 6 jusqu'au tick 14 même si DOWN est relâché avant (tick 9). Le bas garde 6 pendant tout le coup.
- Les 4 coups sont identiques : `C42F` = 6,6,7,7,8,8,8,8,8,8,8,8 puis 0 (ticks 1 à 12), 0 aux ticks 13 et 14.
- Donc la séquence accroupie complète est : **bas 6, haut 6 (2 ticks), 7 (2 ticks), 8 (8 ticks)**. Les images accroupies de la version MD précédente (haut 0, 2, 6) ne couvraient pas 7 ni 8, et utilisaient (6,2) sans preuve. Corrigé dans le commit `ebcc251` (voir ci-dessous).
- Dans la VRAM (sprites), la lanière accroupie est présente (sprites `0x10…0x1C`, comme debout), décalée de 6 px vers le bas (y 151 au lieu de 145 au tick 9). La lanière fait donc partie de la composition, et ne doit pas être dessinée à part.

### 5. Ce que la composition accroupie couvre (vérifié à l'image)

Les frames (6,6), (6,7), (6,8) générées depuis la ROM (`tools/build_simon_crouch_md.py`, canevas 64x32 à l'ancrage de l'attaque) montrent un fouet qui se déploie : enroulé puis tendu vers l'avant (rendu inspecté visuellement, non comparé à une capture MSX).

## Mesures par niveau (2026-10-27, répétées)

Méthode : `tools/measure_whip_levels.py NIVEAU SORTIE` (Tcl : `tools/measure_whip_levels.tcl`), deux runs par niveau (`-r1`, `-r2`), `C416` forcé à 41 s. Chaque run fait trois coups debout face à droite, un accroupi à droite, deux coups debout à gauche (après LEFT) et deux accroupis à gauche. Analyse : `tools/analyze_whip_levels.py SORTIE`. Par run : 3 coups debout, 1 accroupi droite, 2 debout gauche, 2 accroupi gauche, soit 8 coups par niveau. Un coup naturel (C416 non forcé, 36 s) sert de contrôle pour le niveau 0.

Sortie : `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/lvl/m/L{0..3}-r{1,2}/` (`ticks.csv`, `psg.csv`, `sat-pat.bin`).

Lanière : décodage MSX2 1 bpp (motif n à `0xF800 + 8n`, quadrants UL, LL, UR, LR), coordonnées relatives à Simon (`C427`, `C425`). Ce décodage remplace le décodage 4 bpp de `whip-hitbox.md`, qui donnait une boîte plus grande (x +8..+39, y -31..-16). Forme : **déduit** (le motif 0x10 est cohérent avec une lanière), pas validé par image de référence.

Durée d'une composition : nombre de ticks VBLANK (1 tick = 1 image à 60 Hz, mais une image de `C42F` dure souvent 2 ticks). Le coup est repéré par le premier tick où `C42F` devient non nul.

| Niveau (fouet) | Triplet haut droite / gauche | Séquence `C42E/C42F` droite (ticks) | Durée | Lanière mesurée (relative à Simon) | Son au début (PSG, +1 tick) | Sélection | Preuve |
|---|---|---|---|---|---|---|---|
| 0 | 6,7,8 / 21,22,23 | 0/6 ×2, 0/7 ×2, 0/8 ×8 | 12 ticks | x +8..+32, y -23..-16, constante sur les ticks +6..+13 | r4=0, r6=2, r7=156, r10=5 ; +2 : r6=6, r9=14, r10=7 | `C416` = 0 (valeur d'initialisation). Coup naturel à 36 s reproduit ce tracé | **prouvé** (4 coups par run, 2 runs, plus le coup naturel) |
| 1 | 9,10,11 / 24,25,26 | 0/9 ×2, 0/10 ×2, 0/11 ×8 | 12 ticks | x +8..+39, y -23..-16 | identique au niveau 0 | `C416` = 1 forcé. **Naturel** : écriture de 1 à 75 s (PC 0x8D76), voir ci-dessous. Effet de ce niveau après 75 s non relu (les ticks n'étaient pas journalisés) | **prouvé** (forcé, 2 runs) ; sélection naturelle **observée** (une écriture) |
| 2 | 12,13,14 / 27,28,29 | 0/12 ×2, 0/13 ×2, 0/14 ×2 | 6 ticks | balayage : +8..+12 (y -20..-16) à +6/+7, puis x -3..+12 à +8/+9, glisse jusqu'à x +27..+42 à +15, y -27..-19 | pas de rafale du niveau 0 : r8/r9/r10 = 13,14,13 puis 12,13,12 (volumes seulement) | `C416` = 2 forcé ; **non observé** en jeu normal (une session de 220 s) | **prouvé** (forcé, 2 runs) |
| 3 | 12,13,14 / 27,28,29 | 0/12 ×2, 0/13 ×3, 0/14 ×3 | 8 ticks | balayage x -5..+10 à x +10..+25, y -31..-16 ; liste de sprites plus longue (noms 0x10 à 0x34), **incertain** | pas de rafale du niveau 0 (mêmes volumes r8/r9/r10 que le niveau 2) | `C416` = 3 forcé ; **non observé** en jeu normal | **prouvé** (séquence) ; hitbox **déduit** |

Accroupi (`C42E` à 6 à droite, 16 à gauche = 6 + 10, prouvé pour les quatre niveaux) :
- Niveau 0 : droite `6/6 ×2, 6/7 ×2, 6/8 ×8` ; gauche `16/21 ×2, 16/22 ×2, 16/23 ×8` (**prouvé**, 2 coups par run).
- Niveau 1 : droite `6/9, 6/10, 6/11` ; gauche `16/24, 16/25, 16/26` (prouvé).
- Niveau 2 : droite `6/12 ×2, 6/13 ×2, 6/14 ×3` ; gauche `16/27, 16/28, 16/29` (prouvé).
- Niveau 3 : **échec** : le coup accroupi à droite ne produit aucune composition (`C42F` = 0 pendant 14 ticks, deux runs), et le coup accroupi à gauche produit `16/27..29` (prouvé). Cause : **inconnue** (voir plus bas).

Échecs au niveau 3 : dans les deux runs, le deuxième coup debout à droite (43,5 s) et le coup accroupi à droite ne produisent aucune composition. Pendant ces coups, `C096` vaut 6 et `C016` vaut 0x9C, alors que le troisième coup (1,5 s plus tard) fonctionne. Le début de son du niveau 3 n'est donc pas celui des niveaux 0 et 1. Cause : **inconnue** (possible verrou du son ou de l'état d'attaque, non isolé).

### Sélection : écritures réelles de `C416` (observé)

Trace de 220 s de marche sans forçage (`tools/trace_c416_writers.tcl`, PC lu dans `CPU regs` 20 et 21, big-endian ; validé : 0x70E5 à l'initialisation, comme la note précédente) :

| Temps | Valeur | PC | État (`C000`, `C001`) | Contexte |
|---|---|---|---|---|
| 0,1 s | 0 | 0x830F | 0, 0 | démarrage (BIOS) |
| 3,8 s | 0 | 0x40A5 | 0, 0 | démarrage |
| 12,3 s | 0 | 0x44D2 | 3, 2 | intro |
| 18,2 s | 0 | 0x70E5 | 3, 4 | initialisation (écriture connue) |
| 75,1 s | **1** | **0x8D76** | 5, 0 | coup gauche en cours (`C42E` = 10, `C42F` = 23) |
| 201,0 s | 0 | 0x70E5 | 5, 0 et 6, 0 | réinitialisation |

- `0x8D76` : `SUB 25` puis `CP 5` puis `JR Z` ; le `LD (C416),A` est exécuté si la valeur n'est pas 30. La valeur écrite est donc une valeur lue (table ou état) moins 25 ; ici 26 donne 1. **Déduit** : la lecture vient d'un objet touché par le fouet (il est appelé pendant `C42F` ≠ 0), non prouvé.
- `0x8E94` : `LD (C416),A` avec `A = 0` (chemin `XOR A`) : met à 0. Le writer `0x8E94` n'a pas été observé en jeu.
- Donc, en partie normale : **le niveau 1 est atteint** (écriture de 1 par le jeu, observée). Niveaux 2 et 3 : **non observés** en 220 s.
- Le déclencheur précis (objet, collision, salle) : **inconnu**. La condition de `JR Z` et la table lue par `0x8D76` ne sont pas décodées (l'adresse de la table est en dehors de la page décodée).

## Ce qui reste inconnu

1. **Déclencheur du niveau dans le jeu** : l'écriture `0x8D76` (valeur 1 à 75 s) est observée, mais sa condition (table lue, objet touché) n'est pas décodée. Les niveaux 2 et 3 ne sont pas atteints en jeu normal dans une session de 220 s.
2. **Hitbox par niveau** : niveaux 0 et 1 mesurés (1 bpp, deux runs). Niveaux 2 et 3 : balayage mesuré, mais la liste des sprites du niveau 3 est incertaine. Décodage 1 bpp **déduit**, à valider sur une image de référence.
3. **Son** : niveaux 0 et 1 : rafale PSG identique (`r4=0, r6=2, r7=156, r10=5`). Niveaux 2 et 3 : pas de rafale de ce type (seulement des volumes sur `r8`–`r10`). Une éventuelle différence de son hors PSG n'est pas testée (C096/C016 non concluants pour le niveau 3).
4. **Échecs du niveau 3** : coup debout n°2 et accroupi à droite sans composition, dans deux runs. Cause inconnue (`C096` = 6 pendant l'échec).
5. **Gauche et droite** : accroupi à gauche prouvé pour les quatre niveaux (base 16 = 6 + 10).

## Conséquence pour le portage

- Le MD ne porte que le niveau 0. Le niveau 1 est atteint par le jeu normal et ne diffère que par la longueur de la lanière et la liste de sprites (durée et son identiques), ce qui permet de le porter sans nouvelle trame de composition.
- Les niveaux 2 et 3 ont des compositions et sons distincts, mais leur déclenchement dans le jeu n'est pas établi. Les porter demande d'abord le déclencheur (écriture `0x8D76`, condition de la table).
- Cette étape ne modifie pas `sgdk/src/` : `whip.c` et `main.c` restent inchangés.

## Fichiers et empreintes (27 octobre)

- `tools/measure_whip_levels.py`, `tools/measure_whip_levels.tcl`, `tools/analyze_whip_levels.py`, `tools/trace_c416_writers.tcl` (ajoutés au dépôt).
- Mesures (hors dépôt) : `scratchpad/lvl/m/L{0..3}-r{1,2}/ticks.csv` (SHA-256 `L0-r1` : `d8d48a62dd50be0edf3e…`), `scratchpad/lvl/watch3/writes.csv` (SHA-256 `aced0137fcf759e0ae81…`).
