# Z80 — tri contextuel des écritures de stage/salle

Ce rapport va au-delà du balayage binaire en examinant les instructions voisines.
**Aucune destination de salle n’est pour autant authentifiée.**

| Offset ROM | Type probable | Confiance du contexte | Raisonnement |
|---|---|---|---|
| `00332` | `stage_write_candidate` | medium | conditional branch; LD (D000),A writes FF following CP 06; no room destination established |
| `00DFA` | `initialization` | high | XOR A followed by a series of zero stores to C007/CF3D/D001/D002, then INC A and store D000 |
| `00E07` | `initialization` | high | same contiguous initialization block as 00DFA: stage reset to 1 |
| `01E3D` | `stage_select_candidate` | medium | CP 13, JR C, INC/zero A then LD (D000),A; stage boundary clamp candidate |
| `01E59` | `room_reset_candidate` | high | LD A,(D000), indexed read through 4061, LD (D002),A then XOR A and LD (D001),A |
| `030D6` | `room_reset_candidate` | high | XOR A and repeated store-zero sequence including (D001) |
| `1B987` | `room_selection_candidate` | medium | AND 0F / CP 0F / RET Z precede LD (D001),A; carry set before RET |
| `1CE2B` | `table_false_positive_likely` | low | region consists of patterned pointer/data words, 22 01 D0 overlaps table entries |
| `1D0E6` | `table_false_positive_likely` | low | region contains repeated patterned nibble values and 22 01 D0, not verified code |

## Conclusion pratique

- `00DFA`, `00E07`, `01E59`, `030D6` correspondent à des séquences de réinitialisation plausibles, et non à des arêtes prouvées.
- `1B987` est la piste la plus utile pour une sélection effective de salle, mais demande le suivi du flot et du mapper.
- `1CE2B`, `1D0E6` doivent rester exclus des écritures fiables tant que ces plages de données ne sont pas prouvées exécutables.
- Ne pas remplacer les connexions expérimentales par ces hypothèses.

## Capture dynamique requise

Tracer `D000`, `D001`, PC et les banques actives sur chaque écriture ; relever l’état précédent, le suivant, ainsi que la position du personnage.
Vérifier les effets de portes et escaliers en jeu avant d’inscrire un lien dans le graphe.
