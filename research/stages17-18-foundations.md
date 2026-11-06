# Stages 17 et 18 — fondations ROM, état exact (6 novembre 2026)

## Source de vérité et périmètre

Toutes les valeurs statiques ci-dessous proviennent **directement** du dump de *Vampire Killer* MSX2, authentifié par SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`. Le dump n'est **pas** inclus dans la livraison. `tools/build_stages17_18_foundations.py` utilise les décodeurs déjà présents (`probe.py`, `extract_rooms.py`, `decode_room_links.py`).

Le présent livrable **n'exécute pas de routines Z80** : il ne valide ni collisions, ni ennemis, ni objets, ni boss, ni fin de partie. Le module est volontairement indépendant du moteur, conformément à la demande de raccordement ultérieur. Il ne constitue donc **pas un portage complet et jouable** des stages 17 et 18.

## Stage 17

- **12 salles**, fonds de 768 indices de tuiles chacun, soit **9 216 octets** préservés à l'identique.
- Table navigation au pointeur CPU `BB05h`, désassemblée par le décodeur de liens préexistant.
- Liaison asymétrique vérifiée : **salle 10 LEFT → 11**, sans **salle 11 RIGHT → 10**.
- Drapeaux bruts de canaux d'apparition : `00 00 00 80 00 80 00 40 00 00 40 00` ; pointeur CPU `8652h`, à l'offset table `0x1C5C8`. Ces octets ne prouvent ni type ni nombre d'ennemis.
- Entrée de porte brute : offset fichier `0x1BB94`, octets `8B 40 0C`, soit drapeau `8Bh`, salle **11**, X **12**, Y **64**. La clé et l'ouverture ne sont pas extraites.
- Les fonds utilisent des indices de tuiles dans la plage couverte par l'extracteur graphique statique (0..191). Cela **ne garantit pas** le rendu final en émulation.

## Stage 18

- **10 salles**, fonds de 768 indices chacun, soit **7 680 octets** préservés à l'identique.
- Table navigation au pointeur CPU `BB1Dh`.
- Liaison asymétrique vérifiée : **salle 8 LEFT → 9**, sans **salle 9 RIGHT → 8**.
- Drapeaux bruts : `00 80 00 00 00 00 00 80 80 00` ; pointeur CPU `865Eh`, à l'offset table `0x1C5CA`.
- Entrée de porte brute : offset fichier `0x1BB97`, octets `88 80 0C`, drapeau `88h`, salle **8**, X **12**, Y **128**. **Ce n'est pas une preuve des règles de victoire ni des transitions de fin.**
- **Salle 9 : cas graphique non résolu.** Les indices de tuiles `192..245` sont présents dans la carte ROM, mais le décodeur existant ne charge que les tuiles `0..191`. La carte brute est conservée sans altérer ses octets ; l'aperçu produit remplace visuellement les tuiles inconnues par du magenta. Cet aperçu n'est **pas** une capture MSX2. Il faut tracer les chargements VDP/VRAM et les éventuels décors de fin avant de porter cette salle.

## Fichiers livrés

- `sgdk/src/generated/stage17_foundations.c/.h` et `stage18_foundations.c/.h` : **tables brutes uniquement**, pas de code de gameplay activé.
- `research/stage17-foundations.json` et `stage18-foundations.json` : hashes des salles, palettes, offsets et limitations.
- `research/stages17-18-previews/` : aperçus illustratifs des fonds statiques (salle 18/9 incomplète).
- `tools/build_stages17_18_foundations.py` et `tools/verify_stages17_18_rom.py` : génération et vérification octet par octet.
- `tests/test_stages17_18_foundations.c`, `tests/run_stages17_18_foundations.sh` : tests natifs ciblés.
- `LIVRAISON/STAGES17_18_ETAT_REEL.md` : synthèse des modifications et des résultats ; `LIVRAISON/STAGES17_18_DIFF_SHA256.json` : inventaire relatif à la livraison du stage 16.

## Reproduire

Depuis la racine du projet, en fournissant une copie personnelle du dump dont le SHA-256 correspond :

```sh
python tools/verify_stages17_18_rom.py /chemin/vers/VampireKiller.rom
sh tests/run_stages17_18_foundations.sh
sh tests/run_native.sh
```

Le fichier ROM ne doit pas être redistribué. Le générateur modifie les tableaux et inventaires ; vérifier de préférence l'état livré avant de régénérer.

## Travail indispensable pour finir les deux stages

1. Exécuter les routines originales `4F83` pour les **collisions** salle par salle, avec trace des effets VRAM et transitions.
2. Exécuter `5A49`/`5B1B` pour **bougies, coffres, objets, clés et portes** et les conditions d'ouverture.
3. Exécuter `6154` et les routines référencées par les canaux d'apparition pour les **ennemis, positions, trajectoires, dégâts et projectiles**.
4. Tracer séparément la **séquence de boss final et de fin de jeu** : initialisation, états, animations, collisions, musique, séquence de victoire. Ne pas déduire ces comportements des autres Castlevania.
5. Résoudre les tuiles VRAM `192..245` de la salle 18/9 depuis les écritures effectivement observées dans la ROM en exécution.
6. Intégrer les éléments démontrés dans le moteur SGDK, compiler puis confronter image et audio à des traces openMSX.
7. Ne raccorder la progression qu'après validation des éléments ci-dessus.

**Rappel de statut** : fonds et tables statiques vérifiés ; gameplay non porté, stages non jouables.
