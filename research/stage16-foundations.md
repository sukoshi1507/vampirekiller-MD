# Stage 16 — extraction ROM MSX2 vérifiée (non jouable)

## Provenance et méthode

Données lues dans la ROM MSX2 de référence de SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`, avec `tools/build_stage16_foundations.py`, en réutilisant les décodeurs éprouvés du projet (`probe`, `extract_rooms`, `decode_room_links`). La ROM n'est pas incluse dans la livraison. Aucun résultat n'est inféré d'un autre Castlevania ou d'un autre stage.

## Données intégrées

- 10 fonds de salles, chacun composé de 768 indices de tuiles : 7 680 valeurs comparées aux sorties du décodeur sur la ROM.
- Table de navigation (pointeur CPU `BAF1h`), comprenant trois descentes sans retour correspondant : salle 1 → 7, salle 2 → 8, salle 3 → 9.
- Table brute des drapeaux de canaux d'apparition : dix octets à `00h`. Cela **ne prouve pas l'absence d'ennemis fixes**.
- Entrée brute de la porte de fin de stage : offset fichier `0x1BB91`, octets `85 40 0C`, salle 5, X 12, Y 64. Elle ne donne ni la clé ni la logique d'ouverture.
- Valeur brute du groupe de stage et métadonnées de cartes/palettes dans `stage16-foundations.json`.

## Fichiers

- `sgdk/src/generated/stage16_foundations.c` et `.h` : cartes et tables brutes, **non raccordées au gameplay**.
- `tools/build_stage16_foundations.py` : générateur ROM → tableaux C/JSON.
- `tools/verify_stage16_rom.py` : comparaison ROM contre JSON et tableaux C, cartes, liens, drapeaux, porte.
- `tests/test_stage16_foundations.c` et `tests/run_stage16_foundations.sh` : assertions C sur les fonds, liens, drapeaux et porte.

## Non portés et non validés

Collisions produites par `4F83`, objets et clés par `5A49`/`5B1B`, ennemis fixes par `6154`, comportements, animations, bruitages, passages particuliers et éventuel boss. Les routines n'ont pas été exécutées sur Z80 durant cette livraison. Aucune compilation SGDK ni émulation Mega Drive du stage 16 n'a été effectuée. **Le stage 16 n'est pas jouable et sa sortie depuis le stage 15 n'a pas été raccordée.**

## Reproduire les vérifications

Depuis la racine du projet, fournir personnellement la ROM conforme au SHA-256 ci-dessus (sans la redistribuer) :

```sh
python tools/verify_stage16_rom.py /chemin/vers/reference.rom
sh tests/run_stage16_foundations.sh
sh tests/run_native.sh
```

Le rapport JSON contient les offsets exacts et les SHA-256 de chaque carte.
