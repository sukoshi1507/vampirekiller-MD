# Refactor du moteur — octobre 2026

## Fichiers modifiés
- `sgdk/src/gameplay.h`, `gameplay.c` (nouveaux) : routage des salles et événements de progression (`VK_EVENT_ROOM`, `VK_EVENT_ENTER`) ; contexte `VKGame` testable sans SGDK.
- `sgdk/src/main.c` : boucle de rendu/contrôles simplifiée ; les transitions de cour ne réinitialisent plus le personnage ; entrée de porte limitée à x=192..216 ; intégration de `VKGame`.
- `sgdk/src/player.c` : contrôle de collision du sommet du personnage en montée de saut ; arrêt de la montée et passage en chute lorsqu'un plafond est rencontré.
- `tests/test_gameplay.c` et `tests/run_native.sh` : tests natifs de routage de salle, conservation du saut, absence de voisin et entrée du château.

## Limite importante
La scène intérieure `vk_scenes[6]` n'a pas de carte de collisions dédiée dans les ressources générées. `vk_game_collision()` utilise donc temporairement `vk_player_room` (stage 1) **en tant que carte de secours non vérifiée** (`collision_verified == 0`). L'interface le signale. Une véritable extraction de la carte de collision intérieure reste indispensable avant de qualifier cette scène de jouable et fidèle au jeu original.

## Validation
`tests/run_native.sh` compile sous GCC avec `-Wall -Wextra -Werror` et exécute les tests de logique de salle. Une compilation de ROM avec SGDK ainsi que des tests en émulateur restent nécessaires. Les ROM précompilées fournies sont des anciennes versions et **ne contiennent pas ces correctifs**.

## Construire
- Tests : `sh tests/run_native.sh`
- ROM : `cd sgdk && make GDK=/chemin/vers/sgdk` (installation SGDK requise)

Les fichiers SGDK préexistants (Makefile, boot et header) sont conservés tels que reçus, même si leur état Git local différait du dernier commit.

## Extension — hall et physique de Simon

- Le hall `vk_scenes[6]` est charge par `extract_intro.py` avec `D000=1, D001=0` ; `vk_player_room[768]` est la carte du **stage 1 / salle 0**, generee par `extract_player.py` et `decode_room`, pas un fond substitue. `collision_verified=1` pour cette scene. Le generateur de l'introduction compare maintenant les 768 octets du tampon Z80 `D100:D400` avec le decodeur independant (necessite la ROM source pour regenerer).
- `vk_player_tick_map` traite maintenant les deplacements verticaux pixel par pixel, avec collision au plafond, atterrissage sur la premiere ligne solide, transition explicite saut vers chute, et conservation des tables originales de deltas.
- Les fonctions primitives originales de collision sont inchangees.
- `tests/test_physics.c` couvre le saut, le plafond, l'atterrissage depuis une chute rapide et les murs ; `tests/run_native.sh` teste aussi le routeur.

**Limites :** physique C de prototype renforcee, pas encore une reproduction cycle-perfect de la machine d'etats Z80 (escaliers, coups, etats de degats, objets, mort, transitions verticales). La compilation SGDK et la nouvelle ROM ne sont pas effectuees ici.

## Iteration suivante — accroupissement et infrastructure escaliers

- `VKPlayer` conserve `crouched` et `on_stairs`.
- `vk_player_tick_controls` prend HAUT/BAS et des volumes `VKStair` explicites.
- BAS au sol interdit le déplacement et le saut ; les primitives de collision
  `vk_wall(..., crouched=1)` restent inchangées pour la vérification Z80.
- Dans un volume autorisé, HAUT/BAS déplacent Simon d'un pixel par frame ;
  C quitte l'escalier en sautant. Hors volume, aucun escalier n'est inventé.
- `vk_game_tick_controls` transmet les directions verticales ; l'ancien
  `vk_game_tick` reste disponible pour compatibilité des tests existants.
- Tests natifs étendus : accroupissement, sortie d'accroupissement, entrée,
  montée et sortie de l'escalier, refus des escaliers non déclarés.

**Limites** : les coordonnées effectives des escaliers du jeu MSX2 et les
poses graphiques correspondantes ne sont pas encore extraites/validées ;
le rendu utilise temporairement une pose existante. Aucun escalier n'est
activé dans les salles du parcours, car leur géométrie ne peut être déduite
sans ambiguïté des seules cartes solides. La recompilation SGDK et les tests
émulateur restent à faire côté utilisateur.
