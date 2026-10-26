# V30 — Analyse contextuelle des transitions Z80

Cette étape complète l'inventaire brut V29 par un classement conservateur des neuf écritures potentielles vers D000 et D001.

## Réalisé
- Nouveau script `tools/classify_room_transitions_z80.py` utilisant la ROM de référence validée par SHA-256.
- `research/castle-navigation/Z80_ROOM_WRITE_TRIAGE.md` : explication contextualisée de chaque occurrence.
- `research/castle-navigation/z80-room-write-triage.json` : résultats machine-lisibles.
- Deux tests de non-régression ajoutés.

## Constats
- `00DFA`, `00E07`, `01E59` et `030D6` : séquences de réinitialisation plausibles.
- `1B987` : écriture candidate de sélection de salle, à suivre en priorité avec le flot d'exécution et le mapper.
- `1CE2B` et `1D0E6` : deux faux positifs probables dans des données.

**Aucune connexion originale entre salles n'est encore prouvée.** Les connexions expérimentales du mode test restent inchangées. Aucun résultat dynamique openMSX n'est revendiqué.

## Tests exécutés
- `python3 -m unittest discover -s tests -p 'test_*.py' -q`: 21 tests OK.
- `sh tests/run_native.sh`: OK.

Compilation SGDK et émulation BlastEm de cette livraison : non exécutées ici (aucune modification du gameplay).
