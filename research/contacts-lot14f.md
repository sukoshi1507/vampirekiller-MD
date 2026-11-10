# LOT14F — contacts09 des deux pools et rendu boss/butin

Base LOT14E final :1503 fichiers snapshot et39 contenus ZIP vérifiés,
aucun autre travail actif. ROM exclusivement
`probe.load_rom(probe.reference_rom())`, instructions intactes.

## Scores et sons de l'objet09

7E67 traite C800 puis7EE4 traite D700.7EB6 appelle81AB,7EB9/9A3E
puis7EBC demande0D pour l'ennemi.7F10/99F6 supprime le projectile et
7F18 saute à822A/44EC :100 points, sans requête de son.

Le port écrasait contactScore au second pool. Contre-exemple : un ennemi09
actif (200 points) et un projectile (100 points) donnent300 dans le Z80,
100 dans le port. De plus, un projectile seul déclenchait indûment0D.
Les points sont maintenant additionnés ; D700 ne demande plus0D.

75ED peut demander17h (23 décimal) avant le contact. Le champ unique
itemSound perdait cet avertissement si C800 demandait ensuite0Dh (13).
Deux requêtes sont conservées hors EnemyState166 et actors.c les transmet
séquentiellement. itemSound reste le dernier événement pour les anciens
harnais. Le nombre maximal des deux chemins contrôlés est2 ; un dépassement
est une erreur interne, jamais une troncature silencieuse.

Attention : requêtes et effets acceptés sont distincts.509F réellement
exécuté, sans son hook, conserve C096=23 après les requêtes23 puis13 ;
5172 refuse13 car sa priorité est inférieure. Le core vérifie23 accepté,
une seule acceptation et la file[23,13]. Une première assertion du nouveau
harnais attendait à tort13 ; son échec est conservé, l'assertion remplacée
par comparaison au vrai C096, sans retrait du contrôle des deux requêtes.

## Différentiel borné

24 situations :0/1/2 ennemis09 actifs, aucun projectile ou03/04/09 D700,
avertissement C43A absent ou initialisé17.360 images natives chacune,
8640 images. C434 est activé par8D30 et Enemies_item ; sa durée n'est pas
inventée. C002 bit6 active le score. Entrées communes : Simon(88,172),
D010=1, canaux bloqués. Les objets09 sont activés parAF5F, pas par une
instruction ROM modifiée. Les résultats incluent les états dormants.

Harnais synchronisé : contacts chaque image native, timers/98E5/9E31 une
image sur deux. Cette ordonnance contrôlée ne prouve pas la cadence de la
boucle MSX complète. Les écarts du premier contact ne dépendent pas de cette
cadence. À chaque image :7 C800+8 D700,32 octets actifs/inactifs, points
BCD, CF40 conservé, timers, sons ; sprites6445 des corps et128 couleurs
D400 de Simon aux passes de palette. La preuve ne couvre pas tous les types
C800 en contact mixte : les anciennes preuves isolées restent rejouées.

Core13/3 :300 points + file[23,13], puis projectile seul100 sans requête,
360 images de continuation pour chaque cas. Fixtures RAM, pas parcours naturel.

## Boss15 : écart réel et correction de rendu

La salle15/9 contient quatre vraies bougies (contenus02/07/02/02).
B6AB/98E5/9E31 crée le corps16h (12 entrées SAT) et trois projectiles08h
(6 entrées). Les18 entrées sont occupées. Frapper la première bougie par
999F/7D68/8671 échoue à réserver C800 et crée un repli C500.
Le port crée à tort un butin C80026 : les réservations de boss et ordinaires
ne sont pas partagées. Ce défaut reste **FAIL reproduit**, pas résolu.

Le renderer avait un autre défaut : il ne dessinait le boss que si la liste
ordinaire était vide. Lorsque le butin est légitime (troisième projectile
libéré), le Z80 permet C80038 à côté du corps. Le port faisait disparaître
le boss. Le renderer combine maintenant corps, butin puis tirs, en respectant
la priorité C800 du corps au stage15. Le cas libérant explicitement le
projectile2 par99F6 côté Z80 et son champ active côté MD passe :8 sprites
avant le butin,9 ensuite, et minimum8 durant360 images de continuation.
Le partage SAT et l'ordre de toutes les allocations restent à traiter.

`reproduce_boss15_pool_gap.py --free-third` vérifie cette coexistence et est
appelé par la suite core. Sans option, le programme retourne1 avec le rapport
FAIL de réservation. Il ne transforme pas ce défaut en assertion PASS.
Positions de Simon, gel et libération du projectile sont des entrées
contrôlées ; aucune traversée naturelle revendiquée.

## Suite nécessaire

LOT14G : allocation partagée/indices physiques C800 et D700 des boss,
notamment15/9 plein puis partiellement libéré ; persistance C500/C800 et
ordre SAT. Vérifier aussi12/18, interactions09 des boss, couleurs modifiées
en combat et interactions de sons avec ramassages/IA. Ne pas déduire ces
règles des24 cas d'ennemis09 ordinaires. Puis LOT15/16 ; LOT99 différé.
