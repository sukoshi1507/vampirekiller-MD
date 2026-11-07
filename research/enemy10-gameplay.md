# Ennemi 10h exécuté : ADDE / ADF8

Voir LIVRAISON/ENNEMI_10H_BILAN.md pour la portée exacte et les commandes.

Le C est dans sgdk/src/enemies_type10.inc, inclus par enemies.c. La disposition EnemyState est conservée.

| Octets Z80 | Champs C |
| --- | --- |
| +01 état | Enemy.state (source) |
| +02/+03 Y | Enemy.sub / Enemy.y |
| +04/+05 X | Enemy.timer / Enemy.x |
| +06 mobile | S8Ext.mobile |
| +07/+08 vy | Enemy.vy |
| +09/+0A vx | S8Ext.vx |
| +0B pose | Enemy.pose |
| +0D HP | Enemy.extraPhase (masque HP existant) |
| +10 mode de distance | S8Ext.c10 (source) |
| +11 délai de tir | S8Ext.c11 (source) |

Projectile 09h : S8Ext.c10 indexe l'emplacement D700 ; c12 conserve +0C (animation persistante), c11 contient +13 (délai 24), c18 désigne le lanceur. Enemy.state conserve +10 (signe initial de vx). La pose commence à 3 puis devient 63..66 par 9EC5.

Le test reconstruit les 32 octets pour les comparer sans exclure de champ. Les champs inutilisés restent à zéro dans les créations fraîches de ces scénarios. Les pointeurs du projectile vers C800 sont traduits par le bridge pour la comparaison avec le lanceur isolé du scénario. La trace n'est pas une sauvegarde réinjectée dans le C.

Les entrées sprite sont comparées après la routine Z80 de dessin 6445, avec le décalage Y matériel de -1 par rapport à EnemySprite. Aucune image graphique n'est utilisée comme oracle.

Test : tests/run_enemy10_trace.sh. Il relit la ROM locale par probe.load_rom(probe.reference_rom()) et exécute les instructions originales, sans signatures statiques comme preuve de comportement.
