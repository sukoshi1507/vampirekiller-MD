# Analyse Z80 : boucle VBLANK et accès VDP (ports 98/99)

Analyse statique de `Vampire Killer (Japan, Europe) (En).zip`, ROM 128 Ko (SHA-256 contrôlé). Aucune exécution, aucune trace : les points marqués **déduit** attendent une confirmation en émulation.

## Segmentation

Le cartouche est à banques de 8 Ko, mappeur écrit aux adresses `6000`/`8000`/`A000`.

| Bande | Fenêtre CPU | Rôle observé |
|---|---|---|
| 0 | `4000`–`5FFF` (fixe) | En-tête `AB`, initialisation, gestionnaire VBLANK `4028`, routines VDP |
| 14 | `8000`–`9FFF` (sélectionnée par `4028`) | Routine `8964`, logique de jeu |
| 15 | `A000`–`BFFF` (sélectionnée par `4028`) | Non examinée |

En-tête : `INIT = 4075`.

## Boucle principale et VBLANK (prouvé par le code)

- **Installation du hook** en `40B0`–`40C2` : `LD A,C3 ; LD (FD9F),A` puis `LD HL,4028 ; LD (FDA0),HL`. Le BIOS MSX appelle `H.TIMI` (`FD9F`) à chaque VBLANK, donc `4028` est le gestionnaire d'interruption de chaque image.
- **Premier plan** : `40C3 JR $` après `EI`. C'est une boucle d'attente ; le travail se fait dans `4028`.
- **Gestionnaire `4028`** :
  1. `DI`, puis si `(E600) ≠ 0` saut à `40C5` (autre chemin).
  2. Sélection des banques `14` en `8000` et `15` en `A000`, puis `CALL 8964`.
  3. Restauration des banques depuis `F0F2`/`F0F3`.
  4. Garde de réentrance `(C005)`, puis `CALL 4B9D` et `CALL 414D`.
  5. `EI ; RET`.

Le point d'échantillonnage `0x4028` de la trace openMSX (`tools/trace_simon_live.tcl`) est donc bien le gestionnaire VBLANK.

## Accès aux ports 98/99 en bande 0

