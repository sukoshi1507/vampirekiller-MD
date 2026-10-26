# Développement expérimental : salles, escaliers, animation et fouet

## Provenance et niveau de vérification

- `generated/stage1_collision.c` contient 8 cartes du stage 1. La salle 0 conserve les 768 octets de `vk_player_room`, vérifiés contre la ROM lors de l'extraction antérieure. Pour les salles 1-7, seules les 704 tuiles visibles ont été réutilisées depuis `generated/rooms.c`; les 64 tuiles masquées supérieures sont **inconnues et mises à zéro**. Ces cartes ne sont donc pas des reconstitutions exactes des 768 octets de la RAM MSX2.
- Les états de marche et de saut utilisent toujours les 8 poses existantes, dont la provenance est dans `research/player-format.md` et `assets/player/manifest.json`. **Aucune pose authentique supplémentaire (fouet, accroupissement, escalier) n'a été extraite**, car la ROM MSX2 brute ne figure pas dans cette archive. Les poses 6/7 servent encore temporairement aux escaliers.
- Une seule zone d'escalier *factice* est volontairement activée pour les essais : salle 1 du stage 1, axe x=124..132, pieds y=104..192. Elle n'est pas issue d'une table MSX2 et n'est pas une affirmation quant au placement original.
- Le fouet est **un prototype gameplay**, pas une restitution Z80 prouvée : touche B, animation de 14 ticks dont une fenêtre active (timer 3..10), collision rectangulaire `vk_whip_bounds`, 3 segments 8px. La collision est exposée pour les futurs ennemis/objets, qui ne sont pas encore implémentés.

## Essai en jeu

1. Lancer l'intro, traverser la cour et entrer dans le hall avec HAUT devant la porte.
2. Dans le hall, maintenir A et appuyer sur DROITE/GAUCHE pour visiter les salles 0 à 7 du stage 1. **C'est une navigation de débogage, pas le cheminement original du jeu.**
3. Dans la salle 1, approcher x≈128 et maintenir HAUT/BAS pour tester la montée/descente dans la bande expérimentale (sans marqueur graphique fiable).
4. Utiliser B pour attaquer avec le fouet, C pour sauter, BAS pour s'accroupir. La touche START retourne au titre.
5. Depuis le titre, A ouvre toujours l'explorateur des 18 stages ; B dans l'explorateur lance le mode de test original, où A déclenche le fouet et B retourne à l'explorateur.

## Recommandations pour la vraie rétro-ingénierie

1. Fournir la ROM MSX2 originale **acquise légalement** dans l'environnement d'extraction ; ne pas l'incorporer à l'archive distribuée.
2. Rejouer `tools/extract_rooms.py` / `tools/extract_player.py` pour les cartes de 768 octets dans chaque salle et retrouver les tables d'objets/escaliers, plutôt que déduire un escalier d'un motif visuel.
3. Tracer les transitions d'état des escaliers et de l'attaque dans l'émulateur Z80 ; comparer les séquences d'entrées image par image.
4. Étendre la table `POSES` d'`extract_player.py` aux motifs d'attaque et d'escaliers, uniquement après avoir vérifié leurs offsets et leurs coordonnées dans les tables 0xA281/0xA2D1 et 0x7985/0x79D5.
5. Remplacer la géométrie de démonstration et les segments graphiques temporaires par des ressources validées.

## Tests

`sh tests/run_native.sh` : tests de transitions, de physique, et nouvelle séquence de fouet. Ils ne remplacent pas une compilation SGDK et des tests en émulation ; aucun environnement SGDK n'est installé dans ce conteneur.
