# Type 10h — chemin de tir Z80 (analyse statique, non exécutée)

Source : ROM originale certifiée SHA-256 par `tools/probe.py:load_rom`.
Table de mise à jour CPU `9942h`, type `10h` → `ADF8h` (image ROM mappée
`8000h–BFFFh`, offsets fichier `4000h–7FFFh`). Le code ci-dessous est
vérifié instruction par instruction par `tools/verify_enemy10_fire_path.py`.

## Branche `AEAFh` → `9F61h`

| PC | Instruction | Conséquence locale (si ce PC est atteint) |
| --- | --- | --- |
| `AEAF` | `BIT 1,(IX+01)` | Lit le bit 1 du champ objet +01 |
| `AEB3` | `JR NZ,AEE9` | Si bit 1 = 1, quitte le chemin de tir |
| `AEB5` | `CALL AF40` | Calcule une valeur dans A (sémantique de AF40 à étudier) |
| `AEB8` | `CP 2A` | Compare A à `2Ah` |
| `AEBA` | `JR NC,AEDF` | Si A ≥ `2Ah`, ne tire pas par ce chemin |
| `AEBC` | `BIT 0,(IX+10)` | Teste un bit du champ +10 |
| `AEC0` | `RET Z` | Si bit 0 = 0, ne tire pas |
| `AEC1` | `SET 1,(IX+01)` | Active le bit 1 du champ +01 |
| `AEC5`/`AEC7` | `PUSH IX; POP HL` | Copie le pointeur de l'objet source |
| `AEC8` | `LD (CFFC),HL` | Enregistre ce pointeur à `CFFCh` |
| `AECB` | `LD HL,0000` | Initialise HL |
| `AECE` | `LD DE,0400` | Initialise DE à +`0400h` |
| `AED1` | `LD A,(C427)` | Lit la position X de Simon (nom établi dans le port existant) |
| `AED4` | `CP (IX+05)` | Compare au champ X objet +05 |
| `AED7` | `JR NC,AEDC` | Si Simon X ≥ objet X, conserve DE=`0400h` |
| `AED9` | `LD DE,FC00` | Sinon DE = -`0400h` en 16 bits |
| `AEDC` | `JP 9F61` | Saut direct vers la routine partagée |

`9F61` commence par `LD A,(IX+03); SUB 10h; LD C,A`. Cette séquence
n'est **pas** une preuve que `9F61` crée effectivement un projectile à chaque
appel, ni une trace de la position et du sprite produits. `AF40h` reste à
analyser ; les conditions de parcours depuis `ADF8h` jusqu'à `AEAFh`, la
cadence, l'initialisation, la collision, la durée de vie et les motifs ne
sont pas établis par ce contrôle.

**Attention** : la routine `9F61h` est partagée avec le type 0Bh existant
(`sgdk/src/enemies.c:boneSpawn`). Ne pas réutiliser sa vitesse horizontale
`0180h` pour le type `10h` : cette branche charge explicitement `0400h` ou
`FC00h` avant le saut. Le rôle exact de DE à l'entrée de `9F61h` doit être
confirmé par exécution Z80.

Reproduction : `bash tests/run_enemy10_fire_path.sh`, puis
`bash tests/run_native.sh`. Ce sont des contrôles *statiques*, pas le test
natif contre une trace image par image demandé par le lot 1.
