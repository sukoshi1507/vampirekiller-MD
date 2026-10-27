# Bougies, torches et drops : coup de fouet vers objet

Statut : **partiel**. Un seul cas complet est prouve par trace openMSX : la torche B de la cour (stage 0, salle 1). Les bougies des salles du chateau ne sont pas localisees, et la table de placement n'est pas lue.

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
| Drop fixe pour la torche B | **deduit** | Un seul cas. Aucune valeur aleatoire vue, mais un seul essai. |
| Torche A ne produit pas de drop | **prouve** pour 2 essais | Mais pas de test a distance plus courte (dx 8 a 16). |

## 4. Ramassage

| Objet | Ramasse par | Effet | Statut |
|---|---|---|---|
| Coeur (C500) | Simon, par contact | `C417` += 5 (00 vers 05) ; HUD 00 vers 05 | **prouve** pour un coeur. Attention : `research/player-health.md` cite `C411` comme candidat ; ce cas le ecarte pour les coeurs. |
| Condition de ramassage | inconnu | Pas de ramassage a dx 8 a 26 avec Simon arrete ou en marche, un ramassage a dx 28 (X 188 contre 160). Le drapeau `+4` 00 vers 01 a 1224 peut etre une condition de delai (28 trames apres la creation). | **inconnu**, a tester |
| `C419` 00 vers 02 au ramassage | inconnu | | inconnu |
| Sous-armes, argent, bonus | non vus | | inconnu |

Divergence : la capture `scratchpad/mx/run9/before-whip.png` montre `05` au HUD avant le fouet, alors que la trace `c2` lit `00` jusqu'a 1225. Les deux runs ne sont pas identiques ; la valeur initiale de `C417` depend du scenario. Non tranche.

## 5. Inconnues

1. Positions des bougies et torches dans les autres salles, et table de placement en ROM. Les 8 sprites fixes `X=E0` de la salle 0 (`research/enemies-platforms.md`) ne sont pas identifies.
2. Routine d'ecriture de `C800` et `C500` (PC), et table des types (`26` flamme, `83`/`84` coeur). Pas de watchpoint PC.
3. Drop aleatoire ou fixe : un seul cas. Il faut plusieurs coups sur la torche B, puis sur d'autres torches.
4. Portee exacte du fouet contre une torche (dx minimal et maximal). Torche A non touchee a dx 24 et 28.
5. Condition de ramassage (drapeau `+4` de C500, distance).
6. Sens de `C419`, `C5E5`/`C5E6` (valeurs `FF`/`20` qui decroissent apres le ramassage : son ou compteur ?).
7. Lien entre coordonnees objet (`A0`/`C0`) et VRAM : le coeur est au pedestal de la torche B, mais le decalage exact n'est pas etabli.

## 6. Fichiers

- Trace : `tools/candle_drop_trace.tcl` (scenario cour, salle 1 ; sorties `diff.log`, `state.csv`, snapshots `snap-c8.bin`, `snap-c4.bin`, `snap-d6.bin`).
- Ce document : `research/candles-and-drops.md`.
- Donnees de travail (non commitees) : `scratchpad/candles/` (`c2`, `c3`, `c4`, runs `t/`).
