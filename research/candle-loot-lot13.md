# LOT13 : butins des bougies raccordés au pool

## Source exécutée

Tous les essais utilisent `probe.load_rom(probe.reference_rom())`, dans l'arbre cumulatif LOT12. Aucun opcode ROM n'est écrit. Le coup réel7D68 puis8671 détruit la bougie et met à jour son flux.898F applique8A29 : arme1A..1D déjà tenue → petit cœur.89BF ajoute8 à X et16 à Y, puis appelle5F1F avec B=contenu :1E pour le petit cœur,26 pour les autres objets positifs. Un constructeur réussi rend CF31=1 et supprime tout C500 immédiat. Les objets00/15 restent sur leur chemin C500 préexistant, les blocs cachés ont leurs chemins89E3/89FD/8A13 distincts.

`build_loot_data.py` étend les masques exécutés5F1D à1E, avec sentinellesA5/5A ; generated est régénéré, jamais édité à la main. Le port utilise `Enemies_candleLoot` et un adaptateur `vk_candles_airborne` enregistré par `vk_reset_enemies`. Tous les démarrages du moteur le raccordent. L'API8992 (`vk_candles_drop`, arme secondaire déposée) utilise aussi89BF, sans la conversion8A29, comme la ROM. L'API décor isolée sans adaptateur conserve sa façade historique pour ses tests ; **ses anciens tests ne prouvent pas le pool C800**. Le nouveau pont active obligatoirement l'adaptateur et vérifie explicitement l'absence de C500 à la naissance réussie.

La flamme1E conserve son emplacement, dure16 passes puis recrée24.26 garde son emplacement, ses20 passes d'attente, son état, sa chute et son contenu. Les règles9BD9/9C1E/99B9/99E5, contacts7E67 et rendu6445 reposent sur le module du LOT12. Mouvement et contact positifs sont maintenant sur la même phase globale : un nouveau cœur ne peut plus être pris avant son prochain déplacement de référence. La prise donne le vrai contenu à `vk_candles_give`, puis les effets et sons du moteur. La chute au sol émet directement C50083 sans seconde flamme ; phase globaleC003, durée, position et nature sont comparées.

## Réallocation et ordre

9B90 libère le bloc de la flamme puis appelle5F1D : un emplacement antérieur libéré entre-temps gagne. L'objet24 ne reste donc pas forcément à l'ancien IX. De plus,5F1D change IX ;98E5 continue après le nouvel IX et repasse sur les objets intermédiaires de la même passe. Un cas avec trois flammes longues, une flamme portant un cœur et un trou libéré après8 passes compare les sept préfixes32 octets pendant360 images. Le port réalloue et reproduit les repasses pour flammes/butins. **Les repasses impliquant d'autres IA et les projectiles restent à prouver au LOT14**, les anciens passages par type n'étant pas un parcours C800 général. Le code le signale PROVISIONAL.

## Refus d'allocation : vrai secours C500

Un échec89BF rend CF31=0 ;898F continue vers8A37. Le butin n'est pas perdu tant qu'un C500 est libre. `candle_fallback.inc` porte ce chemin :01 compte16 passes par8A99, devient82, puis83/84 après8AF8/8B70 ; ramassage8BDF et disparition8BC7. Pour le petit cœur,8B22 utilise les fractions, l'accélération, la phaseC003 et les champs C500 propres, distincts des règles24h. Les autres contenus tombent de8 pixels par passe. Un C50083 se positionne sur la grille, initialise sa durée, puis84 la diminue surC003&15=0. Les16 octets sont comparés, y compris ceux retenus après disparition.

La synchronisation `vk_candles_sync` reçoit le vrai tick et l'indicateur de passe du moteur. `vk_candles_fallback_sprite` reproduit8C60..8CBC : F4/F8 et rouge pour le petit cœur en attente, E8 ou EC et couleurs8/1/14/1, coordonnées exactes ; les états cachés ne sont pas remplacés par une image visible. Le rendu réutilise la banque du LOT12, tuiles1440..1455, et les flammes existantes. Les sprites visibles sont comparés aux écritures réellesD628/D4A0 ; le repos84 est dessiné par le rendu d'objet au sol préexistant.

