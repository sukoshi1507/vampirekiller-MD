# LOT10 : objets, preuves et limites

## Exécution de référence

Tous les nouveaux harnais chargent `probe.load_rom(probe.reference_rom())`. Aucune instruction ROM n'est remplacée. Les ports BIOS/VDP/son du harnais TradeHarness isolent les sorties matérielles ; les instructions de jeu s'exécutent sur Z80. Les adresses ci-dessous sont les adresses CPU, avec les banques du jeu.

`build_item_data.py` exécute 8264 (jusqu'à 864B) pour les boîtes de projectiles et 5B1B pour les objets de chaque salle. `build_stage_map_md.py` extrait les 19 stages (0..18), avec `--check`. Les tables ne sont pas éditées à la main.

## Résultats comportementaux

- 44 prises d'objets (22 identifiants, C431.2 absent/présent), puis 260 passes chacune : 22 880 images. Comparaison inventaire, énergie/coeurs, compteurs, couleurs D400 de Simon et sons. Une arme secondaire réduit à 64 octets la zone de couleurs écrite ; la fin du tampon doit rester intacte.
- 0C : 6B39, deux sens avec/sans C431.3, 1 440 images, X/Y, fraction et compteur. Le recul passe de 2 à 2,5 pixels par passe ; aucune accélération de marche n'est déduite.
- 04 : 67 584 confrontations de géométrie exécutée 8264 ; types bruts 1..11, deux orientations, X sur tout l'octet et 12 Y. 8610 n'exige ni orientation du projectile ni état/immunité particuliers de Simon. Douze scénarios de 360 images (types3/4/9, 16/1/0 charges et objet absent), préfixes D700 de 32 octets, son0B, C701 et C441 comparés. Zéro charge sous-déborde à FF. La dernière charge retire l'icône sans interrompre le balayage déjà engagé. La flamme type12 n'est pas interceptée.
- Les cinq familles de boss utilisent aussi la disparition type12. Le harnais contrôle les préfixes de disparition sur toute la durée de la flamme, sans extrapoler une nouvelle IA de boss.
- 08/09/10/11 : contacts isolés, IA figée, sur les types09/12/0B/0D/0E/10/0F/23. Énergie, score, sons, dormance09, flammes et événement d'objet lâché comparés selon le cas. 08 ne détruit pas, les bibles ne donnent pas d'immunité. 09h ennemi retourne à la pose26 avec délai64 et conserve ses vitesses. Corrections des décalages de mort10/12 et des positions des quatre flammes de dragon (position source sauvegardée avant réemploi du slot).
- 0E : 870C, 92 blocs sur 156 salles, 94 208 pixels de contours comparés. Le rendu MD utilise trois tuiles libres, sans agrandir les structures de persistance. Les contours sont masqués pendant F2 et coupés autour de la fenêtre marchande.
- F2 : 9552 et dessin ROM, 4 088 448 pixels de 156 salles. La copie HMMC des petits marqueurs arrondit X au pixel pair : correction de l'ancien décalage d'un pixel. Six scénarios de 360 images du contrôleur : phases panneau/dessin/fermeture, troisième utilisation affichée même si l'inventaire disparaît, quatrième refusée, conditions de salle/boss. Le monde reste figé mais C003 avance.
- 1E : eau bénite, pas révélation. 7116/72B2, cinq scénarios de 360 images : deux orientations, posture refusée, coeurs insuffisants, arme absente. Comparaison des deux préfixes de16 octets C450/C460 et des coeurs. L'animation doit suivre C003 global, y compris après une pause par F2 ; ajout d'un adaptateur d'horloge sans modification de structure.
- 06 : 75C0 s'exécute avant 5EB8 ; le canal voit le compteur après décrément. Le passage1→0 autorise déjà les apparitions (149 passes bloquées pour une valeur150). 156 scénarios tardifs de1 000 images, durées ordinaires/longues, pause et arrêt du temps. L'ancien harnais remplaçait ED5F (LD A,R) par une constante ; il est remplacé : observation après la vraie instruction B216/B29E, puis rejeu des valeurs R comme entrées du C. Consommation exacte vérifiée. Le code de production conserve son générateur de hasard, ce rejeu est un point d'entrée de test à la compilation.

## Limites explicites

La capacité du pool natif générique reste celle du LOT04 : sept objets partagés, contre sept C800 et huit D700 sur MSX. Ce lot ne démontre pas l'équivalence pour toutes les saturations possibles. Pour le bouclier générique, les traces longues utilisent D700 slot0 ; la relocalisation de la flamme vers un trou antérieur dans D700 n'est pas implémentée/provée. Le code est marqué PROVISIONAL. Les pools boss/Dracula conservent leurs mécanismes de disparition propres.

Le chargeur général des bougies/blocs tardifs est au LOT11. Les contours et cartes connaissent leurs données initiales ; les blocs muraux bénéficient de la persistance du LOT08. La persistance après destruction d'un autre bloc tardif reste provisoire dans item_effects.c. Aucune traversée complète de tous les stages n'est annoncée.

Les 32 contacts tardifs sont des transitions instantanées avec IA figée, pas des scénarios complets de300 images par ennemi après ramassage du butin. Les minuteries/couleurs sont couvertes séparément sur520 images par cas ; les IA individuelles relèvent des tests antérieurs rejoués. Le préfixe32 complet est comparé pour les projectiles ; les contacts de mort vérifient les champs et sorties listés, pas tous les octets inutilisés de C800. La réduction d'énergie par armes de l'ennemi09 n'est pas nouvellement prouvée ici. Ces limites interdisent de déclarer le LOT10 intégralement clos.
