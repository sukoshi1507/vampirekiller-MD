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

Le jeu a **trois fouets** (trois triplets de compositions), chacun en version droite et gauche (miroir). Ils ne sont **pas** changés par un état « niveau » observé dans le jeu : la variable qui les choisit (`C436`, recopiée depuis `C416`) n'a été changée que par écriture RAM dans l'émulateur. Le fouet 0 est le seul utilisé par le portage actuel, et le seul observé dans une partie normale (23 coups mesurés précédemment, plus les contrôles de cette note).

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
- `0x70E5` : `C416` est remis à 0 (dans une routine d'initialisation, probablement entrée de salle ou de partie). Deux autres écritures de `C416` existent à des adresses hors de la page décodée (fichier `0x4D76`, `0x4E94`) : non décodées.
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

## Ce qui reste inconnu

1. **Où le jeu écrit `C416`** : les deux écritures hors de la page décodée (`0x4D76`, `0x4E94`) ne sont pas identifiées. Sans elles on ne sait pas si un objet ou un événement change le fouet. Aucun niveau 1 ou 2 n'a été observé en jeu normal.
2. **Hitbox par niveau** : `vk_whip_bounds` n'a qu'une forme. Les étendues de la ROM (+32, +39, +12 pour les dernières frames) suggèrent des portées différentes, non mesurées en jeu.
3. **Son par niveau** : niveaux 0 et 1 démarrent le même effet (`C096` = 5). Niveaux 2 et 3 : aucun début détecté, un seul essai.
4. **Durée et cadence** : mesurées pour les 4 niveaux, un seul coup pour 2 et 3.
5. **Accroupi, niveaux 1 et 2** : non mesuré.
6. **Accroupi à gauche** : le bas attendu serait 16 (6 + 10), non mesuré. Le portage utilise un miroir des frames droites.

## Conséquence pour le portage

- Le MD ne porte que le triplet A (niveau 0). C'est cohérent avec ce qui est observé en jeu normal, mais il manque les triplets B et C si le jeu les utilise.
- Une éventuelle différence de hitbox ou de son par niveau reste à mesurer avant de porter ces variantes.
