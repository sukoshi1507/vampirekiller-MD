# LOT12 : butins réservés et scénarios tardifs

## Exécution de référence

`tools/build_loot_data.py` charge exclusivement `probe.load_rom(probe.reference_rom())`. Le constructeur5F1D est exécuté pour24h et26h avec deux remplissages A5/5A : les champs écrits sont distingués des octets conservés. Les préfixes conservés sont des entrées partagées des tests, jamais une constante inventée. Aucune instruction ROM n'est modifiée. Les motifsE8/EC sont extraits après5768/5780 et vérifiés identiques sur19 stages. EC, et nonF4, représente la flamme26h.

`loot.inc` conserve la réservation Enemy de24h/26h jusqu'au ramassage, à la sortie d'écran99E5 ou à l'émission du blocC500 au sol. La mort1Eh portant+1F=1 devient24h après16 passes et effectue immédiatement99B9 avec vx0200/vy0080. Les passes suivantes exécutent les règles9BD9 : sonde du sol avant déplacement, oscillation avec accélérationFFE0, inversion toutes32 passes, fractions8.8. Pour26h,9C1E attend20 passes, puis vy0800 ; couleurs8,1,14,1 selonC003. Le constructeur initialise la couleur14.

9C03/9C4E effacent+00/+0E et émettent le sol : cœur (X&F8,(Y−16)&F8), flamme ((X−4)&F8,(Y−16)&F8). La vraie boucle exécute encore99B9/99E5 sur le préfixe qui vient de devenir inactif ; ce détail est conservé et comparé. Le C500 est créé directement au repos, sans seconde animation de chute. Son premier passage initialise83 sans diminuer la durée ; les suivants respectent la phase globaleC003. Le ramassage aérien utilise la classe2 de82CB (L8,H16), les gardes de7E67/7FE2 et le même objet d'inventaire8D30. Un seul ramassage par passe de deux images.

Le rendu passe par la banque4, tuiles1440..1455, sous la limite des plans1536 et au-delà de Simon. Les placements6445 sont X−4,Y−8 pour24h et X−8,Y−15 pour26h (Y SAT MSX inclut−1). Le cœur donne1 ; le grand cœur contenu02 donne5, son0F. Aucune disposition EnemyState166, VKGame36 ou VKCandles522 n'est changée. EnemyDrop reste6 octets, son nouvel indicateur landed utilise l'ancien octet de bourrage final.

## Preuves et périmètre

- `run_loot_trace.sh` :30 scénarios ×360 images =10800, cinq couples stage/salle,24h/26h, trois positions chacun ;32 octets, actifs et inactifs, après chaque passe.
- `run_item_trace.sh` : dix cycles intégrés ×360 images (sol, air, garde, invulnérabilité, recul),32 octets, position et nature du C500, durée, cœurs, positions SAT6445, couleurs et sons. Un cas à sept réservations pendant360 images compare le refus de création et les entrées SAT à5F1D/99F6/98E5. Ces3960 images ne démontrent pas toutes les saturations mixtes C800/D700.
- Les32 contacts prolongés des ennemis tardifs avec08/09/10/11 comparent désormais **tous** les préfixes C800, y compris les réservations24h/26h et les objets inactifs, sans l'exclusion LOT11.11520 images. Simon est fixe et les états de recul/invulnérabilité sont des entrées contrôlées ; pas une simulation complète de sa physique après dégâts.
- `run_loot_md.sh` : ROM SGDK, deux morts/constructeurs réels dans le moteur, maintien de l'emplacement en vol, ramassage aérien, cœurs+1/+5 et PSG0F ; fixtures RAM du monstre/Simon/fouet et R explicites.253 images : contrôle de branchement complémentaire, pas preuve prolongée indépendante.

Le pont32 octets reconstruit les champs du moteur et partage les octets non écrits de l'état initial réel. Il ne prétend pas que la représentation physique 68000 possède le format MSX. Les octets inutilisés retenus de tous les types antérieurs ne sont pas stockés intégralement par le moteur ; les champs lus par24h/26h sont initialisés par leurs constructeurs et comparés.

## Réserves conservées

Les cœurs/flammes issus des bougies utilisent encore la représentation historique VKDrop ; leur réservation C800 et toutes leurs boîtes de ramassage aérien ne sont pas couvertes par cette correction. Le chemin est marqué PROVISIONAL dans candles.c. Le pool natif de sept objets reste partagé avec les projectiles ; le stress LOT04 rejoué n'a pas observé de dépassement, mais ne prouve pas toutes les combinaisons des sept C800 et huit D700 MSX. Défaut20 images de gel lors d'un changement de salle tardif et cartes alternatives C41A : réserves LOT11 conservées.

Les scénarios zzp_stage12..18 ouvrent toutes les salles par le vrai menu de sélection, avancent300 images dans chacune, exécutent une transition horizontale, détruisent une bougie, tuent un ennemi et ouvrent une porte par les callbacks du moteur. Ils déclarent les fixtures RAM pour isoler les actions. Les portes12/15/18 rejoignent les boss ;13/14/16/17 rejoignent le stage suivant. Stage18 valide les deux formes, générique et retour cour, avec énergie du boss réduite et positions/fouet contrôlés. Ce n'est pas un parcours continu ni un combat naturel à énergie complète. La validation de tels parcours reste ouverte ; aucun score de complétion globale n'est revendiqué.

## Reproduction

```sh
export PYTHON=/workspace/vk-venv/bin/python
export PATH=/workspace/toolchains/mars/m68k-elf/bin:$PATH
export VK_MD_CORE=/workspace/emulators/genesis-plus-gx/genesis_plus_gx_libretro.so
$PYTHON tools/build_loot_data.py --check
$PYTHON tools/build_late_scenarios.py --check
sh tests/run_loot_trace.sh
sh tests/run_item_trace.sh
make -C sgdk GDK=/workspace/sgdk -j3
sh tests/run_loot_md.sh
sh tests/run_late_scenarios.sh
sh tests/run_native.sh
```

Les fichiers research/*-trace.json se régénèrent et sont exclus de la livraison. Les scripts de test ne changent aucune instruction ROM.
