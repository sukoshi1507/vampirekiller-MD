# Ennemis et plateformes mobiles - etat de la recherche

Statut : **recherche partielle**. Aucun ennemi ni aucune plateforme n'a encore
ete observe en execution, et aucune table de spawn ou de trajectoire n'est
authentifiee. Ce document ne decrit donc pas de mecanique finale; il fixe ce
qui est prouve, ce qui est deduit du code, et ce qui bloque la suite.

Dump de reference : SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`.
Adresses CPU sauf mention contraire. Banque 0 fixe en `4000-5FFF`; la fenetre
`6000-7FFF` est commutable (banque 01 pendant la boucle de jeu).

## 1. Prouve (execution openMSX, ou lecture directe de la ROM)

### 1.1 Boucle de frame et sondage

- Le hook VBLANK `4028` est atteint a 60 Hz : 1 frame = 1/60 s (frame 1 a 4,07 s,
  frame 121 a 6,07 s). Source : trace `entities5/entities-live.csv` (3592 lignes).
- Le handler d'etat `414D` dispatche sur `C000` via la table a `4161`
  (14 entrees). Le handler de l'etat 4 (`422C`) appelle `62E6`, la chaine de frame
  de jeu (`423A`). Le handler de l'etat 6 (`4294`) appelle `62D0`.
- Pendant la trace, la machine atteint `C000=3`, puis `4`, puis `5` (`C001=0`),
  l'etat ou `4257` teste `C40C`, `C40A`, `C41B`, `C408`, `C409` et `C413`.

### 1.2 Sprites et objets

- La SAT est a `D600` (32 entrees de 4 octets Y, X, N, couleur). `63C5` la
  remplit de `E0` (sprite masque) a chaque frame. Dans la trace, les entrees
  masquees valent bien `E0`.
- Dans la premiere salle, la trace n'a montre que 4 sprites (Simon, Y ~ `90`/`A0`,
  X ~ `08`) apres l'etat 5. **Aucun ennemi n'est apparu** pendant environ 60 s
  d'emulation, avec les entrees testees ci-dessous.
- Les frames `500-850` (`C000=3`, `C001=4`) affichent 14 sprites a `Y=39`, dont
  `X` varie de `B8` a `AF` avec des numeros `40/44/50/54`. Ce motif est un
  mouvement continu. Il est **inconnu** s'il s'agit d'un ennemi, d'une
  animation d'intro ou d'un element de l'ecran titre (voir section 4).

### 1.3 Entrees clavier (lecture, pas la correspondance des boutons)

- `4B9D` lit la matrice via le BIOS (`SNSMAT`, `0141` en page 0, wrapper `0093`).
  Ligne 8 : `4BCF`. Ligne 6 : `4BF4`.
- `C007` (etat de la ligne 8 via `4BBB`) change lorsque les bits impairs de la
  ligne 8 sont presses (valeur `0x10`). Preuve par la trace.
- `C00C` (etat de la ligne 6 via `4BF4`) n'a **jamais** change pendant la trace,
  malgre les pressions des bits 0 a 7 de la ligne 6. `C00B` (front montant
  de `C00C`) reste donc a 0.
- Simon ne se deplace pas. La routine `5C25` (appelee par `4257`) ne passe a la
  suite que si `C00B` bit 0 vaut 1 (`5C41`: `LD A,(C00B); RRA; JR NC`). Sans
  front sur ce bit, le controle de Simon n'est pas active.

### 1.4 Chemin de demonstration (sans entree)

- `4B9D` ne lit l'entree que si `C002 & 0x40` est non nul. Sinon `4E2E` appelle
  une sequence pilotee par `CF3A` (compteur) et `CF3B` (indice). Si `CF3B != FF`,
  la routine `4BB1` alimente `C007` avec ces valeurs. Ce chemin pourrait servir
  de scenario reproductible, mais son contenu n'a pas encore ete lu.

## 2. Deduit du code (non verifie en execution)

- **Chaine de frame** `62E6..6336` (banque 01) : `564C`, `570D`, `63B7`, `6402`,
  `5A49`, `63C5`, `6382`, `5780`, `56E1`, `4F83`, `47D4`, `6333`, `4F91`, `91BE`,
  `8671`, `9147`, `8EE6`, `47C7`, `9CA9`, `6154`, `61BB`. Les candidats objets
  sont `63B7` (efface `C420`, 0x4F octets), `6382` (efface `C470`, 0x28F octets,
  et appelle `5B1B`, `5A40`, `902D`, `909B`, `5A37`, `7513`), et `5A49`.
- **`5A49` (decodage d'un flux)** : efface `E000..E47F`, puis lit un flux a
  partir de `5A56`. Le code traite des octets de commande `00` (fin), `01`, `02`
  (appel `4066` avec `A=18`) et copie des paires d'octets jusqu'a `7F`. Ce
  flux pourrait etre la liste d'objets d'une salle, mais le format n'est pas prouve.
- **`6402`** : table indexee par `D000` (stage) a `641F`, qui ecrit `C425`,
  `C427` et `C42C`. Il s'agit probablement de la position initiale de Simon.
- **Adresses RAM candidates** : `C420-C46F` (objets ?), `C470-C5FF` (efface par `6382`),
  `E000-E47F` (flux decode par `5A49`), `D600-D67F` (SAT, prouve), `C800`
  et `D700` (effaces par `6382`).

## 3. Inconnu

- Format de la table de spawn par salle (ennemis, quantite, position, type).
- Table de comportement ou de trajectoire d'un ennemi. Vitesse en px/frame.
- Cadence d'apparition, degats, collisions avec Simon (boite de collision).
- Ascenseurs, plateformes mobiles : aucune trace, aucune routine identifiee.
- Correspondance des touches de la ligne 6 avec les boutons du jeu.
- Contenu de la table `CF3B` (demonstration) et son role dans le jeu normal.

## 4. Blocages et prochaines etapes

1. **Authentifier les entrees** : comprendre pourquoi `C00C` reste a 0 malgre
   la matrice ligne 6. Hypotheses : le `CPL/RLCA/AND 7` de `4BF4` ne lit pas les
   bits attendus, ou la ligne 6 est fausse dans le mapping openMSX. Methode :
   tracer les valeurs de `A` en sortie de `4BF4`, ou utiliser le chemin de
   demonstration `4E2E` (`CF3B`).
2. **Trace d'entree reelle** : une fois Simon controlable, rejouer une marche
   vers la droite, en enregistrant `E000-E47F`, `C420-C5FF` et `D600-D67F` a
   chaque frame, puis identifier le moment ou un ennemi apparait.
3. **Plateformes** : chercher les routines qui ecrivent `C425` ou `C427` hors de
   la boucle de Simon, et les ecritures dans `C470-C5FF` qui se repetent a
   chaque frame.

## 5. Plan d'implementation SGDK minimal (a faire ensuite, rien n'est modifie)

Objectif : un ennemi simple (aller-retour, sans degats au debut) et une
plateforme mobile (translation horizontale), une fois les donnees ROM prouvees.

Fichiers a creer (aucun fichier existant n'est modifie) :

- `tools/extract_entities.py` : lit la ROM via `tools/probe.py` (`reference_rom`),
  extrait la table de spawn et le format de comportement une fois prouves.
- `sgdk/src/enemies.h` et `sgdk/src/enemies.c` : structure d'un ennemi
  (position, vitesse, borne gauche/droite, sprite), `enemies_init`,
  `enemies_update` (aller-retour), `enemies_draw` (SPR_setPosition).
- `sgdk/src/platforms.h` et `sgdk/src/platforms.c` : plateforme mobile
  (position, vitesse, bornes), `platforms_update`, et une fonction de collision
  `platforms_check_standing` qui renvoie la hauteur de sol.

Points d'integration (a faire par le responsable, pas par cette recherche) :
`sgdk/src/main.c` ou `sgdk/src/gameplay.c` appelleront `enemies_update` et
`platforms_update`. Ces deux fichiers sont interdits dans cette tache :
`main.c`, `player.c`, `whip.c`, `gameplay.c`.

## 6. Outils et traces produits

- `tools/trace_entities_live.tcl` : trace openMSX en lecture seule (VBLANK
  `4028`), journalise `C000/C001/C003-C005`, `C40A/C40C/C410/C415/C418`,
  `C425/C427`, `C002/C006/C007/C00B/C00C`, `C420-C44F`, `D600-D67F`, `C470-C4FF`.
  Requiert `vk_output` et `vk_seconds` definis par le lanceur. Les entrees
  clavier de test ne sont pas dans ce fichier.
- Lanceur et calendrier d'entrees (hors depot) :
  `/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad/run_entities5.sh`
  et `.../scratchpad/inputs2.tcl`.
- Traces : `.../scratchpad/entities5/entities-live.csv`, et
  `.../scratchpad/entities4/entities-live.csv`.
- Desassemblage de travail : `.../scratchpad/dis.py` (z80dis, banque/fenetre
  explicites).
- Aucun fichier genere existant n'a ete modifie. Rien n'a ete commite.
