# LOT14G — réservation commune du corps15 et des butins

Base LOT14F vérifiée : snapshot1538 fichiers, ZIP42 contenus ; aucun indicateur
ni processus concurrent au démarrage. Instructions ROM intactes ; tous les
oracles chargent `probe.load_rom(probe.reference_rom())`.

## Origine et correction

Salle15/9 réellement chargée par le harnais Z80. B6AB/98E5/9E31,
9918/64EC/64E5 exécutés quatre fois, D010=1. Le corps16h occupe C8000,
12 entrées SAT0..11 ; les trois08h D7000..2 prennent17..12 par paires.
5F1D parcourt seulement0..13.99F6 libère réellement un projectile : libérer
D7000 ou1 laisse0..13 pleines ; libérer D7002 ouvre12/13.

Le registre SAT normal ignorait ces propriétaires ; de plus, le corps
stocké séparément laissait C8000 et D7000 disponibles dans le port.
`Enemies_boss15_reserve` conserve les masques de chaque propriétaire hors
EnemyState166, VKBoss192 et des autres structures imposées. Il utilise
le même sat_reserve que5F1D/9F6D, réserve C8000 et les indices D700 concernés.
Les masques persistent lorsque des trous apparaissent : aucun recalcul à
partir d'un simple nombre d'objets. L'allocation de boss15 appelle le registre
commun ; un slot D700 ordinaire occupé est sauté, une SAT insuffisante refuse
le tir. Le compteur CF20 reste incrémenté par BC29 même si le tir échoue.
Actors libère les propriétaires disparus avant/après la passe boss et avant
la création d'un butin. Les autres boss ne sont pas déclarés intégrés.

## Comparaisons contrôlées

`tests/run_boss15_pools.sh` : huit situations,360 images natives chacune,
2880 images. Aucun tir libéré, puis chacun des trois tirs libéré par99F6,
avec/sans ramassage. Vraie première bougie frappée par999F/7D68/8671 ;
`vk_candles_hit` reçoit le même indice. Carte réelle gardée vivante pendant
les appels ctypes (un premier harnais perdait sa référence au buffer ; son
échec est conservé, pas une divergence de la logique C).

Le boss est maintenu gelé ; le script B6AB n'est pas répété dans la continuation
pour garder ces trois propriétaires contrôlés.98E5/9E31,7E67/7FE2/8A4A
restent réellement exécutés. Entrées Simon/attaque, C003 et garde sont
explicites dans le script. Deux images natives par passe logique ; cette
preuve n'est pas un parcours naturel ni un ordonnanceur MSX complet.

À chaque passe : IX+00..1F des six emplacements C800 ordinaires1..6,
inactifs inclus ; inventaire et énergie de Simon ; sons ; coordonnées6445
du butin lorsqu'actif. Le corps C8000 et les tirs du boss ont une autre
représentation native ; leurs32 octets ne sont pas prétendus comparés ici :
les différentiels boss15 historiques sont rejoués par la suite complète.

C500 : après échec d'allocation, les16 octets bruts sont comparés. Après
atterrissage d'un butin C800, l'adaptateur historique utilise VKDrop :
allocation des huit slots, X/Y/type/compteur comparés, sans prétendre comparer
les octets sans correspondance. Le ramassage aérien, la persistance et le
transfert au sol sont couverts dans ces conditions bornées.

Le core SGDK frappe la vraie bougie15/9 au bouton B, sous fixture d'entrée,
avec trois tirs puis après libération du troisième. C8000 est réservé au
corps, butin en C8001 ; repli C500 vérifié dans le cas plein. Corps/butin/tirs
restent affichés selon la priorité vérifiée en14F ;360 images de continuation
par cas. Pour comparer C500 après les quatre images MD supplémentaires,
8A4A est appelé deux fois côté Z80. Une comparaison initiale non synchronisée
avait +6=0 côté Z80 contre2 côté MD ; aucun octet comparé n'est ignoré.

## Réserves maintenues — LOT14H

Cette livraison lève le FAIL précis LOT14F « première bougie, corps15 vivant,
trois tirs gelés », pas toute la possession SAT des boss. Restent à tracer et
intégrer : réallocations de corps/flammes/orbes et D7000C durant la mort,
alias de couleurs Y=E0 des boss, propriétaires physiques des boss12/18,
concurrence prolongée avec nouveaux tirs et autres objets dans ces phases.
La libération des masques se fait actuellement à la synchronisation actors,
pas à chaque instruction de la boucle Z80 : ordre fin à vérifier. Le compte
historique du boss15 reste présent pour ses limites isolées.

Restent aussi contacts09 des boss, couleurs de combat, sons mêlés aux
ramassages/IA ; puis LOT15 parcours naturels, LOT16 audit et régressions.
Aucune de ces réserves n'est supprimée par les nouveaux PASS.
