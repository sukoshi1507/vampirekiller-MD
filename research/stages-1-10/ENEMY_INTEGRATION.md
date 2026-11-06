# Intégration ennemis — premier lot

Cette version démarre l’intégration à partir de la ROM fournie. Elle ne constitue
pas un portage complet de tous les ennemis des niveaux 4–10.

## Intégré dans le moteur

- Table de drapeaux des 73 salles, issue de 85A6 en banque 14.
- Listes fixes décodées en exécutant la routine Z80 6154 ; 53 enregistrements
  conservés, y compris les types non pris en charge. Les coordonnées suivent
  le décodage de 61BB. Aucun remplacement arbitraire par un ennemi existant.
- Panthères : chargement depuis ces listes au lieu de la table limitée au niveau 1.
  Les trois du niveau 1 sont conservées ; trois sont ajoutées au niveau 3 :
  salle 0 à (112,128), salle 1 à (128,128) et (176,160). Salles numérotées depuis 0.
- Volants de type 4 : canal activé au niveau 10, salle 0, selon le drapeau 0x08.
  Le comportement et les graphismes existants sont réutilisés ; la fidélité
  graphique propre à ce niveau reste à vérifier en émulation.
- Type 1 au niveau 3, salle 3 : hauteur corrigée. 9D17 donne E=C0, et non A0
  comme au niveau 2 ; le haut du sprite MD passe donc de 128 à 160.
- Au niveau 1, le côté d’apparition lit l’orientation de Simon conformément
  à 9D30/C42C, au lieu d’alterner à chaque tir.
- Identifiant de salle des ennemis : espacement de 16 au lieu de 8, pour éviter
  qu’une salle 8 partage l’identifiant de la salle 0 du niveau suivant.
- Choix des tuiles homme-poisson par drapeau de salle, et non par numéro codé
  en dur. Les comportements des types 1, 2, 4 et 5 déjà présents sont conservés.

## Reste à porter

Les valeurs ci-dessous sont des identifiants ROM, pas des noms d’espèces déduits.
Certains enregistrements peuvent être des acteurs spéciaux plutôt que des ennemis.

| Niveau | Types fixes non implémentés (décimal) | Canaux non implémentés (masque hex.) |
|---|---|---|
| 3 | 31 | — |
| 4 | 6, 31 | 20 |
| 5 | 6 | 20 |
| 6 | 10 | 10, 20 |
| 7 | 11, 12, 13 | — |
| 8 | 10, 11, 12 | 20 |
| 9 | 10, 11, 13 | 20 |
| 10 | 33 | 04 |

Le bit 7 de certains drapeaux est conservé, mais n’est pas interprété comme un
canal supplémentaire. Les boss, projectiles spécifiques et objets spéciaux
restent à analyser et à porter. Les niveaux 4–9 ne deviennent donc pas peuplés
de tous leurs ennemis avec ce premier lot.

## Validation effectuée

- Régénération des données à partir de la ROM puis comparaison exacte : réussie.
- 73 salles × 600 images de logique native, avec leurs cartes réelles : aucune
  erreur AddressSanitizer/UndefinedBehaviorSanitizer détectée.
- Six panthères présentes aux coordonnées extraites dès la première image,
  dont les trois déjà existantes ; deux simultanées au niveau 3 salle 1.
- Apparition des volants dans les six salles marquées 0x08, dont niveau 10 salle 0.
- Génération des entrées de sprites, remise à zéro en quittant la salle,
  hauteur du type 1 au niveau 3 et indices invalides contrôlés.
- Tests des arrivées, escaliers, transitions et sorties toujours réussis.

Ces tests ne simulent pas une partie complète ni l’affichage SGDK. Aucun build
Mega Drive ou essai Kega Fusion n’a été effectué ici. L’animation, les palettes,
les dégâts, la synchronisation et les interactions des nouveaux placements
restent à valider en jeu. La logique des ennemis était déjà partiellement
provisoire dans la base, comme l’indiquent les commentaires d’enemies.c.

```sh
sh tools/test_enemies.sh
sh tools/test_room_transitions.sh
python3 tools/extract_enemies.py /chemin/reference.rom
```

La dernière commande nécessite z80==1.2.0. Ajouter --write pour régénérer le
fichier generated/enemy_rooms.h et le manifeste MSX2_ENEMIES.json.
La ROM n’est pas incluse dans cette archive.
