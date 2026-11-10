# LOT14 — pools mixtes et limites SAT

Référence chargée exclusivement par `probe.load_rom(probe.reference_rom())`.
Aucune instruction ROM modifiée. Les entrées RAM contrôlées sont explicites
ci-dessous ; aucun de ces tests ne constitue un parcours naturel au contrôleur.

## Dépassement dans les salles réelles

`trace_mixed_rooms.py` charge les données de chacune des74 salles12..18,
appelle6154, les constructeurs5F1D, les passages98E5/9E31, la chaîne du fouet
999F/7DAD, les bougies7D68/8671 et le sol8A4A. Deux délais32/96,
400 passages =800 images par scénario :148 scénarios,118400 images.
Simon est replacé, C43A=255 protège les contacts ; ni HP ennemis ni code ROM
ne sont forcés. Le maximum est8 objets : stage13 salle3, délai32, passage64,
5 C800 et3 D700. Cela invalide la capacité unique de7, pas la nécessité de
15 objets simultanément. Les événements et32 octets de chaque bloc sont
régénérés dans `research/mixed-room-pools-trace.json` (non livré).
Cette exploration est Z80 seulement : elle ne prouve pas une simulation C
complète de ces148 scénarios.

## Allocation réellement exécutée

C800 :7 blocs, SAT5F61 croissante,14 entrées physiques0..13.
D700 :8 blocs,9F6D/9FCA, SAT décroissante17..10, deux entrées par objet.
Les domaines partagent quatre entrées. CE03 non nul restreint D700 à17..16.
Sept flammes longues C800 utilisent14 entrées : deux projectiles peuvent
encore être créés ; le troisième échoue. Huit blocs D700 ne signifient donc
pas huit projectiles visibles simultanés. L'hypothèse initiale15 actifs était
fausse et n'est pas conservée comme résultat.

L'allocation réserve chaque entrée avant de connaître le succès final.
Dragon8 puis deuxième dragon8 : échec du second après six réservations,
conservées ; un1E échoue. Après libération du premier dragon, quatre1E
réussissent, le cinquième échoue. Test prolongé360 images.

## Différentiel natif

`verify_mixed_pools.py` :19 scénarios,6840 images. Douze combinaisons
projectiles03/04/09, bouclier oui/non et CE03=0/1,360 images chacune ;
comparaison des32 octets des7 C800 et8 D700, y compris inactifs,
allocations/refus, sprites6445 (visibilité et positions), charges et sons.
Les constructeurs Z80 sont exécutés ; leurs sorties constituent les entrées
communes du pont C. Cela ne prouve pas tous les producteurs C.
Pose initiale03 gelée conservée et vérifiée ; son motif F0 a une couche
couleur0 et une couche couleur8, et reste visible. Les poses initiales des
os/tirs retournés sont désormais incluses dans le générateur graphique.

Quatre scénarios ramassage C800 puis contact D700 (contenus01/02/03/08),
360 images chacun : Simon énergie/cœurs/armure, préfixes et sons. L'effet
ramassé s'applique avant le contact D700. Croix05 :8D2C/7806 ramasse24/26,
transforme les ennemis en1E sans tirage CF40/R, libère les flammes du dragon,
et transforme D700 en0C par9A1A. 360 images, états et sons.
Réallocation9B90 vers un IX antérieur : le passage98E5 doit rejouer les IA
intermédiaires ; test du hopper0B360 images, puis gel contrôlé après24
passages. Dernier scénario : réservations SAT partielles, décrit ci-dessus.

## Implémentation et mémoire

EnemyState reste166 ; les7 anciens objets et leurs extensions gardent leurs
offsets. `vk_projectiles[8]` ajoute144 octets séparés et les extensions D700
sont privées. Accesseurs communs pour IA, collisions, rendu, bouclier,
fouet et armes. Les autres tailles imposées sont conservées. Le harnais MD
vérifie explicitement le nouveau symbole et expose les deux pools.
Le ramassage immédiat utilise un callback ; l'ancien pont isolé conserve sa
file d'événements. Les anciens tests qui assimilaient destruction de
projectile à active=0 sont corrigés pour accepter le0C non hostile réellement
créé par9A1A, sans supprimer le contrôle de disparition hostile.

## Provisoire, à ne pas confondre avec une preuve complète

Le bitmap SAT suit les réservations/libérations normales et les échecs
partiels. La réutilisation d'une entrée dont le rendu Y vautE0 alors que le
bloc reste actif n'est pas prouvée ; l'intégration complète des entrées SAT
possédées par les boss reste à tracer. Le code porte cette réserve explicite.
Les repasses des anciennes IA panthère/poisson3 et toutes les combinaisons
simultanées score/son objet09 ne sont pas couvertes par ce nouveau test.
Les anciennes preuves sont rejouées, pas remplacées. Les parcours naturels,
combats complets et chemins alternatifs restent LOT15/16. LOT14 est une
livraison bornée avec réserves, pas une validation générale de toutes les
saturations ou une déclaration de complétion du port.
