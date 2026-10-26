# VampireKiller V24 — Stabilisation de Simon

Base : V23, validée par l'utilisateur sous SGDK/BlastEm.

## Changement

La chronologie des 15 valeurs du compteur d'attaque est maintenant représentée par une table unique `vk_attack_keys` dans `sgdk/src/whip.c`. Chaque entrée associe la phase du bras, la composition visuelle debout et la composition visuelle accroupie. Les valeurs des séquences V23 et la géométrie de collision restent inchangées.

## Garanties testées

- 15/15 états de la chronologie : correspondance aux frames V23 ;
- index graphiques debout 0..7 et accroupi 0..4 ;
- segments visibles exactement pendant les fenêtres de collision ;
- attaque accroupie conserve sa hauteur et son orientation ;
- tests gameplay, physique, escaliers et cibles sans régression.

## Statut de fidélité

L'assemblage graphique provient des ressources ROM MSX2, mais l'ordre et les durées des poses ne sont **pas encore certifiés** par une trace dynamique openMSX. Le faux fouet segmenté reste une couche de rendu provisoire. Cette V24 clôt la **stabilisation du prototype jouable de Simon** sans prétendre que ses animations sont une reproduction exacte de la ROM.

## Tests exécutés

- `sh tests/run_native.sh` : PASS
- `python3 -m unittest discover -s tests -p 'test_*.py' -q` : 14 tests OK

Compilation SGDK et rendu BlastEm de cette V24 : non exécutés ici.
