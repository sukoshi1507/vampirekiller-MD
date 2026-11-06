# Bougies, torches et drops : coup de fouet vers objet

Statut : **partiel**. Un seul cas complet est prouve par trace openMSX : la torche B de la cour (stage 0, salle 1), repetee 17 fois (section 7) avec le meme drop. Les bougies des salles du chateau ne sont pas localisees, et la table de placement n'est pas lue.

ROM de reference : SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2` (zip, jamais committee). Adresses CPU. Scripts : `tools/candle_drop_trace.tcl` (trace, un VBLANK sur `0x4028`), analyse en scratchpad (non commitee).

## 1. Tableau

| Salle | Position | Comportement au fouet | Drop observe |
|---|---|---|---|
| Cour (stage 0), salle 1, torche B | Flamme VRAM (192,144), 16x16 ; objet C800 cree en X=`A0`, Y=`91` (sprite) | Coup a Simon X=168 (`C427`), decalage 24 px. La flamme s'eteint (VRAM change au bloc (144,192) entre 1150 et 1160). | **Coeur** : objet C500, type `83` puis `84`, X=`A0`, Y=`C0`. Apparait a 1196, ramasse a 1225. |
| Cour, salle 1, torche A | Flamme VRAM (64,144) | Fouet a X=36 (run 1) et X=40 (run 2), decalage 28 puis 24 px : aucune reaction (pas d'objet cree, `C422` inchange). | Aucun |
| Chateau (stage 1 et suivants) | inconnue | non teste | inconnu |

## 2. Chronologie du drop (torche B, run `c2`)

| Trame | Evenement | Preuve |
|---|---|---|
| 1148 | `C422` 00 vers 02, `C42F` 00 vers 06 : contact du fouet | diff RAM, prouve |
| 1151 | Objet C800 : type `26`, `+3` (X) = `A0`, `+5` (Y) = `C8`, compteur `+12` = `14`. Sprite SAT entree 14 (`D638`) : Y=`91`, X=`C0`, pattern `EC` (flamme). | diff RAM + SAT, prouve |
| 1151 a 1189 | `+12` decroit de `14` a `01`, 1 pas tous les 2 trames | prouve (lecture de la trame) |
| 1190 a 1196 | `+3` croit de `A0` a `B8` (4 px par trame), `+1` passe a `01` ; sprite SAT Y `91` puis `99` (1193) puis `A1` (1195) | prouve |
| 1196 | C800 `26` vers `00` (objet mort). C500 `00` vers `83` : nouvel objet, `+1`=`A0`, `+2`=`C0`, `+4`=`02`, `+5`=`FF` | diff RAM, prouve |
| 1197 | C500 type `83` vers `84` | prouve |
| 1196 a 1224 | Simon marche de X=168 a X=186, pas de ramassage | prouve |
| 1224 | C500 `+4` 00 vers 01 | prouve |
| 1225 | C500 type `84` vers `00` (objet supprime). `C417` 00 vers `05`. `C419` 00 vers `02`. Simon a X=188. | diff RAM, prouve |
| 1200 vs 1230 | HUD : compteur de coeurs `-00` vers `-05`. Bloc VRAM (0,192) et (16,208) change. | VRAM, prouve |

Les images VRAM confirment la sequence sur le pedestal de la torche B : flamme (1150), vide (1160, 1190), coeur (1200), vide (1230, 1260).

## 3. Mecanisme

| Point | Statut | Detail |
|---|---|---|
| Le fouet touche la torche B et cree un objet flamme C800 | **prouve** (une trace) | `C422` change au contact ; C800 change dans la meme trame. La routine d'ecriture (PC) n'est pas relevee. |
| La flamme tombe (Y +8 toutes les 2 trames) puis disparait a 1196 | **prouve** | Suivi SAT entree 14. |
| A la fin de la flamme, un coeur est cree dans C500 | **prouve** pour ce cas | C800 `26` vers `00` et C500 `00` vers `83` dans la meme trame (1196). Lien de cause : **deduit** (meme trame). |
| Le coeur est un objet du bloc C500 (type `83`/`84`) | **prouve** (valeur) | Pas de table de drop lue. |
| Drop fixe pour la torche B | **probable**, voir section 7 | 17 coups (4 identiques, 13 decalages de 0 a 12 trames) : meme objet (coeur `83`) a chaque fois. Pas de graine identifiee. |
| Torche A ne produit pas de drop | **prouve** pour 2 essais | Mais pas de test a distance plus courte (dx 8 a 16). |

## 4. Ramassage

| Objet | Ramasse par | Effet | Statut |
|---|---|---|---|
| Coeur (C500) | Simon, par contact | `C417` += 5 (00 vers 05) ; HUD 00 vers 05 | **prouve** pour un coeur. Attention : `research/player-health.md` cite `C411` comme candidat ; ce cas le ecarte pour les coeurs. |
| Condition de ramassage | inconnu | Pas de ramassage a dx 8 a 26 avec Simon arrete ou en marche, un ramassage a dx 28 (X 188 contre 160). Le drapeau `+4` 00 vers 01 a 1224 peut etre une condition de delai (28 trames apres la creation). | **inconnu**, a tester |
| `C419` 00 vers 02 au ramassage | inconnu | | inconnu |
| Sous-armes, argent, bonus | non vus | | inconnu |

Divergence : la capture `scratchpad/mx/run9/before-whip.png` montre `05` au HUD avant le fouet, alors que la trace `c2` lit `00` jusqu'a 1225. Les deux runs ne sont pas identiques ; la valeur initiale de `C417` depend du scenario. Non tranche. Le deuxieme essai (section 7) lit `C417` = `00` avant le coup dans les 17 runs : le `05` de run9 n'est pas reproduit par ce scenario.

## 5. Inconnues

1. Positions des bougies et torches dans les autres salles, et table de placement en ROM. Les 8 sprites fixes `X=E0` de la salle 0 (`research/enemies-platforms.md`) ne sont pas identifies.
2. Routine d'ecriture de `C800` et `C500` (PC), et table des types (`26` flamme, `83`/`84` coeur). Pas de watchpoint PC.
3. Drop fixe pour la torche B : probable (17 coups, section 7). Reste a verifier sur d'autres torches et d'autres salles, et a identifier la routine qui choisit le drop (pas de graine trouvee).
4. Portee exacte du fouet contre une torche (dx minimal et maximal). Torche A non touchee a dx 24 et 28.
5. Condition de ramassage (drapeau `+4` de C500, distance).
6. Sens de `C419`, `C5E5`/`C5E6` (valeurs `FF`/`20` qui decroissent apres le ramassage : son ou compteur ?).
7. Lien entre coordonnees objet (`A0`/`C0`) et VRAM : le coeur est au pedestal de la torche B, mais le decalage exact n'est pas etabli.

## 6. Fichiers

- Trace : `tools/candle_drop_trace.tcl` (scenario cour, salle 1 ; sorties `diff.log`, `state.csv`, snapshots `snap-c8.bin`, `snap-c4.bin`, `snap-d6.bin`).
- Repetition : `tools/candle_drop_repeat.tcl` (meme scenario, `VK_SHIFT` decale le coup ; sorties `diff.log`, `state.csv`, `ram-*.bin`).
- Ce document : `research/candles-and-drops.md`.
- Donnees de travail (non commitees) : `scratchpad/candles/` (`c2`, `c3`, `c4`, runs `t/`) ; `scratchpad/drop2/` (`r0` a `r3`, `s1` a `s12`, `an.py`, ROM extraite dans `scratchpad/drop2/rom/`).

## 7. Deuxieme essai : repetition du coup torche B

Protocole : meme ROM (SHA-256 verifie), power-on depuis zero, meme scenario que la trace (titre, salle 1, coup sur la torche A a X=36 sans reaction, puis marche jusqu'a X=168 et coup sur la torche B, ramassage par marche a droite de 45 trames). `r0` a `r3` : coup identique (4 repetitions). `s1` a `s12` : coup retarde de 1 a 12 trames. Toutes les frames sont comptees depuis la trame 1 du run ; les chiffres sont des numeros de trame VBLANK.

| Run | Touche fouet (trame) | Flamme C800 `26` creee | Flamme eteinte (C800 vers `00`) | Coeur C500 `83` cree | Coeur supprime (C500 vers `00`) | `C417` 00 vers 05 |
|---|---|---|---|---|---|---|
| r0, r1, r2, r3 (identiques) | 1146 | 1151 (+5) | 1196 (+50) | 1196 (+50) | 1225 (+79) | 1225 |
| s1 | 1147 | 1151 (+4) | 1196 (+49) | 1196 (+49) | 1227 (+80) | 1227 |
| s2 | 1148 | 1153 (+5) | 1198 (+50) | 1198 (+50) | 1227 (+79) | 1227 |
| s3 | 1149 | 1153 (+4) | 1198 (+49) | 1198 (+49) | 1229 (+80) | 1229 |
| s4 | 1150 | 1155 (+5) | 1200 (+50) | 1200 (+50) | 1229 (+79) | 1229 |
| s5 | 1151 | 1155 (+4) | 1200 (+49) | 1200 (+49) | 1231 (+80) | 1231 |
| s6 | 1152 | 1157 (+5) | 1202 (+50) | 1202 (+50) | 1231 (+79) | 1231 |
| s7 | 1153 | 1157 (+4) | 1202 (+49) | 1202 (+49) | 1233 (+80) | 1233 |
| s8 | 1154 | 1159 (+5) | 1204 (+50) | 1204 (+50) | 1233 (+79) | 1233 |
| s9 | 1155 | 1159 (+4) | 1204 (+49) | 1204 (+49) | 1235 (+80) | 1235 |
| s10 | 1156 | 1161 (+5) | 1206 (+50) | 1206 (+50) | 1235 (+79) | 1235 |
| s11 | 1157 | 1161 (+4) | 1206 (+49) | 1206 (+49) | 1237 (+80) | 1237 |
| s12 | 1158 | 1163 (+5) | 1208 (+50) | 1208 (+50) | 1237 (+79) | 1237 |

Constantes sur les 17 runs (**prouve**, lecture diff) :
- Type de l'objet flamme : `26` (decimal 38) ; type du coeur : `83` puis `84` (decimal 131 puis 132) ; coeur cree en X=`A0`, Y=`C0` (`+1`, `+2`) : identique.
- La flamme meurt a la trame ou le coeur apparait, dans chaque run. Le decalage (+4 ou +5, +49 ou +50) depend de la parite du coup : l'objet evolue par pas de 2 trames.
- Ramassage : `C417` 00 vers `05` et `C419` 00 vers `02` dans les 17 runs, dans la phase de marche a droite.
- RAM complete `C000`-`FFFF` : 4 repetitions identiques **bit a bit** (MD5 identiques) aux trois instants (avant coup, creation C800, creation C500).
- `C417` = `00` avant le coup dans les 17 runs.

**Verdict fixe ou aleatoire (torche B, cour, salle 1) : fixe a l'echelle teste.** 17 coups sur 17 donnent le meme coeur. Si la chance d'obtenir un coeur etait de 80 % ou moins, 17 coups identiques auraient une probabilite inferieure a 3 %. Les coups ne sont pas des essais independants (meme etat de depart, seul le moment change) : la preuve est indicative, pas une preuve de table. Si le tirage dependait du moment du coup (compteur de trames avance par le jeu), les 12 decalages auraient du changer le resultat : ce n'est pas le cas. Un tirage unique fait a la mise sous tension (graine fixe) ne serait pas detecte par cette methode ; il n'est pas exclu.

**Graine ou compteur (candidat, non prouve).** Entre `r0` et `s1` (une trame d'ecart, au moment du coup), 42 octets de RAM different. Groupes : `C003`, `C005`, `C007`, `C00F`, `C025`, `C026`, `C039`, `C03A`, `C04D` (zone `C0xx`), `C42E`, `C42F`, `C476`, `C486`, et `F0B8` a `F0D7` (32 octets, probablement une pile ou un tampon, non identifie). Ce sont des candidats de compteur : aucun n'a ete relie au choix du drop, car le drop ne change pas dans ce scenario. La routine d'ecriture et la table de drop ne sont pas lues.

**Ce qui n'est pas tranche.** Autres torches (seule la torche B est testee ici), autres salles, autres etats de jeu (HUD, ramassages anterieurs). Le `05` du HUD avant coup de `run9` n'est pas reproduit (section 4).