Deux motifs de refus sont testés : sept C800 occupés par des flammes longues ; ou14 entrées SAT occupées par dragon8 + sauteur4 + flamme2, avec des C800 encore libres. Chaque motif est suivi pour les contenus01/02/03/13/1A, au sol et en ramassage aérien,360 images par scénario. Les gardes de Simon sont des entrées contrôlées ; pour le motif SAT, C43A est maintenu non nul pour isoler les objets des contacts hostiles. Les sept préfixesC80032 et C50016, inventaire, énergie/armure et sons sont comparés. Un cas avec sept C800 et huit C500 déjà pleins prouve qu'aucun objet de remplacement n'est inventé.

## Couverture

- 622 scénarios sur299 bougies positives réelles des stages0..18 : deux situations sol/air par sortie, plus les armes déjà tenues.252 concernent12..18. Chacun360 images,32 octets C800 actifs et inactifs, champs C500 position/nature/durée/existence, drapeau de flux, Simon/inventaire, sons, SAT et couleur.
- 16 scénarios depuis89BF avec garde, recul, invulnérabilité ou attaque, aux phasesC003 initiales15 et31,360 images chacun. Les entrées de Simon sont fixées et communes : ce n'est pas une simulation de sa physique complète après dégâts.
- 1 scénario de réallocation antérieure et repasses,20 de secours C500,1 où les deux pools sont pleins.
- Total660 scénarios,237600 images (118800 passes de référence, deux images natives par passe). Les traces JSON se régénèrent par le test et ne sont pas livrées.
- Contrôle core : vrais coups B sur les bougies12/0 et13/0, naissance C800 sans doublonC500, prise aérienne+1/+5 et son0F ; refus C800 avec le même vrai coup et prise du secoursC500 ; continuation360 images après chaque prise. Les positions, l'invincibilité, le gel de l'IA, l'inventaire et l'occupation du pool sont des fixtures RAM déclarées, aucune instruction modifiée.

## Dispositions et limites

EnemyState166, VKGame36, VKCandles522, VKItems20, VKWall693/shop348, VKBoss192 et VKSecret8 inchangés ; EnemyDrop6 inchangé. L'adaptateur et les préfixes privés du secours sont hors des structures publiques. Les vieux offsets restent valides. Les champs non écrits des préfixes sont des entrées communes du pont, pas une affirmation d'identité physique des structures68000/MSX.

Le pool partagé natif de sept Enemy contre sept C800 plus huit D700 reste une réserve LOT14. Les essais de ce lot sont isolés et ne prouvent pas toutes les situations mixtes, les allocations SAT partielles, les constructeurs imbriqués, les repasses d'autres IA ni les bougies pendant tous les combats de boss. L'allocation5F61 écrit des réservations SAT au fil de sa recherche ; une preuve générale ne doit pas se réduire à un compteur atomique. La création de l'aigle appelle celle de sa puce et peut modifier CF31 : tenir compte du constructeur complet dans l'étude suivante.

Le code de la façade décor sans adaptateur reste historique ; seule la chaîne raccordée par le moteur bénéficie des nouvelles preuves. Les objets00/15, coffres et blocs cachés ne sont pas redéfinis ; leur suite antérieure est rejouée. Parcours continus et combats complets (LOT15), gel tardif par défaut20 images et cartes alternativesC41A (LOT16) restent ouverts. La fermeture globale du LOT10 n'est pas revendiquée avant étude des interactions mixtes.

## Rejouer

```sh
export PYTHON=/workspace/vk-venv/bin/python
export PATH=/workspace/toolchains/mars/m68k-elf/bin:$PATH
export VK_MD_CORE=/workspace/emulators/genesis-plus-gx/genesis_plus_gx_libretro.so
$PYTHON tools/build_loot_data.py --check
sh tests/run_candle_loot_trace.sh
make -C sgdk GDK=/workspace/sgdk -j3
sh tests/run_candle_loot_md.sh
sh tests/run_native.sh
```