Exploration récursive depuis `4028`, `40C3` et `8964` (fenêtre suivie seulement dans la bande où l'instruction se trouve). Le registre `C` est chargé depuis les variables RAM `(0006)`/`(0007)`, dont la valeur n'est pas vérifiée (voir limites).

### Lecture du registre d'état (S#n) — port 99

| Adresse | Instruction | Rôle |
|---|---|---|
| `4855`–`486E` | `OUT (C),A` / `IN A,(C)` | Lit un registre d'état en pointant R#15 : écrit `n`, `0x8F`, lit, puis restaure R#15 à 0 |
| `484C` | boucle sur `4855` | Attend le bit de prêt du moteur de commandes (bit 0 de S#2) |

### Pointeur de registre indirect (R#17) et écriture de paramètres — ports 99 puis 9B

| Adresse | Instruction | Rôle |
|---|---|---|
| `4875`–`488E` | `OUT (C),A` | Met `24` puis `91` dans R#17 (pointeur sur R#36), écrit les paramètres via `9B` |
| `4900`–`4945` | `OUT (C),r` ×11 | Même schéma ; R#36 à R#46 reçoivent les paramètres, la dernière valeur `C0` (`4940`) va dans R#46 |

La valeur `C0` dans R#46 correspond à la commande **HMMV** (remplissage rectangulaire) dans la table V9938. Ce n'est **pas** une copie LMMM/HMMM. **Déduit** de la table des commandes, à confirmer par trace.

### Écriture de données VRAM et adresse — port 98

| Adresse | Instruction | Rôle |
|---|---|---|
| `4698`–`469D` | `OUT (C),A` en boucle (`DEC D`, `DJNZ`) | Écrit une suite d'octets dans la VRAM (port de données) |
| `46A1`–`46AE` | `OUT (C),A` | Écrit une valeur unique dans le port de données |
| `46AF`–`46C9` | `OUT (C),A` ×5 | Positionne l'adresse VRAM : R#14 (`8E`), puis octets bas et haut (dont `40` ou bits `&3F`) |

## Autres occurrences trouvées

- `IN A,(99)` en bande 5 (offset `1A1A`, fichier `47514`) : aligné sur une instruction, mais aucune fenêtre CPU connue. Non atteint depuis `4028`.
- `OUT (98),A` en bande 14 (offset `193B`, fichier `121147`) : non aligné (probablement une donnée). Non vérifié.
- Plusieurs `OUT (C),r` et `OTIR`/`OUTI`/`OUTD` en bande 13–15 (non atteints depuis les points d'entrée explorés).

## Correction d'une erreur précédente

J'avais annoncé un désalignement des instructions entre `4660` et `4700`. C'était une erreur de mon outil d'analyse : le désassemblage lisait l'offset fichier `0x4660` au lieu de `0x0660`. Le balayage linéaire initial était correct, et l'exploration récursive le confirme (`OUT (C),A` en `4698`).

## Limites

- **Valeurs de `(0006)`/`(0007)`** : supposées `98` d'après la convention MSX, qui rend cohérents les ports `99` et `9B`. Non vérifiées en RAM. Une trace openMSX ou BlastEm les confirmerait.
- **Routines bancaires** : les appels de `8964` vers la bande 0, et les routines des bandes 14 et 15, ne sont pas explorés. Les accès VDP qui s'y trouvent ne sont pas listés.
- **Appels BIOS** : `8964` appelle `0093` (non identifié ici). Les appels au BIOS VDP (`WRTVDP`, `LDIRVM`…) ne sont pas recensés.
- **Accès VDP déclenchés par le jeu** : sans trace, on ne sait pas quelle routine s'exécute réellement à chaque image.

## Prochaine étape

Tracer en openMSX les écritures sur les ports 98/99 et 9B pendant une image de jeu, avec `PC` et la valeur de `(0006)`/`(0007)`. Cela tranchera les points déduits et complétera les routines bancaires.

## Vérification en émulation (openMSX 21.0, C-BIOS MSX2)

Script : `tools/trace_vdp_routines.tcl`. Scénario : démarrage, puis SPACE répété de 40 s à 60 s, comme dans `ATTACK_TRACE_OPENMSX.md`.

**Confirmé :**
- Indexes des registres Z80 dans openMSX : `0=A 1=F 2=B 3=C 4=D 5=E 6=H 7=L`.
- À l'entrée de `4917` (configuration de commande), `C = 0x99` : l'écriture part bien du port de contrôle 99 (pointeur R#17), puis les paramètres passent par `9B`.
- À `4698` (boucle d'écriture), `C = 0x98` : port de données VRAM.
- À `46BB` (adresse VRAM), `C = 0x99`.
- `(0006)`/`(0007)` : cohérent avec `0x98` (`L = 0x98` avant `INC L` en `4855`). Non relu directement en RAM.

**Activité par fenêtre :**

| Fenêtre | `status_read` | `cmd_setup_9B` | `data_loop_98` | `vram_addr_set` |
|---|---|---|---|---|
| 0–10 s (boot) | 2950 | 4 | 256 | 431 |
| 10–40 s (intro, titre) | 8128 | 46 | 384 | 8756 |
| 40–60 s (coups de fouet) | 299 | **0** | **0** | 2400 |

**Conclusions :**
- Pendant les coups, aucune écriture de bloc de commande (HMMV) et aucune écriture VRAM via la boucle `4698`. Le fouet n'utilise donc pas ces routines.
- Il reste à identifier la routine qui écrit réellement pendant les coups, probablement via `46BB` ou un autre chemin d'écriture non couvert.
- Les valeurs de `C` à l'entrée de `4855` et `4875` ne sont pas significatives : `C` y est chargé après l'entrée.
