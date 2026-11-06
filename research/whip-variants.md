# Fouets : variantes dans la ROM MSX2 et séquence accroupie

> **Correction (30 octobre)** : `C416` = 2, 3 et 4 ne sont pas des fouets mais la dague, la hache et la croix lancées (objets 1B, 1C, 1D). Les compositions 12-14 / 27-29 sont la pose de lancer ; le « verrou » de `C450` est la vie du projectile (hache : 94 images de boomerang à droite, bord gauche atteint en 6 images depuis X 16). Voir `secondary-weapons.md`.

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
- Niveau 3 : **échecs** pour les appuis qui tombent pendant un verrou de l'emplacement `C450` (20 appuis sur 64 au total, 10 par run, voir « Niveau 3 : échecs et séquence »). Les coups acceptés donnent `12/13/14` à droite et `27/28/29` à gauche. Le coup accroupi à gauche produit `16/27..29` (prouvé).

### Niveau 3 : échecs et séquence (27 octobre, mesure des emplacements)

Mesure : `tools/measure_whip_slots.py 3 DIR` (Tcl `tools/trace_whip_slots.tcl`, analyse `tools/analyze_whip_slots.py`). Une série par run : 8 coups debout à droite, 8 accroupis à droite, marche à gauche (LEFT 0,6 s), 8 debout à gauche, 8 accroupis à gauche, avec des intervalles entre débuts de 0,25 ; 0,5 ; 0,75 ; 1 ; 1,25 ; 1,5 ; 2 ; 3 s. `C416` forcé à 3 à 41 s. Deux runs identiques : `scratchpad/slots/L3-r{1,2}/slots.csv`.

**Cause (prouvée : désassemblage et mesure).** Le verrou vient d'un emplacement de fouet, pas du son, de la posture ou de la position :

1. Lancement (`0x7116`, banque 1) : SPACE (`C006` & 0x10) et `C422` = 0 (pas d'attaque en cours). `C436` reçoit `C416`. Niveaux 0 et 1 : lancement direct. Niveaux 2 et 3 : appel de `0x71BE`.
2. `0x71BE` : `B` = 2 si `C436` = 2, sinon `B` = 1. La routine parcourt les emplacements de 16 octets `C450` puis `C460` et prend le premier dont l'octet `C451` est nul (elle y écrit `C450` = 1). Si aucun n'est libre, `B` = 0, le lancement est **abandonné sans écriture** (`C42E` et `C42F` restent inchangés, `C42F` = 0).
3. Au niveau 3, `B` = 1 : **un seul emplacement** (`C450`). Tant que `C451` est non nul, tout appui SPACE est ignoré.
4. `C451` reçoit `C436` (= 3) à la fin de la composition (`0x728C`). Le gestionnaire de type 3 est en `0x7473` (table `0x72D6`, index `C451` - 2). Il appelle aussi le son 6 (`CALL 0x509F` avec A = 6) quand `C003` & 7 = 0 : c'est la source de `C096` = 6 pendant le verrou (observé). `C096` n'intervient pas dans la condition de lancement.

Durée du verrou (identique dans les deux runs) :
- face à droite (`C42C` = 0) : `C451` non nul pendant **94 trames**, de début + 10 à début + 103 (debout et accroupi) ;
- face à gauche (`C42C` = 1) : **6 trames** (début + 10 à début + 15). Le verrou à gauche est donc presque nul.

Règle vérifiée : sur 32 appuis par run (64 au total), « appui ignoré » ⇔ « `C451` non nul à l'appui » : **accord 64/64**. Appuis ignorés dans chaque run : debout à droite n°2, 4, 5, 7 ; accroupi à droite n°2, 4, 5, 7 ; debout à gauche n°5 ; accroupi à gauche n°5 (intervalle de 15 ticks après l'appui précédent).

Hypothèses éliminées :
- **Posture** : debout et accroupi sont bloqués aux mêmes appuis (droite n°2, 4, 5, 7).
- **Son** : appuis ignorés avec `C096` = 0 (accroupi à droite n°4 et 5), appui accepté avec `C096` = 6 (accroupi à gauche n°5).
- **Position** : `C427` vaut 16 à droite et 14 à gauche, pour les appuis acceptés comme pour les ignorés. La lecture de lancement ne lit pas la position.
- **Cooldown** : pas de compteur de cooldown dédié. Le blocage est la durée de vie du gestionnaire (94 ou 6 trames).

