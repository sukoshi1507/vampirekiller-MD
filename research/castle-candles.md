# Bougies du chateau : positions et table de placement

Statut : **partiel mais solide pour les positions**. Les bougies (objets fixes qui lachent un objet au fouet) sont decodees pour les 156 salles (cour 3 + chateau 153) par execution de la routine ROM de chargement. Quatre salles sont verifiees en jeu (cour 0 et 1, hall 0 et 1). La regle de touche au fouet, le format des enregistrements et la table des objets laches sont lus dans le code (section 9, 2026-10-28).

ROM de reference : `Vampire Killer (Japan, Europe) (En).zip`, SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2` (verifie). Adresses CPU Z80 ; offset fichier = CPU pour la fenetre fixe 4000-5FFF (banque 0), sinon banque × 0x2000 + (CPU & 0x1FFF). Preuves : **prouve** (trace openMSX ou execution du code ROM), **deduit** (lecture statique), **inconnu**.

## 1. Methode

1. Trace des ecritures sur les objets (watchpoints openMSX, PC et pile) pendant le scenario cour (`scratchpad/castle-candles/work/pc.tcl`) : ecriture de C470 et C480 au chargement, puis mise a jour de type par la routine `868C`.
2. Lecture statique du chargement de salle (`638x`, `5B1B`, `5BCF`, `5A98`, `5E6A`, `5C04`) et des routines de coup (`7116`, `898F`, `89BF`).
3. Harnais CPU + mapper (meme principe que `tools/verify_z80_maps.py`) : `tools/castle_room_objects.py` execute `5A49` puis `5B1B` pour chaque salle de `assets/manifest.json` et pour les trois salles de la cour. Sortie : 156 salles (`scratchpad/castle-candles/work/tool.json`, non versionne).
4. Verification en jeu : marche droite continue du titre jusqu'au hall 1 (`scratchpad/castle-candles/work/verify_rooms.tcl`), dump de C420-C5FF 120 trames apres l'entree de chaque salle (`scratchpad/castle-candles/verify/`).

## 2. Table de placement et chargement

| Point | Statut | Detail |
|---|---|---|
| Chargement d'une salle efface C470-C6FF puis appelle `5B1B` | **prouve** (code) | `638A`-`638D` : `LDIR` depuis `C470` (0x28F octets + 1), puis `CALL 5B1B` en `638F`. |
| `5B1B` remplit les objets de la salle | **prouve** (code + harnais + jeu) | Lit une cellule de 24 octets a `E000 + decalage(stage) + salle*24`, `decalage(stage)` = table `5C04` x 16. Ecrit jusqu'a 8 emplacements `C470`, `C480`, ..., `C4F0` (0x10 octets chacun). |
| Le flux compresse est decode en `E000-E47F` par `5A49` | **prouve** (code ; format des octets repris de `enemies-platforms.md` §1.4, relu dans le code `5A62`-`5A93`) | `5A49` efface E000, puis `5A5C` decode. `5AAF` construit ensuite une table annexe en `DE00`. Banques 14 (`8000`) et 15 (`A000`) via `5350`. |
| Flux de la cour (stage 0) | **prouve** (code) | `5A98` : `HL = 800C` (banque 14) si `D000=0`. |
| Flux des stages 1 a 18 | **prouve** (code) | `5A98` : `HL = mot a 8000 + 2*D002`. `D002 = table ROM 5E6A[stage]` (fichier offset `1E6A`) : `0,0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5` pour stages 0 a 18. |
| Entrees d'objet : type `01` au chargement, Y en `+1`, X en `+2` | **prouve** pour 3 salles (jeu) | Cour salle 1 : `C480` = `01 90 C0 ...` au chargement, puis `00 90 C0` apres le coup (torche B). Le sprite de la flamme est a X=`C0`, Y=`91` (`research/candles-and-drops.md`). |
| Type `01` passe a `02` a chaque trame | **prouve** (jeu) | Routine `868C` (banque 2) : `LD (HL),2`. Trace : `C470` = `02` en jeu. |
| Octet `+5` (noté k) | **inconnu** | Valeurs 00 a 1F. Non interprete. Torches cour : `02` pour les deux, les deux en salle 1. |

Limite : 8 emplacements par salle (`B = 8`). Aucune salle decodee n'en depasse 8.

## 3. Mecanisme du coup (partiel)

| Point | Statut | Detail |
|---|---|---|
| Le fouet pose un drapeau `C422` | **prouve** (trace, `candles-and-drops.md`) | Routine `7116`-`7135` (banque 1). `7133` ecrit `C422 = 1`. |
| Les objets type 2 sont testes par `868C` -> `87BA` -> `898F` | **prouve** (chaine d'appels, PC des ecritures) | `868C` boucle sur les objets ; `898F` (banque 2) appelle `89BF`. |
| Un coup cree la flamme `C800` (type `26`) | **prouve** (PC `5F82` dans `5F1D`, appele depuis `89D7`) | `89BF` : `E += 10`, `D += 8`, `C = 26` (ou `1E` si `B = 1`), puis appel de `5F1D`. |
| Un coup cree un objet `C500` (drop, type `83`) | **prouve** (trace, PC `89F0`) | Meme cas que la torche B (`candles-and-drops.md`). |
| Routine de test de touche (fouet contre objet) | **inconnu** | `884A`, `8874` et `877C` ne testent rien : ce sont des helpers d'affichage (ecriture de zone et mise a 0 du pointeur `(IX+7)`). Le test reel n'est pas identifie. |

## 4. Tableau par salle

Bougies : coordonnees (X,Y) en hexadecimal, comme stockees dans l'objet (`+2`, `+1`). Le sprite est affiche a Y+1. Objets C500 type 83 : objets fixes preplaces, probablement des coeurs (meme type que le coeur observe apres la torche B, non verifie).

Confiance :
- **elevee** : position verifiee en jeu sur la salle.
- **moyenne** : decodee par le harnais, non verifiee en jeu.

Touchable : seule la torche B de la cour salle 1 a ete testee.

## Tableau

| Stage | Salle | Bougies (X,Y) en hex | Objets C500 type 83 (X,Y), prob. coeurs | Touchable | Confiance |
|---|---|---|---|---|---|
| 0 (cour) | 0 | (40,90), (C0,90) | - | inconnu (non teste) | elevee : position verifiee en jeu |
| 0 (cour) | 1 | (40,90), (C0,90) | - | Torche B (X=C0) : oui, 1 essai (fouet a dx 24). Torche A (X=40) : non, 2 essais (dx 24 et 28) | elevee : position verifiee en jeu |
| 0 (cour) | 2 | (40,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 1 (chateau 1) | 0 | (70,90), (B0,90) | - | inconnu (non teste) | elevee : position verifiee en jeu |
| 1 (chateau 1) | 1 | (90,40), (B0,80), (30,90), (70,90) | - | inconnu (non teste). La bougie (B0,80) est absente en jeu, voir Notes | elevee pour 3 bougies sur 4 ; 1 absente en jeu |
| 1 (chateau 1) | 2 | (60,40), (B0,40), (D0,90), (80,A0) | (40,B0), (B0,B0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 1 (chateau 1) | 3 | (B0,50), (40,60), (20,A0), (80,A0) | (30,70) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 1 (chateau 1) | 4 | (30,20), (B0,40), (30,60), (60,90), (A0,90) | (20,30), (20,70), (20,B0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 1 (chateau 1) | 5 | (90,80), (40,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 1 (chateau 1) | 6 | (D0,50), (70,80) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 1 (chateau 1) | 7 | (60,A0), (A0,A0), (C0,A0) | (80,70) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 2 (chateau 2) | 0 | (80,40), (A0,60), (C0,60), (20,90), (80,A0), (A0,A0), (C0,A0) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 2 (chateau 2) | 1 | (70,30), (20,60), (40,60), (60,60), (20,A0), (40,A0), (60,A0) | (10,30) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 2 (chateau 2) | 2 | (40,30), (E0,30), (A0,40), (50,70), (A0,70) | (60,50) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 2 (chateau 2) | 3 | (60,30), (C0,30), (40,70), (E0,80) | (80,B0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 2 (chateau 2) | 4 | (E0,40), (20,50), (40,50) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 2 (chateau 2) | 5 | (50,40), (C0,40), (C0,70) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 3 (chateau 3) | 0 | (80,60), (30,90), (80,90) | (20,B0), (40,70) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 3 (chateau 3) | 1 | (D0,50), (70,90) | (C0,90) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 3 (chateau 3) | 2 | (A0,60), (10,A0), (A0,A0) | (70,70), (70,B0), (C0,B0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 3 (chateau 3) | 3 | (30,50), (30,90), (60,90), (90,90), (C0,90) | (C0,50) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 3 (chateau 3) | 4 | aucune | (30,50), (30,B0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 3 (chateau 3) | 5 | (70,80), (A0,80) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 4 (chateau 4) | 0 | (20,40), (80,40), (80,90), (E0,90) | (D0,50) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 4 (chateau 4) | 1 | (40,30), (C0,30), (40,80), (90,80) | (C0,70), (90,B0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 4 (chateau 4) | 2 | (50,30), (C0,40), (30,80), (D0,A0) | (D0,70) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 4 (chateau 4) | 3 | (40,30), (A0,30) | (10,B0), (70,90) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 4 (chateau 4) | 4 | (10,30), (70,30), (B0,30) | (B0,B0), (20,70), (A0,70) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 4 (chateau 4) | 5 | (60,30), (D0,70) | (10,90) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 5 (chateau 5) | 0 | (40,20), (80,20), (C0,20) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 5 (chateau 5) | 1 | (70,40), (B0,40) | (60,70), (A0,90) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 5 (chateau 5) | 2 | (20,20), (B0,30) | (A0,70) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 5 (chateau 5) | 3 | (C0,40), (20,80), (C0,80) | (B0,50), (B0,90) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 5 (chateau 5) | 4 | (40,40), (80,40), (D0,40), (30,80), (80,80) | (50,90), (C0,90) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 5 (chateau 5) | 5 | (50,30), (D0,30), (40,90), (90,90), (C0,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 6 (chateau 6) | 0 | (C0,20), (40,50), (40,A0), (60,A0), (80,A0), (A0,A0), (C0,A0) | (70,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 6 (chateau 6) | 1 | (20,20), (60,20), (A0,20), (20,80), (60,80), (A0,80) | (C0,90) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 6 (chateau 6) | 2 | (20,30), (80,30), (30,70), (B0,70), (80,80) | (40,40), (20,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 6 (chateau 6) | 3 | (20,60), (60,60) | (D0,90) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 6 (chateau 6) | 4 | (20,60), (60,60), (A0,60), (E0,60) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 6 (chateau 6) | 5 | (20,60), (E0,60), (60,70), (A0,70) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 7 (chateau 7) | 0 | (A0,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 7 (chateau 7) | 1 | (30,30), (60,50), (A0,50), (20,90), (E0,90) | (40,80), (E0,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 7 (chateau 7) | 2 | (E0,30), (60,50), (40,60), (90,90) | (20,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 7 (chateau 7) | 3 | (C0,40), (10,50), (C0,80) | (D0,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 7 (chateau 7) | 4 | (40,20), (C0,20), (10,80), (40,90) | (40,40), (A0,40), (A0,70) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 7 (chateau 7) | 5 | (90,30), (50,50), (80,90) | (20,80), (B0,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 7 (chateau 7) | 6 | (40,40), (40,90), (80,90) | (B0,90) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 7 (chateau 7) | 7 | (10,90), (40,90), (80,90) | (E0,70) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 7 (chateau 7) | 8 | (90,50), (C0,80), (40,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 8 (chateau 8) | 0 | (60,50), (80,50), (A0,50), (50,70), (60,70), (80,70), (A0,70), (C0,70) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 8 (chateau 8) | 1 | (50,60), (A0,60), (C0,60), (40,70), (60,70), (80,70), (C0,70) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 8 (chateau 8) | 2 | (B0,20), (40,60), (80,60) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 8 (chateau 8) | 3 | (70,30), (70,70), (E0,70) | (20,80), (A0,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 8 (chateau 8) | 4 | (60,20), (A0,20), (40,80) | (10,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 8 (chateau 8) | 5 | (40,50), (90,50), (A0,50), (B0,B0) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 8 (chateau 8) | 6 | aucune | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 8 (chateau 8) | 7 | (C0,40), (20,70), (50,70), (C0,80), (C0,A0) | (20,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 9 (chateau 9) | 0 | aucune | (80,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 9 (chateau 9) | 1 | (E0,50) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 9 (chateau 9) | 2 | (30,40), (60,40), (90,40), (C0,40) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 9 (chateau 9) | 3 | aucune | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 9 (chateau 9) | 4 | (C0,30), (60,40), (90,40) | (E0,A0), (B0,60), (E0,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 9 (chateau 9) | 5 | (10,90), (70,90) | (A0,60), (90,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 9 (chateau 9) | 6 | (40,30), (B0,30), (40,A0) | (20,40), (90,80) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 9 (chateau 9) | 7 | (20,60), (60,90), (90,90), (C0,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 9 (chateau 9) | 8 | (90,30), (90,80), (C0,90) | (D0,60), (70,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 10 (chateau 10) | 0 | (50,60), (A0,70) | (30,30), (E0,80) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 10 (chateau 10) | 1 | (80,30), (E0,30), (50,60) | (90,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 10 (chateau 10) | 2 | (20,60), (D0,70) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 10 (chateau 10) | 3 | (60,70), (A0,70), (D0,70) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 10 (chateau 10) | 4 | (70,70), (D0,70) | (40,80) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 10 (chateau 10) | 5 | (50,30), (D0,30), (D0,70) | (E0,80), (60,40), (70,80) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 10 (chateau 10) | 6 | (20,80), (60,80), (A0,80), (C0,80) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 10 (chateau 10) | 7 | (40,80), (60,80), (90,80), (20,90), (B0,90), (E0,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 10 (chateau 10) | 8 | (40,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 11 (chateau 11) | 0 | (70,90), (A0,90), (D0,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 11 (chateau 11) | 1 | (60,50), (60,70), (80,70), (A0,70), (C0,70), (40,90), (60,90) | (80,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 11 (chateau 11) | 2 | (20,70), (50,70), (D0,80) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 11 (chateau 11) | 3 | (60,50), (80,70), (C0,70), (40,90), (60,90), (80,90), (A0,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 11 (chateau 11) | 4 | (B0,80), (E0,80), (80,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 11 (chateau 11) | 5 | (40,90), (70,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 12 (chateau 12) | 0 | (30,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 12 (chateau 12) | 1 | (40,70), (90,80), (E0,80) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 12 (chateau 12) | 2 | (20,80), (70,90), (C0,90) | (E0,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 12 (chateau 12) | 3 | (30,90), (80,90), (B0,90), (E0,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 12 (chateau 12) | 4 | (E0,80) | (60,80) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 12 (chateau 12) | 5 | (C0,60), (20,90), (80,90), (A0,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 12 (chateau 12) | 6 | aucune | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 12 (chateau 12) | 7 | (A0,70), (E0,70), (10,90) | (C0,80) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 12 (chateau 12) | 8 | (20,70), (90,70), (C0,90) | (E0,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 12 (chateau 12) | 9 | (20,70), (40,70), (70,70), (A0,70) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 12 (chateau 12) | 10 | (C0,80), (10,90), (60,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 12 (chateau 12) | 11 | (70,90) | (40,80) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 13 (chateau 13) | 0 | (20,30), (30,90), (80,90), (E0,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 13 (chateau 13) | 1 | (C0,80), (40,90) | (E0,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 13 (chateau 13) | 2 | (70,80), (C0,80) | (90,40), (D0,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 13 (chateau 13) | 3 | (10,30), (60,30), (B0,30) | (90,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 13 (chateau 13) | 4 | (50,30), (A0,30), (40,90), (80,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 13 (chateau 13) | 5 | (40,30), (90,30), (E0,30) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 13 (chateau 13) | 6 | (C0,30), (A0,70) | (90,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 13 (chateau 13) | 7 | (30,80), (90,80), (C0,80) | (B0,40), (20,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 13 (chateau 13) | 8 | (80,20), (B0,20), (E0,20), (20,30), (20,50) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 13 (chateau 13) | 9 | (B0,50), (E0,50) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 13 (chateau 13) | 10 | (90,50), (C0,50), (C0,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 13 (chateau 13) | 11 | (20,30) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 14 (chateau 14) | 0 | (30,20), (30,80) | (D0,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 14 (chateau 14) | 1 | (40,90), (80,90), (C0,90) | (D0,60), (40,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 14 (chateau 14) | 2 | (40,60), (C0,80) | (B0,40), (E0,60), (60,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 14 (chateau 14) | 3 | (40,20), (90,20) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 14 (chateau 14) | 4 | (40,20), (D0,20), (80,70) | (50,A0), (C0,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 14 (chateau 14) | 5 | (40,40), (40,60) | (B0,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 14 (chateau 14) | 6 | (30,40), (90,40), (C0,40) | (30,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 14 (chateau 14) | 7 | (70,30), (A0,30), (A0,80), (30,90), (50,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 15 (chateau 15) | 0 | (50,30), (C0,30), (40,90) | (B0,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 15 (chateau 15) | 1 | (B0,30), (D0,80), (60,90), (A0,90) | (40,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 15 (chateau 15) | 2 | (C0,30), (10,80), (50,80), (90,80), (D0,80) | (B0,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 15 (chateau 15) | 3 | (90,80), (D0,80) | (20,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 15 (chateau 15) | 4 | aucune | (B0,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 15 (chateau 15) | 5 | (20,40), (A0,40) | (20,60), (B0,60), (C0,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 15 (chateau 15) | 6 | (70,80), (C0,80) | (90,A0), (D0,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 15 (chateau 15) | 7 | aucune | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 15 (chateau 15) | 8 | aucune | (40,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 15 (chateau 15) | 9 | (20,60), (D0,60), (60,70), (90,70) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 16 (chateau 16) | 0 | (80,60), (60,90), (B0,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 16 (chateau 16) | 1 | aucune | (C0,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 16 (chateau 16) | 2 | aucune | (20,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 16 (chateau 16) | 3 | aucune | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 16 (chateau 16) | 4 | aucune | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 16 (chateau 16) | 5 | (60,50) | (90,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 16 (chateau 16) | 6 | (10,20), (60,30), (40,90), (80,90) | (60,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 16 (chateau 16) | 7 | aucune | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 16 (chateau 16) | 8 | aucune | (C0,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 16 (chateau 16) | 9 | (70,50), (90,50) | (60,60), (80,60), (D0,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 17 (chateau 17) | 0 | (50,20), (80,50), (10,90), (80,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 17 (chateau 17) | 1 | (20,30), (D0,30), (80,50), (80,70), (80,90) | (70,A0), (10,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 17 (chateau 17) | 2 | (D0,40), (40,90), (60,90), (D0,90) | (20,50), (80,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 17 (chateau 17) | 3 | (70,40), (C0,40), (40,90), (C0,90) | (D0,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 17 (chateau 17) | 4 | (10,30), (50,30), (C0,30), (40,80), (C0,80) | (A0,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 17 (chateau 17) | 5 | (80,30), (B0,30), (E0,30), (E0,80) | (20,40), (C0,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 17 (chateau 17) | 6 | (A0,30), (40,80), (00,90), (C0,90) | (D0,40), (80,60), (B0,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 17 (chateau 17) | 7 | aucune | (D0,80), (E0,A0), (E0,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 17 (chateau 17) | 8 | (80,30), (80,70) | (E0,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 17 (chateau 17) | 9 | (80,50), (A0,50), (60,70), (80,70) | (40,40), (90,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 17 (chateau 17) | 10 | aucune | (D0,40), (D0,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 17 (chateau 17) | 11 | (30,40) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 18 (chateau 18) | 0 | aucune | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 18 (chateau 18) | 1 | (60,30), (E0,30) | (40,80) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 18 (chateau 18) | 2 | (50,70), (D0,70) | (10,80), (40,40) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 18 (chateau 18) | 3 | (60,40), (E0,40), (10,90), (60,90), (E0,90) | (10,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 18 (chateau 18) | 4 | (20,20), (90,60), (40,90), (D0,90) | (10,40), (60,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 18 (chateau 18) | 5 | (C0,30) | (80,40), (10,B0), (20,40), (70,A0) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 18 (chateau 18) | 6 | (A0,30), (60,40), (B0,B0) | (10,30) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 18 (chateau 18) | 7 | (40,30), (90,40), (D0,70), (10,90) | (30,A0), (60,60) | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 18 (chateau 18) | 8 | (50,30), (80,30), (B0,30), (40,90), (80,90), (C0,90) | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |
| 18 (chateau 18) | 9 | aucune | - | inconnu (non teste) | moyenne : decode harnais, non verifie en jeu |

## 5. Ecarts et deductions

- **Salle hall 1 (stage 1, salle 1)** : le harnais donne 4 bougies. Le dump en jeu (120 trames apres l'entree, Simon en marche droite) n'en montre que 3. La bougie `(B0,80)` (entree `C4A0`, octet `+5 = 09`) est absente. **Inconnu** : elle a pu etre retiree par le jeu (regle non trouvee) ou le harnais ne reproduit pas un etat initial (C417, D002 ou un drapeau). Les trois autres positions concordent exactement.
- **Salles 0 et 1 de la cour, salle 0 du hall** : positions identiques entre harnais et jeu (**prouve**).
- **Les 8 sprites a X=E0 de la salle 0 de `research/enemies-platforms.md`** : **deduit** de la structure (`+7 = E0` sur les deux entrees de la cour, `+1 = 90` donne Y=8F, deux bougies a 4 sprites). Ce n'est pas prouve.
- **Objets C500 type 83 (147 entrees)** : preplaces par `5B1B`. Ce sont **probablement des coeurs** (meme type que le coeur de la torche B). **Non verifie** (aucun ramassage teste hors cour).

## 6. Touchabilite au fouet

| Cas | Resultat | Essais |
|---|---|---|
| Cour salle 1, torche B (X=C0, Y=90) | **touchee**, objet flamme puis coeur | 1 (Simon X=168, dx = 24 px) |
| Cour salle 1, torche A (X=40, Y=90) | **non touchee**, aucun objet | 2 (Simon X=36, dx = 28 ; X=40, dx = 24) |

Le cas 1 et le cas 2 sont **contradictoires** pour une regle de simple distance : une torche a dx 24 est touchee (B) et l'autre a dx 24 ne l'est pas (A). Les deux ont le meme Y, le meme `+5` (02) et le meme type. **Inconnu** : facteur possible non teste (direction de Simon, phase du fouet `C42F`, niveau `C416`, ou ordre des objets dans la boucle de `868C`).

Pour les autres bougies (y compris toutes les bougies du chateau), la touchabilite est **inconnue**.

## 7. Inconnues

1. Regle exacte de touche au fouet (routine de test non localisee).
2. Signification des octets `+4`, `+5`, `+6`, `+8` et du drop associe a chaque bougie.
3. Identite des objets `C500` type 83 (coeurs ou autres) et leur ramassage (voir `candles-and-drops.md`).
4. Ecart de la salle hall 1 (une bougie absente en jeu).
5. Verification en jeu des salles du hall 2 a 7 et des stages 2 a 18 (le decodage est fait, pas le controle).
6. Salles avec 8 bougies (limite de la boucle `5B1B`) : aucune observee, a confirmer.

## 8. Fichiers

- `tools/castle_room_objects.py` : harnais de decodage (texte, lecture seule de la ROM). Usage : `python3 tools/castle_room_objects.py "<ROM .rom>" sortie.json`.
- `research/castle-candles.md` : ce document.
- Donnees de travail (non versionnees, `scratchpad/castle-candles/`) : `work/*.tcl`, `work/tool.json`, `work/summary.json`, `pc/pc.log` (PC des ecritures), `verify/room-*.bin` (dumps en jeu).

## 9. Code du coup, des objets laches et du ramassage (lecture du code, 2026-10-28)

Desassemblage `tools/disasm_rom_window.py` (banque 1 en 6000, banque 2 en 8000, banque 0 en 4000). **Prouve (code)** sauf mention.

### 9.1 Enregistrement de bougie (`C470`-`C4F0`, 16 octets)

`+0` type (01 au chargement, 02 en jeu), `+1` Y, `+2` X, `+3` compteur de touche (mis a 1 par le fouet), `+4` **mode** (00 = bougie ou torche ; 03 = cache dans le decor, revele par le fouet ; 02 et autres : variantes de `87D2`), `+5` **objet lache**, `+6` ?, `+7` motif (`E0`/`E1`), `+7`-`+9` pour le mode 3 : contenu du bloc (`8814`).

Exemple : hall salle 2, `01 a0 80 00 03 18` = bloc cache en (80,A0) contenant **la cle blanche** (0x18). Sur le MSX, la cle n'apparait qu'apres un coup de fouet sur ce point du mur (coherent avec le contre-test K4, `door-room7.md`).

### 9.2 Touche au fouet (`7FB7` banque 1, test `8450` -> `84A7`/`84D9`)

Seulement en phase de fouet `C422 = 3`. Pour chaque bougie non vide : centre `D = X + 8`, `E = Y + 16`. X du fouet = `C427 + 20` (regard a droite, `C42C = 0`) ou `C427 - 20` ; 24 si `C416 = 1`. Touche si `|X fouet - D| < 20` (`L = 8 + 12`, ou `8 + 16` si `C416 = 1`) et `0 <= C425 - B - E + 16 < 20` avec `B = 18` debout, `10` accroupi (`C420 = 2`), soit `C425 - B - Y` dans [0, 20). Effet : `+3` += 1, son `0C`.

Note : la regle donne la torche A de la cour touchee a Simon X 36 et 40 regard a droite ; la mesure (section 6) dit non. A verifier (direction de Simon pendant ces essais, phase `C422`).

### 9.3 Bougie touchee (`868C` -> `87BA` -> `898F`)

La bougie est effacee (`+0 = 0`). Mode < 2 : effacement de l'affichage (`884A`, `8874`) puis `898F` avec B = objet. Si B = 0 ou 0x15 : pas de flamme. Sinon `89BF` cree la flamme dans `C800` (type `26`, ou `1E` si B = 1) en (E + 16, D + 8). Puis `8A37` cree l'objet lache dans `C500` : type `01`, Y, X, `+4` = B, `+5` = 2 (sauf B < 2 ou 0x15 : 0). Il devient `83` quand la flamme disparait (`candles-and-drops.md` : 45 images). Mode 3 : son `0E`, revelation du decor (`885E`/`8899` ou `884A`/`887D`) ; objet 0x18 : `883E` ; 0x1F : contenu du bloc en `+7`-`+9` (`8814`).

### 9.4 Ramassage (`7FE2` banque 1, test `8230` -> `847A`)

Hors fouet (`C422 = 0`), pour les objets `C500` de type >= `80` : centre `D = X + 8`, `E = Y + 16` ; ramasse si `|C427 - D| < 13` et `0 <= C425 - 2 - E + 16 < 42` (debout ; `C425 - 8` si `C420 = 1` ; 34 si `C420` vaut 1 ou 2). Effet : `+3` += 1, puis `8D30` avec A = `+4`.

### 9.5 Table des objets (`8D30`, sauts en `8D3E`, index = objet - 1)

| Objet | Routine | Effet |
|---|---|---|
| 00 | - | rien (pas de flamme) |
| 01 | `8DAF` | coeurs + 1 (`4594` : `C417` += 1 en BCD, max 99) ; son `0F` |
| 02 | `8DB7` | coeurs + 5 (mesure : torche B de la cour) |
| 03 | `8D98` | `C701` : bit 5 efface, bit 4 mis ; `C441` = 10h (armure, voir 9.8) |
| 04 | `8DA1` | `C701` : bit 4 efface, bit 5 mis ; `C441` = 10h (projectiles `D700` detruits au contact, `8610`) |
| 05 | `8DBB` | `7806` (tous les objets `C800` et `D700` detruits, sans points), `928D`, `C43E` = 18h (eclair de la couleur de bordure) ; son `1B` |
| 06 | `8D7C` | minuterie `C440` = 96h (F0h si bit 2 de `C431`) : apparitions arretees (`5EB8`) |
| 07 | `8DDE` | **energie** + 8 (`4605` : `C415` += B, max 20h) ; ce ne sont pas des points |
| 08 | `8DCD` | minuterie `C43A` = 96h / F0h : aucun contact ; son `16` |
| 09 | `8DE5` | minuterie `C434` = 96h / F0h : le contact detruit l'ennemi |
| 0A | `8D94` | `C701` bit 6 |
| 0B, 0C, 0D | `8DF5`, `8DFC`, `8E03` | `C431` bits 2, 3, 4 |
| 0E | `8E16` | `870C`, `C702` bit 0 |
| 0F, 10 | `8E33`, `8E1D` | `C431` bit 6, `C70F` = 3 et `C701` bit 7 ; `C702` bit 6 (bit 7 efface) |
| 11 | `8E26` | `C702` bit 7 (bit 6 efface) |
| 12 | `8E79` | `C431` bit 1, `C701` bit 2, `C700` = 3 |
| 13, 14 | `8E42`, `8E4B` | points (`44EC`, 5000 et 1000) |
| 15 | `8E14` | rien (pas de flamme) |
| 16 | `8E0D` | **energie** + 20h (`4605`) |
| 17 | `8E50` | **cle doree** : `C701` bit 1, `C700` = 1 |
| 18 | `8E60` | **cle blanche** : `C701` bit 0 (prouve, `door-room7.md`) |
| 19 | `8E14` | rien ici (coffre) |
| 1A a 1D | `8D70` | **niveau du fouet** `C416` = objet - 0x19 (1 a 4), puis `8E9A` (graphismes du fouet) et minuterie `C440` |
| 1E | `8D8D` | `C701` bit 3 |

Les noms des objets (croix, sablier, etc.) et leurs dessins ne sont pas releves : seuls les effets en RAM sont lus. Repartition des objets dans les 457 bougies : 01 (129), 02 (100), 15 (52), 1F (39), 00 (35), 09 (14), 18 (12), 1A (12), 10 (11), 17 (10), 08 (10), 07 (7), 06 (6), 14 (6), 05 (5), 0C (4), 0D (3), 11 (1), 1C (1).

### 9.6 Vie d'un objet lache (`8A4A`, etats `8A8D`), coffre et cles (lecture du code)

**Prouve (code)** :
- **Arme deja tenue** (`8A29`) : un objet 1A a 1D egal a `C416 + 0x19` sort comme petit coeur (01).
- **Etat 1** (`8A99`) : 16 images (`+6 & 15`), puis type 82 ; objet 00 : retire ; 0x15 : cree un ennemi (`5F1D`, type pris dans `8AF0[D002]`) et se retire.
- **Etat 2, chute** (`8AF8`) : pose si la tuile sous (X + 8, Y + 16) vaut 1 a 9, ou si Y + 16 > DB. Sinon Y += 8 par image ; le petit coeur (`8B22`) descend d'un pixel une image sur deux, X en 8.8 avance de `+9/+10` (depart +2,0), vitesse diminuee de `+11/+12` (0,125) a chaque image, signe inverse toutes les 32 images.
- **Etat 3** (`8B70`) : X et Y arrondis a 8 pres, dessin dans le decor (`8CC1`) ; `+6` = 8, ou FF pour 17, 18, 19.
- **Etat 4** (`8BA2`) : au sol ; `+6` diminue toutes les 16 images, l'objet disparait a 0 (128 images), jamais pour les cles et le coffre.
- **Sprites pendant la chute** (`8C60`) : `+5` = 1, motif E8 (petit coeur) ; `+5` = 2, motif EC anime (orbe).
- **Coffre** (`8F68`, `8C14`) : ramasse seulement si `C700` (cles dorees) > 0 ; `C700` -= 1, son 0x11, puis le coffre devient l'objet de son octet `+13`, 8 px plus haut, et retombe (etat 5). Si `C700` = 0 apres : `C701 &= F9` et HUD (`8EC9`). Contenus : hall salle 2 = 02 (gros coeur), hall salle 3 = 0D.
- **Cles** (`8F68`) : 17 et 18 ne sont pas reprises si le bit de `C701` est deja mis. Apres ramassage, `+14/+15` pointe un drapeau du flux de stage remis a 0 : l'objet fixe ne revient pas.
- **Decor sous les bougies** (`873A`) : a la creation, les tuiles de la carte sous la bougie sont sauvees en `E480 + 4 * slot` (2x2) ou `E4A0 + 16 * slot` (4x4, mode 3). Au coup, `884A` / `885E` les redessinent ; pour un bloc (mode 2 ou 3), `887D` / `8899` les recopient aussi dans la carte de collision.
- **Planche de la page 1** : 0x21 et 0x22 sont les deux images de la bougie murale ; les chiffres du HUD sont en y 320, x 0 a 79.

**Port** (`sgdk/src/candles.c`, `candle_view.c`, `tools/build_candles_md.py`) :
- touche, chute, pose, delai, ramassage et effets 01, 02, 17, 18 (bloc cache de la salle 2), 19, 1A ;
- les bougies et objets sont des sprites PAL0 aux coordonnees MSX ; compteur de coeurs en colonnes 24-25 de la ligne 0 ;
- blocs caches (mode 3, `86CA` / `873A` / `885E` / `8899`) : le bloc de briques 4x4 (table `8792` + 4, D002 = 0) est ecrit dans une copie RAM de la carte de collision de la salle (`vk_game_room_block`) et dessine sur BG_A ; au coup de fouet, les tuiles d'origine reviennent dans la carte et a l'ecran. L'homme-poisson de la salle 4 du stage 2 se pose dessus en (48, 80) comme en ROM (scenario `l`).
- **Moment de la pose** (prouve, code et trace) : un changement de salle passe par l'etat 9 (`4377`). Tant que `5A2E` (`B963`) ne rend pas la retenue, l'etat 9 se repete ; ensuite `62F5` charge la salle et appelle `8671` (en `6310`), qui cree les bougies (type 01 → 02, `868C`) et ecrit les blocs (`873A`, `86CA`) ; puis `C000 := 5`. Le bloc est donc present des la premiere image de jeu dans la salle. Trace `scratchpad/fish/out/P2` : `D100` sans bloc en 6123 (etat 9, de 6115 a 6135), bloc present en 6139 (etat 5 depuis 6136). Les « 16 images » notees plus tot n'etaient que l'ecart entre deux instantanes. Le port pose le bloc a l'entree, ce qui est juste par rapport au jeu.
- **Ecart restant, hors bougies** (**porté** : gel de l'etat 9 dans `sgdk/src/room_freeze.c` et `main.c`, `research/room-links.md` section 9) : le port n'avait pas l'etat 9. Sur MSX, Simon et les objets restent figes de 18 a 23 images a chaque changement de salle. La duree est la meme d'une trace a l'autre pour une meme salle d'arrivee (stage 0 : salles 1 et 2 → 18 ; stage 1 : 0 → 19, 1 → 20, 2 et 3 → 22, 6 → 19, 7 → 23 ; stage 2 : 4 et 5 → 21 ; traces `fish`, `type5`, `stage2e`, `drops`).

**PROVISOIRE** :
- la flamme `C800` est remplacee par les etincelles de l'effet de flamme, et l'orbe EC par l'objet 07 de la planche ;
- la cadence de la bougie murale (8 images) n'est pas mesuree ;
- objets 03-16 : voir 9.8 (03, 05-09, 0C, 0D portes ; 0A, 0F, 10, 11 sans effet) ;
- 1B a 1D (dague, hache, croix) ne changent pas l'arme ;
- les bougies restent eteintes jusqu'au changement de stage (le rechargement du flux de stage n'est pas trace) ;
- (fait) icone de la cle doree : voir 9.7.

### 9.7 Cle doree et coffre : inventaire et port (lecture du code, 2026-10-29)

**Deduit (code)**, banque 2 en 8000 (`tools/disasm_rom_window.py ROM 4000 ... 8000`), banque 0 en 4000, banque 1 en 6000. Aucune trace openMSX dediee : les comportements MSX ci-dessous viennent de la lecture du code ; seul le port est verifie (tests natifs et scenario MD `p`).

**Ou** (decodage `5A49`/`5B1B`, `tools/castle_room_objects.py`) :

| Stage | Salle | Objet | Origine |
|---|---|---|---|
| hall (1) | 2 | cle (64,176), coffre (176,176) contenu 02 | objets fixes (type 83) |
| hall | 3 | coffre (48,112) contenu 0D | fixe |
| hall | 4 | coffres (32,48) 0C et (32,112) 03, cle (32,176) | fixes |
| hall | 7 | cle (128,112) | fixe |
| stage 2 | 0 | cle dans le bloc (128,160) | bloc cache, objet 17 |
| stage 2 | 1 | coffre (16,48) 11 ; cles dans les blocs (32,96), (64,96), (96,160) ; coffres dans les blocs 0x1F (32,160) contenu 0A et (64,160) contenu 0C | fixe et blocs |
| stage 2 | 2 | coffre (96,80) 0F | fixe |
| stage 2 | 3 | cle (128,176) | fixe |

La cour n'en a pas. Les blocs 0x1F de la salle 7 du hall (+9 = E1), de la salle 0 (CE) et de la salle 2 (DB) du stage 2 ont `+9 & C0 = C0` : ce ne sont pas des coffres (`9179`, objet `C5B5`/`C5C5`, non porte).

**Comment on les obtient** :
- objet fixe : pose au chargement de la salle, ne disparait jamais (`+6 = FF`) ;
- bloc cache (mode 3) d'objet 17 ou autre (`87FA` -> `89E3`) : au coup de fouet, `8874` efface le drapeau du flux, puis l'objet est cree directement de type 83 en (X, Y + 16), sans flamme ni chute ; `8B70` le fixe au sol (`+6` = 8, FF pour 17 a 19). 0 : rien ; 0x15 : chemin normal (`8992`) ;
- bloc d'objet 0x1F (`8814` -> `8A13`) : si `+9 & C0` != C0, un coffre (type 83, objet 19, `+6 = FF`) de contenu `+9 & 1F` en (X, Y + 16). `8874` n'est pas appele : le pointeur du drapeau (`+7/+8`) passe au coffre (`+14/+15`) et n'est efface qu'a l'ouverture (`8C0B`). Si Simon sort de la salle sans l'ouvrir, le bloc revient.

**Ramassage de la cle** (`8BDF` -> `8F68`) : au contact, hors fouet (meme test `847A` que les autres objets). Refuse si `C701 & 6` (une cle deja tenue, ou l'objet 0x12) : la cle reste au sol, sans son. Sinon `8E50` : `C701` bit 1, `C700 = 1`, `8EC9` (icone), son 14 ; `8C0B` : la cle ne revient pas.

**Ouverture du coffre** (`8F68`, `8C14`) : au contact, hors fouet (pas au fouet). Sans cle (`C700 = 0`) : rien, on reessaie a chaque image. Sinon `C700 -= 1`, son 11, le coffre est efface du decor (`8CE6`), devient son contenu (`+4 = +13`), 8 px plus haut, type 05, `+5 = 2` (orbe `EC`), `+6 = 0` (`8C2F`). Etat 5 (`8BCC`) : 32 images d'attente, puis type 02 (chute, non ramassable) ; contenu 0 : retire (`8B1B`). A la pose : type 83 -> 84, ramassable. Si `C700` tombe a 0 : `C701 &= F9` et `8EC9` efface l'icone. Les octets `+9..+12` d'un coffre valent 0 : un petit coeur n'y oscillerait pas.

**Icone du HUD** (`8EC9`, `DE = 940C`) : objet 0x17 (0x12 si `C701` bit 2) copie en (148, 12) si `C701 & 6` et `C700 > 0`, sinon la case est effacee (`8979`). `8EC9` tourne a la prise (`8E5A`), a l'objet 0x12 (`8E8D`), quand la derniere cle sert (`8C2A`), et a chaque HUD redessine par `4513` (via `8EB4`) : etat 4 (`4237` : debut, nouvelle vie, sortie de stage), fin de partie (`42F8`), entree de partie (`4E1D`), `63F6`. `8EE6`, appele au chargement de chaque salle, ne dessine que la case d'objets de droite (`C701` bits 7 a 3, x B8 a E8) : l'icone de la cle reste telle quelle d'une salle a l'autre.

**Mort, nouvelle vie, fin de partie** : `70BF` (fin de la mort, avant l'etat 6, contact ou chute) met `C700 = 0` (`70F7`) et `C701 &= 80` (`70DC`) ; `62D0` refait `70DC` (etat 6 avec vies `4294`, etat 3 `4211`, entree de partie `4E17`). L'icone reste a l'ecran jusqu'au `4513` suivant : etat 4 apres une vie perdue, ou etat 7 (`42F8`) en fin de partie, ou `C700` = 0 : pas d'icone sur l'ecran GAME OVER. La sortie de stage (etat 10 -> 4, `door-room7.md`) ne passe pas par `62D0` : la cle se garde d'un stage a l'autre.

**Port** (`candles.c`, `candle_view.c`, `main.c`, `tools/build_candles_md.py`) :
- objets fixes et contenus des blocs 0x1F (champ `content` de `VKCandleDef`, genere) ;
- cle : refus si une cle est tenue (`C700 > 0` tient lieu de `C701 & 6`, aucun objet 0x12 dans les stages 0-2) ;
- objets des blocs caches poses en (X, Y + 16) sans flamme ; coffre d'un bloc 0x1F avec retour du bloc tant qu'il n'est pas ouvert (`fixed = 0x80 | (bougie + 1)`) ;
- coffre : etat `VK_DROP_WAIT` de 32 images (orbe : l'objet 07 de la planche, comme pour la chute), chute, pose ;
- icone : `VKCandles.hud_key` = objet que `8EC9` a dessine en dernier, sprite PAL0 en (148, 12) ; mis a jour a la prise, a la derniere cle utilisee, a la nouvelle vie (`respawn`), a la sortie de stage et a la fin de partie (`vk_candles_hud`) ; `vk_candles_death` met `C700` a 0 a l'etat 6 (evenements `REFILL` et `GAME_OVER` de `health.c`).
- ramassage en chute (`7FE2`, bit 7 du type) : `VKDrop.catchable` (octet de bourrage +7) vaut 1 pour un objet de bougie apres l'etat 1 (type 82), 0 pour le contenu d'un coffre (type 02) ; les objets poses (84) restent ramassables ;
- Tests : `tests/test_candles.c` (coeur attrape en chute, contenu du coffre non ramasse en chute, cle refusee, icone, attente du coffre, blocs du stage 2, mort) ; scenario `tests/md_scenarios/p_gold_key_chest.txt` (hall salle 2 : prise de la cle, mort avec la cle, icone gardee jusqu'a l'etat 4, retour de la cle et du coffre, ouverture, gros coeur apres 32 images, coeurs 0 -> 5).

**PROVISOIRE / non porte** :
- effets des contenus 03, 0C, 0D : portes (voir 9.8) ; 0A, 0F, 11 : sans effet (voir 9.8) ;
- objet 0x12 (3 cles, icone 0x12) : absent des stages 0-2 ;
- blocs 0x1F a `+9 & C0 = C0` (`9179`) : le bloc se casse sans rien donner et ne revient pas ;
- les eclats de briques du bloc (`88C7`, `C5A6`) ne sont pas dessines ;
- l'orbe `EC` est remplace par l'objet 07 ; coffre et objets poses sont des sprites (le MSX les dessine dans le decor, `8CC1`/`8CE6`) ;
- pas de trace openMSX de la prise de cle ni de l'ouverture du coffre.

### 9.8 Effets des objets des stages 0 a 2 (lecture du code, 2026-10-29)

**Objets presents** (`candles_md.c`) : bougies 05 (stage 2 salle 2), 06 (hall 1), 07 (stage 2 salle 1), 08 (stage 2 salle 2), 09 (hall 0 et 1, stage 2 salles 3 et 4), 10 (hall 6, stage 2 salle 5), 1C (bloc, stage 2 salle 4) ; coffres 03, 0C, 0D, 0F, 11 ; blocs 0x1F de contenu 0A et 0C. Absents : 04, 0B, 0E, 12-14, 16, 1E.

**Lu dans le code** (banques 0 a 2, `tools/disasm_rom_window.py`). Aucune trace openMSX dediee : le port est verifie par les tests natifs (`tests/test_enemies.c` `test_items`, `test_spawn_pause` ; `tests/test_candles.c`) et le scenario MD `t_item_effects` (07, 09, 08 en hall salle 0).

- **Cadence** : `75C0` (`C440`), `75C6` (`C434`), `75CC` (`C42D`) et `75E2` (`C43A`) sont appeles de `6B27`-`6B2A`, dans la routine de Simon, comme la baisse de `C42D` mesuree a une fois toutes les 2 images (180 images pour 5Ah). 96h = 300 images, F0h = 480. Phase de la baisse : PROVISOIRE.
- **09, `C434`** (`7E67`, `7EB0`) : tant que `C434` != 0, les gardes du contact (`C42D`, `C43A`, `C420` >= 5) sont sautees ; un ennemi `C800` touche est detruit (`81AB` : points de la table `81CE`, comme au fouet ; `9A3E`), effet 0D, un par passe, sans degat ni recul. Les projectiles `D700` (`7EE4` -> `822A`) donnent 100 points. Simon clignote (couleur 8 une passe sur deux, `7932`).
- **08, `C43A`** (`7E72`, `7EEF`) : tant que `C43A` != 0, aucun contact. Effet 17 quand `C43A` tombe a 10h (`75EA`). Simon clignote (couleur 0Eh, `7924`).
- **06, `C440`** (`5EB8`) : la boucle des canaux d'apparition `5ED8` est sautee (types 1, 2, 4...) ; la minuterie partagee `CF10` ne bouge pas. Les fouets 1A a 1D passent aussi par `8D7C` (`8D70` sans RET) : prendre un fouet arrete les apparitions 300 images.
- **07 / 16** : `4605` ajoute B a `C415` (energie, max 20h) puis redessine la barre (`45B9`).
- **05** : `7806` detruit tous les objets `C800` (`9A3A`, sans points) et `D700` ; `928D` met +3 = FF aux objets `C5B5`/`C5C5` ; `C43E` = 18h fait clignoter la couleur de bordure (`7604` : `F3EB` = 0Eh ou 0, `CHGCLR`).
- **03, armure** (`816C`-`81A8`) : degat = table `81CE` doublee, sauf si `C701` bit 4 et Simon tourne vers l'ennemi (`C427 - X` et `C42C`) : degat de la table, et `C441` - 1 ; a 0, `C701` bit 4 efface et cases du HUD redessinees (`8EE6`). Avec `C701` & 30h, la pose de Simon prend +14h (`761E` : graphismes avec bouclier).
- **0A** (`C701` bit 6) : `713B` -> `715F` : avec 5 coeurs ou plus, une touche (`C006` bit 1) retire 5 coeurs et lance `C43B` = 5Ah (96h avec `C431` bit 2), `D010` bit 0 (ennemis figes) et le son FB : une arme secondaire qui arrete le temps.
- **0F** (`C701` bit 7, `C70F` = 3, `C431` bit 6) : `9566` : sur une touche (`C00B` bit 1), `C70F` - 1 (3 usages, puis bit 7 efface) et un affichage `47FE`/`490A` (carte ? non verifie). `C431` bit 6 : aucun lecteur trouve par adresse directe.
- **10, 11** (`C702` bits 6, 7) : lus en `9418` pour choisir `C707` dans la table `9428` (9 entrees de 4 octets) apres `93FF` ; role non compris.
- **Case d'objets du HUD** (lu dans le code, puis capture openMSX, voir plus bas) :
  - `8EE6` : `8F4A` efface la case (`490A`, rectangle x B8, y 0C, 64 x 16, couleur 0), puis pour chaque bit de `C701` du bit 7 au bit 3 (`RL C`), `8F05` lit la table `8F19` (x, objet) : 0F en E8, 0A en D8, 04 et 03 en C8 (exclusifs : `8D98` et `8DA1` effacent l'autre bit), 1E en B8 ; y = 0C (12). `8EA6` copie l'objet de la planche (page 1, x = ((objet - 1) & 15) x 16, y = 50h, ou 60h a partir de l'objet 11h ; `897D` -> `4946`, 16 x 16), les memes graphismes que les objets au sol.
  - Appels de `8EE6` : chargement de chaque salle (`6316`, etat 9), HUD redessine (`452E` dans `4513` : etat 4, fin de partie, entree de partie), armure epuisee (`81A4`, `C441` a 0 apres un coup de face), objet 04 epuise (`863F`, projectiles `D700`), 0F utilise jusqu'au bout (`957F`).
  - Ramassage (`8D30` -> `CALL 8F23` avant l'effet) : pour tout objet sauf 01 et 17 a 1D (mais 1E compris), `8F2D` efface la case et dessine l'objet ramasse (`C419`) seul en (D0, 12), puis `C5E5` = FF et `C5E6` = 20h. `8F55`, appele par `8A4A` a chaque passe, decremente `C5E6` quand `C003 & 3F` != 0 et rappelle `8EE6` a 0 : l'objet reste environ 32 passes, puis la case montre de nouveau les objets tenus. `C5E5` n'est jamais remis a 0 : `8EE6` revient toutes les 256 passes comptees, sans effet visible. Les bits poses par la prise n'apparaissent donc qu'apres ce delai (ou au changement de salle).
  - Mort (`70DC`, `C701 &= 80h`) : la case garde ses objets jusqu'au prochain `8EE6` (nouvelle salle, etat 4).
- **Pose au bouclier** (`7618`, fin de la routine de Simon `6B36`) : si `C422 = 0` (pas de coup de fouet) et `C701 & 30h` (objet 03 ou 04), `C42E` (partie basse, table `A281`) < 14h recoit +14h et `C42F` (partie haute, `A2D1`) 00/01/02 -> 1E/1F/20, 0F/10/11 -> 21/22/23 ; les autres valeurs de `C42F` restent. Pendant un coup sur l'escalier (`C420 = 3`), `764E` retire les 14h. Les cadres bas 27-29 et 37-39 sont les memes images que 07-09 et 17-19 (motifs `B1FF`, `B24B`, `B2BC`...) : recul et mort ne changent pas. `78A2` traite 1Ah/24h comme 06/10h (saut : partie basse - 6 ; accroupi : partie haute + 6). Couleurs inchangees (`790C` : couche 0 couleur 1, couche 1 couleur 42h avec CC).
- **Fin de mort** (`70BF`) : `C434`, `C43A`, `C43B`, `C440`, `C441`, `C431`, `C702`, `C700` a 0, `C701` &= 80h.

**Porte** (`sgdk/src/enemies.c` `Enemies_item` / `Enemies_death`, `candles.c` `VK_CANDLE_EVENT_PICK`, `picked` = `C419`, `vk_candles_sound`, `main.c`, `actors.c`) :
- 07 et 16 (energie), 09 (contact destructeur : points et effet 0D), 08 (aucun contact, effet 17 a 10h), 06 et fouets (`C440`), 05 (ennemis detruits, effet 1B), 03 (armure, 16 coups de face), `C431` bit 2 (F0h), remise a 0 en fin de mort ; son de chaque handler (`8D3E`).
- PROVISOIRE : X de l'ennemi pour le cote de l'armure (centre de sa boite pour les types 1 et 4).
- **Case d'objets et bouclier** (porte le 2026-10-29) : `VKCandles.c701` (bits 7 a 3 de `C701`) pose a la prise : 03 (bit 4, bit 5 efface), 04 (bit 5, bit 4 efface ; `Enemies_item` met l'armure a 0), 0A (bit 6), 0F (bit 7), 1E (bit 3), sans les effets de 0A et 0F ; evenement `VK_CANDLE_EVENT_BOX_ITEM` (son 12) pour 0A, 0F, 1E. `vk_candles_item_box` (`8EE6`) : appele par `vk_candle_view_room` (chaque salle), a la fin de partie et par `vk_candles_armor_out` (`819E`, depuis `vk_step_enemies` quand l'armure tombe a 0). `box_show_pick` (`8F23`) et `box_step` (`8F55`, `c->clock` tient lieu de `C003`). `vk_candles_death` et `vk_candles_new_life` font `C701 &= 80h`. Affichage (`candle_view.c`) : sprites PAL0 16 x 16 en (x, 12), comme la cle doree, dans quatre emplacements de 4 tuiles (392-399, 692-699). Bouclier : `tools/extract_player.py` (`SHIELD_LOWER`, `SHIELD_UPPER`) genere `vk_player_shield_tiles` (18 poses : marche 0-5, saut 6-7, escalier 8-15, accroupi 16-17, PNG `assets/player/shield-*.png`) ; `actors.c` (`shield_pose`) copie la pose du moment en tuiles 888-899 et l'affiche sur le sprite 0 a la place de la pose normale (accroupi compris : les demi-sprites 5 et 6 sont ranges). Tests : `tests/test_candles.c` (bits, objets montres ou non par `8F23`, delai de `8F55`, `819E`, mort), `tests/test_enemies.c` (04), scenario MD `u_hud_items_shield` (prise de 03 : objet seul en D0 puis 03 en C8 ; 1E, 0A, 0F : case pleine ; 04 ; poses debout, en marche, accroupi).
- **Verification openMSX** (`tools/run_hud_shield_capture.py`, `tools/hud_shield_capture.tcl`, captures dans `scratchpad/hud-shield/`, non versionnees) : `C701` force a D8h dans la cour, puis changement de salle. Debout `C42E/C42F` = 20/30 (14h/1Eh), saut 26/30 avec `C420 = 1`, accroupi 26/30 avec la partie haute 6 px plus bas (sprites y 150 et 166 pour `C425 = 176`), regard a gauche 30/33. Les motifs de sprite en VRAM (`F800`) sont identiques octet pour octet aux cadres decodes par `extract_player.py`. La case du HUD rendue depuis la VRAM (page 0) a la meme silhouette pixel pour pixel que la capture MD du scenario `u` (0 ecart sur 64 x 16).
- PROVISOIRE : cadence de `8F55` (une passe par image du port, phase de `C003` prise sur `c->clock`) ; l'objet seul de `8F2D` et la reapparition apres 256 passes ne sont pas mesures sur MSX (pas de prise d'objet dans la capture).

**Non porte** :
- clignotement de Simon (`7918`-`794A`) : Simon partage PAL2 avec les ennemis, une couleur unique demanderait une palette a part ;
- eclair de bordure de 05 (`C43E`) et `928D` ;
- 0A (arret du temps), 0F (affichage a 3 usages), 10/11 (`C707`), `C431` bit 6 : touches et routines non portees ou role non compris (seuls les bits de `C701` et les icones de 0A et 0F sont portes) ;
- objet 04 et projectiles `D700` : absents du port (le bit 5 et l'icone de 04 sont portes ; `863F` n'a rien a decompter).
