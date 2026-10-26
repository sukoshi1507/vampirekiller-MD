# Références candidates Z80 aux variables de navigation

Variables du chargeur : `D000` (stage), `D001` (salle).

Recherche brute : **62 motifs**, dont 9 écritures candidates et 53 lectures candidates.

**Attention : aucun lien entre deux salles n’est authentifié par ce balayage.**

| Offset ROM | Banque | Instruction candidate | Variable | Accès |
|---|---:|---|---|---|
| `00332` | 0 | `LD (nn),A` | `stage` | write |
| `00DFA` | 0 | `LD (nn),A` | `room` | write |
| `00E07` | 0 | `LD (nn),A` | `stage` | write |
| `00F86` | 0 | `LD A,(nn)` | `stage` | read |
| `00F8A` | 0 | `LD A,(nn)` | `room` | read |
| `0164F` | 0 | `LD A,(nn)` | `stage` | read |
| `01657` | 0 | `LD A,(nn)` | `stage` | read |
| `01716` | 0 | `LD A,(nn)` | `stage` | read |
| `01789` | 0 | `LD A,(nn)` | `stage` | read |
| `01797` | 0 | `LD A,(nn)` | `room` | read |
| `017CC` | 0 | `LD A,(nn)` | `stage` | read |
| `01A98` | 0 | `LD A,(nn)` | `stage` | read |
| `01B52` | 0 | `LD A,(nn)` | `stage` | read |
| `01BCF` | 0 | `LD A,(nn)` | `room` | read |
| `01BDA` | 0 | `LD A,(nn)` | `stage` | read |
| `01E3D` | 0 | `LD (nn),A` | `stage` | write |
| `01E4E` | 0 | `LD A,(nn)` | `stage` | read |
| `01E59` | 0 | `LD (nn),A` | `room` | write |
| `01ED8` | 0 | `LD A,(nn)` | `stage` | read |
| `01EE1` | 0 | `LD A,(nn)` | `room` | read |
| `021C2` | 1 | `LD A,(nn)` | `stage` | read |
| `021CF` | 1 | `LD A,(nn)` | `room` | read |
| `02343` | 1 | `LD A,(nn)` | `stage` | read |
| `0234C` | 1 | `LD A,(nn)` | `room` | read |
| `02402` | 1 | `LD A,(nn)` | `stage` | read |
| `030D6` | 1 | `LD (nn),A` | `room` | write |
| `036D4` | 1 | `LD A,(nn)` | `stage` | read |
| `0373A` | 1 | `LD A,(nn)` | `stage` | read |
| `03756` | 1 | `LD A,(nn)` | `stage` | read |
| `037F2` | 1 | `LD A,(nn)` | `stage` | read |
| `03955` | 1 | `LD A,(nn)` | `stage` | read |
| `03965` | 1 | `LD A,(nn)` | `stage` | read |
| `03C66` | 1 | `LD A,(nn)` | `stage` | read |
| `04F9F` | 2 | `LD HL,(nn)` | `stage` | read |
| `0503B` | 2 | `LD DE,(nn)` | `stage` | read |
| `050F4` | 2 | `LD A,(nn)` | `stage` | read |
| `05129` | 2 | `LD A,(nn)` | `stage` | read |
| `05453` | 2 | `LD A,(nn)` | `stage` | read |
| `05465` | 2 | `LD A,(nn)` | `room` | read |
| `055E8` | 2 | `LD A,(nn)` | `stage` | read |
| `0560C` | 2 | `LD A,(nn)` | `stage` | read |
| `05627` | 2 | `LD A,(nn)` | `room` | read |
| `0567A` | 2 | `LD A,(nn)` | `stage` | read |
| `05815` | 2 | `LD A,(nn)` | `room` | read |
| `05853` | 2 | `LD A,(nn)` | `stage` | read |
| `05C6C` | 2 | `LD A,(nn)` | `stage` | read |
| `05D00` | 2 | `LD HL,(nn)` | `stage` | read |
| `062AA` | 3 | `LD A,(nn)` | `stage` | read |
| `062B1` | 3 | `LD A,(nn)` | `room` | read |
| `0651D` | 3 | `LD A,(nn)` | `stage` | read |
| `07611` | 3 | `LD A,(nn)` | `stage` | read |
| `0761A` | 3 | `LD A,(nn)` | `room` | read |
| `077CB` | 3 | `LD A,(nn)` | `stage` | read |
| `07E58` | 3 | `LD A,(nn)` | `stage` | read |
| `1B987` | 13 | `LD (nn),A` | `room` | write |
| `1B9BD` | 13 | `LD A,(nn)` | `stage` | read |
| `1B9CB` | 13 | `LD A,(nn)` | `room` | read |
| `1BB37` | 13 | `LD A,(nn)` | `stage` | read |
| `1BB48` | 13 | `LD A,(nn)` | `room` | read |
| `1BB9A` | 13 | `LD DE,(nn)` | `stage` | read |
| `1CE2B` | 14 | `LD (nn),HL` | `room` | write |
| `1D0E6` | 14 | `LD (nn),HL` | `room` | write |

## Suite de l’analyse

1. Désassembler les banques et suivre les appels vers le chargeur `4F83`.
2. Vérifier les écritures exécutées à `D000/D001` avec état du mapper.
3. Capturer le couple (stage,salle) avant/après une porte et relever la position de Simon.
4. Activer uniquement les liaisons confirmées dans le gestionnaire de salles.

Les transitions automatiques V27 restent des liaisons de démonstration.
