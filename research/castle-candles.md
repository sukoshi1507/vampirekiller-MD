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
| 03 | `8D98` | `C701` : bit 5 efface, bit 4 mis ; `C441` = 10 |
| 04 | `8DA1` | `C701` : bit 4 efface, bit 5 mis ; `C441` = 10 |
| 05 | `8DBB` | `7806`, `928D`, `C43E` = 18 ; son `1B` |
| 06 | `8D7C` | minuterie `C440` = 96 (F0 si bit 2 de `C431`) |
| 07 | `8DDE` | points (`4605`, B = 08) |
| 08 | `8DCD` | minuterie `C43A` = 96 / F0 ; son `16` |
| 09 | `8DE5` | minuterie `C434` = 96 / F0 |
| 0A | `8D94` | `C701` bit 6 |
| 0B, 0C, 0D | `8DF5`, `8DFC`, `8E03` | `C431` bits 2, 3, 4 |
| 0E | `8E16` | `870C`, `C702` bit 0 |
| 0F, 10 | `8E33`, `8E1D` | `C431` bit 6 et `C70F` = 3 ; `C702` bit 6 |
| 11 | `8E26` | `C702` bit 7 |
| 12 | `8E79` | `C431` bit 1, `C701` bit 2, `C700` = 3 |
| 13, 14 | `8E42`, `8E4B` | points (`44EC`, 5000 et 1000) |
| 15 | `8E14` | rien (pas de flamme) |
| 16 | `8E0D` | points (`4605`, B = 20) |
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
- les bougies et objets sont des sprites PAL0 aux coordonnees MSX ; compteur de coeurs en colonnes 24-25 de la ligne 0.

**PROVISOIRE** :
- la flamme `C800` est remplacee par les etincelles de l'effet de flamme, et l'orbe EC par l'objet 07 de la planche ;
- la cadence de la bougie murale (8 images) n'est pas mesuree ;
- le dessin des blocs caches (mode 3) n'est pas porte : le mur n'est pas dessine et la carte de collision ne change pas ;
- les contenus 0x1F (`+7`-`+9`) et les objets 03-16 (minuteries, points, bits `C701`/`C702`) n'ont pas d'effet ;
- 1B a 1D (dague, hache, croix) ne changent pas l'arme ;
- les bougies restent eteintes jusqu'au changement de stage (le rechargement du flux de stage n'est pas trace) ;
- (fait) icone de la cle doree : `8EC9` copie l'objet 0x17 (0x12 si le bit 2 de `C701` est mis) dans le HUD en (148, 12) tant que `C700` > 0 ; port : sprite PAL0 a cette place (`candle_view.c`).
