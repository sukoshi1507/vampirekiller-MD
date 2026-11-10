# LOT14D — repasses panthère/poisson3 et première image des tirs

Base LOT14C final :1445 fichiers snapshot et25 contenus ZIP vérifiés,
aucun indicateur ou processus concurrent. Exclusivité par LOT14D_EN_COURS.
Référence `probe.load_rom(probe.reference_rom())`, instructions intactes.

## Repassage réel98E5 après9B90

Un premier bloc est réservé par une flamme1E, un deuxième contient une
panthère05 ou un poisson21h/03 ; un troisième contient une flamme portant
un cœur. Après libération contrôlée du premier bloc au passage8,9B90 crée
le cœur dans ce premier bloc au passage15.98E5 reprend après le nouvel IX
et réexécute l'IA du deuxième objet. Les anciennes boucles séparées du port
ne la réexécutaient pas.

Contre-exemple panthère mobile au passage15 : port X96/pose41/compteur3,
Z80 X92/pose42/compteur4. Le journal antérieur est conservé dans
validation/lot14d/panther-before.log. Les mises à jour panthère et poisson3
sont maintenant dans le parcours commun C800, avec la même phase de base
mais sans incrément de phase supplémentaire lors d'une repasse.

Les canaux et listes fixes peuvent créer un poisson après ce parcours :
seul ce nouveau poisson reçoit sa première mise à jour dans type2Frame.
Le champ natif age, auparavant inutilisé pour fish3, sert de marqueur0/1 ;
la disposition18 octets et les offsets publics sont conservés. Ce marqueur
n'est pas le compteur Z80+1D, conservé séparément dans Fish3Ext.

## Tir déplacé deux fois

Le chemin A492→9F6D construit le projectile ;9E31/99B9 le déplace après
la boucle C800. Le port faisait un mouvement dans fish3Fire, puis un second
dans la boucle D700 lors des deux images de la même passe.
Contre-exemple à gauche depuis X192 : port186, Z80189 (trois pixels en trop).
Voir shot-before.log. Le constructeur natif laisse maintenant age0 ; la
boucle D700 effectue le premier déplacement une seule fois.

Les anciens vecteurs fish3 restent inchangés. Leur test isolé compare
maintenant la position de construction exactement à celle du Z80 ;
l'ancien décalage de convenance +3 est supprimé, sans retrait d'assertion.
Le mouvement complet est vérifié séparément par le nouveau différentiel.

## Entrées contrôlées et étendue

Huit scénarios,180 passages de deux images chacun,2880 images :

- panthère dormant/mobile, et poisson21h dormant/marche ;
- deux poissons03 en état de tir, avec réallocation/repassage et émission
  dans la même passe, dans les deux directions ;
- deux émissions isolées, état de tir commun et gel dès le passage suivant.

Cartes réelles des salles1/2 et10/6. Positions initiales communes, garde255,
arrêt des canaux C440=255, trou libéré au passage8 et gel au passage24 pour
les repasses. Compteur de marche64, compteurs de tir24 ou9 et état03 de tir
sont des fixtures RAM explicites ; aucune traversée naturelle revendiquée.
Les champs non représentés directement sont conservés dans les métadonnées
du bloc ; les champs représentés sont reconstruits depuis Enemy/S8Ext/f3.
Le projectile de poisson a sa représentation canonique (vitesse3*dx,
pose03, mobile1). Les huit D700 sont initialement propres, un seul tir
par scénario : aucune fidélité générale des réutilisations n'est déduite.

Chaque passage compare les32 octets de7 C800 et8 D700, y compris inactifs,
la position de la première cellule6445 des objets panthère/poisson,
l'énergie de Simon et l'absence de sons des deux implémentations.
Les six premiers scénarios peuvent inclure une longue continuation gelée ;
ces entrées restent indiquées, sans prétendre à360 images de déplacement.

## Core Mega Drive

Scène10/6 préparée en RAM, deux directions. Vérifie le premier tir à189/195
puis360 images de gel dans chaque direction ; captures et summary.json.
L'ancien scénario zzm_stage10 est rejoué aussi. Ce contrôle de scène ne
constitue pas un parcours naturel : son scénario historique comporte ses
propres entrées contrôlées. Les preuves LOT14/14B/14C restent rejouées.

## Réserves restantes

Les autres espèces C800 àY=E0, l'intégration effective des SAT de boss et
les combinaisons score/son09 ne sont pas validées par cette livraison.
Poursuivre ces réserves en LOT14E, puis parcours LOT15 et audit LOT16.
Aucun achèvement général ni parcours naturel12..18 n'est annoncé.