Écrivains mesurés (`tools/trace_whip_slot_writers.tcl`, un coup debout à droite à 42 s, un accroupi à droite à 60 s ; PC = adresse de l'instruction, à 1 près) :
- `C450` ← 1 en `0x71D4` (lancement, `0x71BE`) ;
- `C451` ← 3 en `0x72A6` (fin de composition, `0x728C`) ;
- `C450` ← 2 puis 3 en `0x734F` (gestionnaire, états successifs) ;
- `C453` décroît de 3 à -1 (255) par pas d'une valeur toutes les 4 trames (écritures en `0x7395`) ;
- `C450` ← 0 et tout le bloc `C450`..`C45F` effacé en `0x74EC` (routine `0x74E9`) : fin du verrou, au tick 2378 (42 s + 1,8 s), en accord avec la mesure.

Qui appelle `0x74E9` à la fin (déduit du désassemblage, non confirmé par la pile) : le chemin `0x74A5` du gestionnaire `0x7473`, quand `C433` vaut 2 ou 3 (`0x748C`), ou le test de position de `0x74CE`. Ce second chemin est exclu pour le coup mesuré (position hors fenêtre). Pourquoi 94 trames à droite et 6 à gauche : **non décodé**. Le niveau 2 (`B` = 2, deux emplacements) n'a pas été mesuré avec ce protocole.

**Séquence niveau 3** (prouvée, 2 runs, un tick = une image VBLANK ; `C42E` et `C42F` au premier tick de la composition) :

| Direction | Posture | `C42E` avant | `C42E` pendant | `C42F` (ticks) |
|---|---|---|---|---|
| droite | debout | 0 | 0 | 12 ×2, 13 ×3, 14 ×3 |
| droite | accroupi | 6 | 6 | 12 ×2, 13 ×3, 14 ×3 |
| gauche | debout | 10 | 10 | 27 ×2, 28 ×3, 29 ×3 |
| gauche | accroupi | 10 (debout), puis 6 (1 tick) | 16 | 27 ×2, 28 ×3, 29 ×3 |

Chaque composition dure 8 ticks. Le triplet est le même qu'au niveau 2 (12-14 / 27-29) : seules les durées (6 ticks au niveau 2, 8 au niveau 3) et le verrou diffèrent.

**Lanière par trame** (premier coup debout à droite, base de repos sans verrou précédent ; décodage 1 bpp, **déduit**, précision ±1 trame ; trame 0 = premier tick de la composition) :

| Trames | Boîte x (relative à Simon) | Boîte y |
|---|---|---|
| +8, +9 | +8 à +12 | -20 à -16 |
| +10, +11 | -5 à +10 | -31 à -16 |
| +12, +13 | -2 à +13 | -31 à -16 |
| +14, +15 | +1 à +16 | -31 à -16 |
| +16, +17 | +4 à +19 | -31 à -16 |
| +18, +19 | +7 à +22 | -31 à -16 |
| +20 | +10 à +25 | -31 à -16 |

Le bord avant avance d'environ 3 px toutes les 2 trames (**déduit** des boîtes de sprites). Ce n'est pas `C453` : cet octet, égal à 3 à l'activation, décroît de 1 toutes les 4 trames (écritures mesurées). La liste de sprites du niveau 3 reste **incertaine** : les noms exclus par la fenêtre de repos de 60 trames changent d'un coup à l'autre. Hypothèse probable : la lanière précédente, encore présente pendant 94 trames, fait partie de cette fenêtre (non vérifié).

**Niveau de preuve** : séquence niveau 3 et règle de blocage (64/64) : **prouvé**. Code de lancement et de verrou (`0x71BE`, `0x728C`, `0x7473`) et écrivains de `C450`/`C451` : **prouvé** (désassemblage et écritures mesurées). Sens des valeurs de `C451` et chemin de fin `0x74A5` : **déduit**. Durée 94/6 trames : **mesurée**, mécanisme **inconnu**. Lanière par trame : **déduite** (1 bpp). Liste de sprites : **incertaine**.

**Fichiers et empreintes** : `tools/measure_whip_slots.py`, `tools/trace_whip_slots.tcl`, `tools/analyze_whip_slots.py`, `tools/trace_whip_slot_writers.tcl` (ajoutés). Mesures hors dépôt : `scratchpad/slots/L3-r1/slots.csv` et `L3-r2/slots.csv`, SHA-256 identique `449dfb55aeb59eba3538d8d4812e0c209246e6962394c23d628e040308094df5` (les deux runs sont identiques, donc déterministes). Écrivains : `scratchpad/slots/writers/writers.csv`.

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

## Chronologie du niveau 2 (6 images, 27 octobre)

**Problème** : la chronologie partagée de `sgdk/src/whip.c` (15 entrées, timer 14..0) donnait 12 images au niveau 2 (2/2/8, comme les niveaux 0 et 1). La mesure donne 6 images (2/2/2).

**Répartition retenue** : le niveau 2 a sa propre table (`vk_attack_keys_level2`), avec le même départ (timer 14) :

| Timer | Phase du bras | Image debout (pas `vk_whip_visual_step`) | Image accroupie | Composition haute (`C42F`) |
|---|---|---|---|---|
| 14, 13 | 1 | 0 | 1 | 12 (2 ticks) |
| 12, 11 | 1 | 1 | 2 | 13 (2 ticks) |
| 10, 9 | 1 | 2 | 3 | 14 (2 ticks) |
| 8..0 | 0 | 0 | 0 | 0 |

- Le corps est affiché sur les timers 14..9 (6 images). Les niveaux 0 et 1 restent sur 14..3 (12 images) ; leur table et `vk_whip_active` ne changent pas.
- Autres répartitions possibles (étirer 2/2/2 sur les 12 timers, ou le centrer sur la fenêtre de coup 3..10) : rejetées, car elles ne suivent pas la durée mesurée (6 ticks, deux runs identiques) et placeraient l'image 14 hors du départ mesuré.
- Le tableau de la colonne accroupie suit les mêmes pas de 2 images, car le corps accroupi partage la phase du bras. Le MD dessine les images accroupies du niveau 0 quel que soit le niveau (`main.c`) : seule leur durée change au niveau 2.

**Preuve** :
- Durée et séquence debout du niveau 2 : **prouvé** (forcé `C416` = 2, 2 runs, `C42F` = 12,12,13,13,14,14 puis 0 ; voir « Mesures par niveau »).
- Alignement sur le départ (timer 14 = premier tick de `C42F`) : **déduit** du tracé openMSX du niveau 0 (12 ticks = timers 14..3) ; le niveau 2 suit la même règle.
- Test natif : `tests/test_whip.c` vérifie 6 images au niveau 2 (`0,0,1,1,2,2`), 12 images et la séquence inchangées au niveau 0, et la fenêtre de coup identique.

**Limites (non vérifiées)** :
- La fenêtre de coup reste 3..10 pour tous les niveaux. Elle n'a pas été mesurée au niveau 2 : après le timer 9, le corps ne montre plus la lanière, mais la boîte reste active jusqu'au timer 3. À vérifier à l'écran et en mesure.
- Accroupi au niveau 2 : la mesure donne 2/2/3 (7 ticks, 6/12 ×2, 6/13 ×2, 6/14 ×3, 2 runs de la section « Mesures par niveau ») ; la table en donne 6 (les pas 2/2/2 suivent la durée debout). Non reproduit exactement, et non décodé : le MD ne mesure pas l'accroupi au niveau 2.
- Rien n'a été vu à l'écran (openMSX absent).

## Ce qui reste inconnu

1. **Déclencheur du niveau dans le jeu** : l'écriture `0x8D76` (valeur 1 à 75 s) est observée, mais sa condition (table lue, objet touché) n'est pas décodée. Les niveaux 2 et 3 ne sont pas atteints en jeu normal dans une session de 220 s.
2. **Hitbox par niveau** : niveaux 0 et 1 mesurés (1 bpp, deux runs). Niveaux 2 et 3 : balayage mesuré, mais la liste des sprites du niveau 3 est incertaine. Décodage 1 bpp **déduit**, à valider sur une image de référence.
3. **Son** : niveaux 0 et 1 : rafale PSG identique (`r4=0, r6=2, r7=156, r10=5`). Niveaux 2 et 3 : pas de rafale de ce type (seulement des volumes sur `r8`–`r10`). Une éventuelle différence de son hors PSG n'est pas testée (C096/C016 non concluants pour le niveau 3).
4. **Échecs du niveau 3** : expliqués (verrou de `C450`, voir « Niveau 3 : échecs et séquence »). Reste à trouver l'écrivain qui remet `C450` à 0 (`tools/trace_whip_slot_writers.tcl`, non exécuté) et le mécanisme 94/6 trames.
5. **Gauche et droite** : accroupi à gauche prouvé pour les quatre niveaux (base 16 = 6 + 10).

## Conséquence pour le portage

- Le MD porte les niveaux 0, 1 et 2 (`dc6a353`, `2cc20f8`, `3cac5ce`). Le niveau 0 reste le défaut : aucune touche ni aucun événement de jeu ne choisit encore un autre niveau, donc le comportement en partie normale ne change pas.
- Le niveau 1 est atteint par le jeu normal et ne diffère que par la longueur de la lanière et la liste de sprites (durée et son identiques). Son rendu et sa boîte restent à vérifier à l'écran.
- Le niveau 2 a sa chronologie propre de 6 images (`3cac5ce`). Son rendu n'est pas vérifié à l'écran.
- Le niveau 3 n'est pas porté : ses compositions et sons sont distincts, et son déclenchement dans le jeu n'est pas établi (écriture `0x8D76`, condition de la table).
- `main.c` et `whip.c` ont été modifiés pour brancher les niveaux 1 et 2 ; le niveau 0 garde son chemin d'origine.

## Fichiers et empreintes (27 octobre)

- `tools/measure_whip_levels.py`, `tools/measure_whip_levels.tcl`, `tools/analyze_whip_levels.py`, `tools/trace_c416_writers.tcl` (ajoutés au dépôt).
- Mesures (hors dépôt) : `scratchpad/lvl/m/L{0..3}-r{1,2}/ticks.csv` (SHA-256 `L0-r1` : `d8d48a62dd50be0edf3e…`), `scratchpad/lvl/watch3/writes.csv` (SHA-256 `aced0137fcf759e0ae81…`).

## Palette des fouets 0, 1 et 2 (décision, 27 octobre)

**Décision** : le corps et le fouet (frames standing des niveaux 0, 1 et 2) sont dessinés sur **PAL2**, chargée avec la palette de la salle stage 1 salle 0 (`vk_palettes[0]`, `generated/rooms.c`). Les 16 mots de `vk_whip_level1_palette` et `vk_whip_level2_palette` sont identiques à `vk_palettes[0]` (test `test_palettes_are_the_stage1_room0_palette_used_by_the_wired_body`). Aucune palette propre au niveau n'est chargée.

**Preuve** :
- `main.c` (`enter_play`, `show_room`) : `PAL2 = vk_palettes[...]` ; `show_player` dessine le corps (sprites 5 et 6) avec `PAL2`.
- `attack_palette` (`0x002A, 0x006E, 0x00CE`) est chargée sur **PAL3**, qui ne sert qu'aux tuiles placeholder (fouet, bras), aux flammes et à la cible : elle n'intervient pas dans le coup de Simon.
- Les niveaux 0, 1 et 2 ont la même structure de couches (parité, CC) : mêmes indices de couleur 1 à 3 (VRAM 0xF400 pour le niveau 0 : 0x01 et 0x42).

**Limite** : la palette RGB vivante de la VDP pendant le coup n'a pas été relue pour les niveaux 1 et 2 (openMSX absent de l'environnement de cette session). Le lien « palette vivante = palette de salle » est donc **déduit** de la structure MSX2 (une palette VDP partagée), pas mesuré. Statut : prouvé côté asset, non vérifié à l'écran.
