# Ennemi 10h — calcul de portée et allocation, preuve statique (ROM)

Source : `probe.load_rom(probe.reference_rom())`, SHA-256 vérifié, fenêtres
CPU `8000h–9FFFh` banque 2 et `A000h–BFFFh` banque 3. Le script
`tools/verify_enemy10_helpers.py` confirme **39 instructions** par leurs
encodages originaux, les destinations de branche et évalue toutes les
65 536 paires de coordonnées 8 bits. Il **n'exécute pas** le Z80.

## Portée : `AF40h` (appelé depuis `AEB5h`)

- `AF40: LD A,(C427)` : X du joueur.
- `AF43: SUB (IX+05)` : différence à X de l'objet, sur 8 bits.
- `AF46: RET NC` : si le joueur est à droite de l'objet ou aligné,
  retourne directement la différence non négative.
- `AF47: NEG`, `AF49: RET` : sinon, renvoie son opposé modulo 256.

Pour X objet et X joueur chacun compris entre 0 et 255, le résultat est
exactement `abs(player_x - enemy_x)` (contrôle exhaustif). `AEB8: CP 2Ah`,
`AEBA: JR NC,AEDF` élimine les distances **>=42 pixels** du chemin
`AEBC→AEDC`. Sous réserve des gardes précédentes et du bit 0 de `IX+10`,
une distance de **0..41** permet la suite du chemin. Les chemins alternatifs
et la cadence globale restent à caractériser.

## Préparation du projectile : `9F61h–9F82h`

- `9F61..9F66` : `C=(IX+03)-10h` modulo 256 : Y initial présumé.
- `9F67` : `B=(IX+05)` : X initial présumé.
- `9F6A..9F6D` : sauvegarde `IX+00` en `CFF9h`.
- `9F70` : sauvegarde `BC` en `CFF1h` ; le registre B contient X et C Y.
- `9F74` : sauvegarde `HL` en `CFF5h` (sur le chemin `AEDCh`, `HL=0000h`).
- `9F77` : sauvegarde `DE` en `CFF7h` (sur le chemin `AEDCh`,
  `DE=0400h` ou `FC00h`).
- `9F7B..9F82` : sauvegarde et restauration de `IX` autour de
  `CALL 9F83h`.

Ces transferts sont établis. L'interprétation de `HL/DE` en vitesse, les
collisions, les sprites et l'état projectile final demandent l'analyse de
l'initialisation après `9FAAh` et une trace d'exécution.

## Recherche de case : `9F83h–9FACh`

- `9F83..9F94` : obtient un identifiant via table/routine (`A08Eh`,
  `4061h`) ou la valeur `0Ch` pour le cas `CFF9h=FFh` ; stockage `CFF0h`.
  La sémantique exacte de l'identifiant n'est pas démontrée.
- `9F97: LD HL,D700h` ; `9F9A: LD B,08` ;
  `9F9C: XOR A` ; `9F9D: LD DE,0080h`.
- `9FA0: CP (HL)` ; `9FA1: JR Z,9FA7` : recherche d'un premier octet
  égal à zéro parmi `D700h, D780h, D800h, D880h, D900h, D980h, DA00h,
  DA80h`.
- `9FA3: ADD HL,DE` ; `9FA4: DJNZ 9FA0` : prochaine case.
- `9FA6: RET` : toutes les cases non nulles, pas d'initialisation.
- `9FA7..9FACh` : le premier emplacement libre est chargé dans `IX`,
  et son adresse est sauvegardée en `CFF3h`.

La comparaison des premiers octets n'atteste pas que toute une case libre
correspond nécessairement à un projectile visible : la routine qui suit
reste à analyser.

Reproduction : `bash tests/run_enemy10_helpers.sh` et
`bash tests/run_native.sh`. Ces tests sont des **contrôles statiques**, pas
une trace image par image et pas une validation de portage en C.
