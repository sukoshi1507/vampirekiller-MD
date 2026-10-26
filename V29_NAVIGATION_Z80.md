# VampireKiller V29 — Recherche des transitions originales

## Résultat

Le nouveau script `tools/audit_room_state_z80.py` analyse la ROM de référence vérifiée (SHA-256 contrôlé par `probe.load_rom`) et inventorie les **motifs binaires candidats** de lecture/écriture dans les variables du chargeur :

- `D000` : stage ;
- `D001` : salle.

Il détecte 62 motifs (9 écritures candidates et 53 lectures candidates). Le rapport complet est conservé dans `research/castle-navigation/z80-room-state-candidates.json` et `Z80_ROOM_STATE_AUDIT.md`.

**Important :** un motif brut n'est pas une instruction Z80 exécutée. Les banques peuvent contenir des données, et l'adresse CPU dépend du mapper. Il n'y a donc **aucune connexion authentifiée** par cette étape. Les liaisons du stage 1 restent celles du mode test V27, sans modification du gameplay V28.

## Usage

```sh
python3 tools/audit_room_state_z80.py "Vampire Killer (Japan, Europe) (En).zip"
```

## Validation

- 19 tests Python passés ;
- tests natifs de gameplay, physique, fouet et gestionnaire de salles passés ;
- SGDK / BlastEm non exécutés ici.

## Prochain point décisif

Tracer les écritures réellement exécutées vers D000 et D001 au franchissement d'une porte (avec valeurs avant/après, banque chargée, PC et coordonnées de Simon). En attendant, ne pas présenter les connexions horizontales expérimentales comme la topologie MSX2.
