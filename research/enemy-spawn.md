# Spawn des ennemis, salle 1 : table et routine trouvees

Statut : **table et routine de spawn identifiees et recoupees par la trace** (openMSX, `scratchpad/enemies6/frames.txt` et watchpoints `scratchpad/enemies7/`). La table n'est pas une liste de frames : c'est une **minuterie** pilotee par une table de periodes et une table de drapeaux par salle. Le module `sgdk/src/enemies.c` reproduit cette logique (pur C, sans SGDK). Il ne lit pas la ROM : les valeurs de table sont recopiees avec leur adresse.

Adresses CPU sauf mention contraire. ROM de reference : SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2` (zip, jamais committee). Banques : fenetre `4000-5FFF` fixe a la banque 0 ; `6000` = registre de banque (valeur `01` pendant le spawn) ; `8000` = registre (valeur `02` pendant 9CE6, `0E` pendant la lecture de 85A6) ; `A000` = registre (valeur `03`). Offset fichier de la banque n : `n * 2000` pour la fenetre (ex. 9CE6 banque 2 = fichier `0x4000 + 0x1CE6`, offset `0x5CE6` ; 5F1D banque 0 = fichier `0x1F1D`).

## 1. Methode

- openMSX (binaire du scratchpad, `-machine C-BIOS_MSX2 -cart game.rom`), scenario identique a `enemies6/trace.tcl` (SPACE au titre, RIGHT de 25 s a 60 s). Watchpoints d'ecriture sur `D638-D65F` (SAT des ennemis), points d'entree sur `5F1D`, `6048`, `9CE4`, `9CC3`, ecriture sur `6000/8000/A000`.
- Desassemblage : `scratchpad/dis.py` (banque et fenetre explicites) avec le module `z80dis` de `scratchpad/venv`.
- Correspondance des images : image trace = image compteur openMSX - 1194 (`908` = `2102`, `1012` = `2206`). Le compteur ne sert qu'a relier les deux sources.
- Pas d'emulateur Z80 maison utilise pour ces resultats.

## 2. Table de drapeaux par salle (verifie, ROM)

- Pointeur de stage : `85A6 + 2*D000`, lu en banque `0E` (`5ED1`). Pour `D000 = 1` : `85A8 = CF 85`, donc table a `85CF`.
- Octet de drapeaux de la salle : `[85CF + D001]`, lu par `4066` (ADD A,E) puis `LD A,(DE)` en `5EE7`.
- Valeurs stage 1 (`85CF-85D6`) : `01 01 00 00 08 01 01 00` (D001 = 0 a 7). `85D7` appartient au stage 2.
- Bits : bit0 -> appel de `9CE6` (type 1) ; bit1 -> `9D4B` ; bit2 -> `9D52` ; bit3 -> `9D97` (type 4 observe dans la table, non porte) ; bit5 -> `9DD5` (type 8). Seul le bit0 est necessaire pour la salle 1 (D001 = 0 et 1).
- Chaine d'appel : `98E9 : CALL Z,5EB8` (banque 2) -> `5EB8` (gardes : `C440 = 0`, `C420 <> 6`, `C5AC` hors de `{2,3,5}`) -> boucle de drapeaux `5ED8-5F19` -> `9CE6` (`5EF6`).

## 3. Minuterie de spawn (verifie, code et trace)

Variables RAM : `CF10` (compteur partage, decremente a chaque appel), `CF00` (compte a rebours du canal type 1), `CF01` (index de sequence). Initialisation : `9CA9` ecrit `CF00 = 04`, `CF01 = 00` (table `9CBB`, banque 2) ; `9CA9` est appele en `6324` (banque 1, chargement de salle).

`9CC3` (banque 2), appele avec `HL = CF00` :
1. `DEC (CF10)` ; retour si `(CF10) & 3 <> 0`. Donc le compte a rebours ne bouge que sur un appel sur quatre.
2. `DEC (CF00)` ; retour si non nul.
3. Tir : `idx = CF01` puis `CF01 = idx + 1` ; `periode = [9D43 + (idx & 7)]`.
4. Rechargement : `CF00 = max(periode - 2*D012, 0) + 1` (`9CD8-9CE3`).

Table `9D43-9D4A` (verifie) : `0C 12 0C 0C 0C 12 0C 0C`. Donc rechargements 13 (`0D`) et 19 (`13`), observes en trace (`CF00 = 0D` puis `13` apres les tirs 1 et 2).

Cadence : `CF10` passe de `F0` (tir 1, image 908) a `BC` (tir 2, image 1012) : 52 decréments en 104 images, soit **un appel de 9CC3 tous les 2 images** (mesure). Un tir n'arrive que sur un appel dont `CF10 & 3 = 0`, donc l'intervalle entre deux tirs vaut exactement `4 * rechargement` appels, soit `8 * rechargement` images :
- tir 1 (908) -> tir 2 (1012) : 104 images = 8 * 13 ;
- tir 2 (1012) -> tir 3 (1164, refuse) : 152 images = 8 * 19 ;
- premier tir apres l'entree en etat 5 (`C000=5`, image 877) : 908 - 877 = 31 images, coherent avec 16 appels (+ phase).

Les intervalles 130, 111 et 152 de l'ancienne note ne sont pas des periodes de cette minuterie : ils melent les refus (section 5) et les changements de salle.

## 4. Position, cote et type (verifie, code)

`9CFC` (banque 2), avec `L = D000 = 1` et `H = D001` :
- `E` (Y de spawn) = `C0`, sauf `H = 1` et `C425 < 88` : `DE = F060` (cas de haut, non observe en trace).
- `D` (X de spawn) = `F0` (droite) ou `10` (gauche) selon `C42C` : `C42C = 0` -> `F0`, sinon `10` (`9D2E`). Observe : tir 1 `C42C = 00`, tir 2 `C42C = 01`.
- Pour `D000 = 1`, `H` vaut 0 ou 1 : meme regle de cote, sauf le cas `C425 < 88`.
- Autres valeurs de `D000` (`2`, `3`, `8`, `11`) : autres branches de `9CFC` (ex. `L = 2` : `E = A0`, cote selon `CF01` bit1). Non portees (stage != 1).

Creation (`5F1D`, banque 0) : `C = 1` (type). Table `6057` (banque 1) : `[6058] = 04` = nombre d'entrees SAT du type 1 (4). Le premier bloc libre `C800` (puis `C880`) recoit l'entite ; `5F5C-5F76` cherche les entrees SAT libres (`D638` + 4*k, valeur `E0` = masquee) et ecrit `Y = E1` provisoire (`5F66`, image de spawn). `6048` (banque 1) enregistre l'index SAT de chaque sous-entree dans le bloc (`+21 + 5k`) ; `6501` (banque 1) recopie Y/X/N dans la SAT pour chaque sous-entree.

Observe (trace) : tir 1 -> SAT `s14-s17` (`D638`), tir 2 -> `s18-s21` (`D648`) : le scan commence a `D638` et saute les entrees deja utilisees.

Type par bloc : tir 1 dans `C800` (motifs `40/44`, `48/4C`), tir 2 dans `C880` (motifs `58/5C`, `60/64`) ; reapparitions 1256 (`C800`, type A) et 1360 (`C880`, type B). **Observe**, 4 cas. La cause (quel code choisit `N`) n'est pas identifiee.

## 5. Garde de spawn (verifie, code et 6 cas de trace)

`9E16` (banque 2) : entree `X = C427` (X de Simon), `D` (cote calcule ci-dessus). Refus = `RET C` dans `9CF6` (le tir consomme la minuterie, sans entite).
- `X >= C0` : refus si `D >= C0`.
- `40 <= X < C0` : pas de refus.
- `X < 40` : refus si `D < 40`.

Cas observes :
- 908 (`X=30`, D=F0) : accepte. 1012 (`X=7A`, D=10) : accepte. 1164 (`X=CE`, D=F0) : refuse (pas de SAT ecrit, trace `s14 = E0` a 1164). 1256 (`X=28`, D=F0) : accepte. 1360 (`X=7A`, D=10) : accepte. 1512 (`X=C6`, D=F0) : refuse (trace : pas de nouvelle entite a `s14`).

## 6. Corrections de l'ancienne version

- Les spawns 1256 et 1360 n'ont **pas** une position `X=00` (gauche) et `X=D8` (droite) : ce sont des images de spawn provisoires (`Y=E1`, comme 908 et 1012). La position reelle est `X=E5` a 1257 (cote droit) et `X=0A` a 1361 (cote gauche), conforme a la regle `C42C`.
- Le compteur `C476` n'est utilise ni par `9CC3`, ni par `9CE6`, ni par `9CFC`, ni par `9E16`, ni par `5F1D` : ce n'est **pas** le declencheur. Abandonne.
- `C000 = 9` : les ecritures `E0` en `63CA` (appelees depuis `62F8`) vident la SAT au changement d'ecran (observe a 293 et au passage `C000=9`). Ce n'est pas une regle de spawn.

## 7. Valeurs portees dans `sgdk/src/enemies.c` (provenance)

| Donnee | Adresse ROM / source | Valeur | Confiance |
|---|---|---|---|
| `kEnemyStage1RoomFlags` | `85CF-85D6` (banque 0E) | `01 01 00 00 08 01 01 00` | verifie |
| `kEnemySpawnPeriod` | `9D43-9D4A` (banque 2) | `0C 12 0C 0C 0C 12 0C 0C` | verifie |
| Nombre d'entrees type 1 | `6058` (banque 1) | `4` | verifie |
| Init minuterie | `9CBB` (banque 2) | `CF00=4`, `CF01=0` | verifie |
| Reload | `9CD8-9CE3` | `max(p - 2*D012, 0) + 1` | verifie (D012 = 0 observe) |
| Garde | `9E16` | section 5 | verifie |
| Cote (salle D001 != 1 ou C425 >= 88) | `9D2C-9D2E` | `C42C != 0 -> gauche` | verifie |
| Position E = 60 (D001 = 1, C425 < 88) | `9D38` | `DE = F060` | code, non observe |
| Y de sprite = E - 20 | `5F1D` + trace | `C0 -> A0` | observe (un seul E) |
| Type par bloc (C800 A, C880 B) | trace, 4 cas | | observe |
| `extraPhase` (A 1, B 15) | trace (decalage +1 px) | | observe, cause inconnue |
| Mouvement, animation, sortie | trace (`frames.txt`) | `research/enemies-platforms.md` section 1.2 | observe |

Limites : `CF10` n'est pas initialise par `9CA9` ; sa valeur au chargement est inconnue (module : 0), donc la phase exacte du premier tir (+/- 3 appels) n'est pas prouvee. L'origine de la cadence "un appel sur deux images" (`98E9`) n'est pas identifiee.

## 8. Tests

- `tests/test_enemies.c` : tables, intervalles 104 et 152 images (16, 68, 144, 196 appels), reload avec D012, drapeau, garde gauche et droite, cote, cas `E = 60`, trajectoires E1/E2 (images 908 et 1012 fournies par le driver de test, tirees de la trace).
- Commande : `gcc -std=c99 -Wall -Wextra -Werror -pedantic -I sgdk/src tests/test_enemies.c sgdk/src/enemies.c`.

## 9. Ce qui reste

1. Verification en emulation du decalage de phase (`CF10` au chargement de salle) avec un watchpoint sur `CF10` et `6324`.
2. Choix du motif N (quel code ecrit `N` dans le bloc, `6501` ne fait que recopier).
3. Stages autres que 1 : extraire les autres pointeurs de `85A6` et les branches `9CFC` (`L = 2, 3, 11`), et les bits 1, 2, 3, 5 de drapeaux (routines `9D4B`, `9D52`, `9D97`, `9DD5`).
4. Geometrie du sprite (16x32 ou empile) et boite de contact : non verifiees.
5. Routine appelant `98E9` (parite de cadence) : non identifiee.
