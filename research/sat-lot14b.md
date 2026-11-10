# LOT14B — réutilisation SAT des flammes et sortie99E5

Base LOT14 final vérifiée :1403 fichiers snapshot,64 fichiers ZIP.
Aucun autre processus actif. ROM chargée par
`probe.load_rom(probe.reference_rom())`, instructions intactes.

## Chaîne réellement exécutée

5F61/9FCA réservent une entrée uniquement si son Y dans D638..D67C vautE0,
et écrivent E1 pendant la réservation.6445 prépare les enregistrements
sprites du bloc objet à +20 ;64EC (C800) et64E5 (D700) les copient dans
l'ombre SAT.6501/651B force Y=E1 pour une couche de couleur nulle, même
si le Y géométrique vautE0. Il faut donc distinguer les deux couches.

Le constructeur1E5F1D, pose85 en Y240, ou pose86 en Y239, produit une
première entrée à Y=E0, couleur8 ; la seconde est couleur0, donc E1.
L'objet reste actif après ce rendu. Un deuxième constructeur1E alloue les
indices0 et2 : l'indice0 est partagé avec l'objet encore actif.
Le bitmap natif LOT14 conservait à tort les deux entrées réservées.

`Enemies_sat_render` actualise les réservations au rendu dans actors.c,
dans l'ordre C800 puis D700. Les flammes utilisent la géométrie85/86 et
conservent la réservation de la couche de couleur0. Les masques d'appartenance
ne sont pas effacés lors de la réutilisation : les deux objets référencent
réellement la même entrée. Cela préserve les effets des libérations ultérieures.
Les dispositions publiques ne changent pas.

## Second écart découvert, conservé dans le test

En exécutant98E5 après ce rendu, le Z80 supprime la première flamme :
99E5 est appelé après9B71, et Y>=E4 déclenche99F6. Le port oubliait ce
contrôle pour les flammes immobiles. `deathStep` applique maintenant ces
mêmes bornes (Y>=E4, X>=F1 ou X<7) après sa mise à jour. Le compteur et
la pose restent conservés dans le bloc inactif et sont comparés.

## Preuves et limites exactes

`tests/run_sat_alias.sh`, `tools/verify_sat_alias.py` : quatre scénarios,
360 appels de rendu chacun. Deux gardent explicitement les états des
constructeurs sans appeler l'IA : preuve statique de rendu/allocation,
pas une trajectoire. Deux exécutent98E5 et9E31 entre les rendus et deux
images natives par passage :720 images par scénario,1440 au total.
Positions initiales et compteur255 sont des entrées contrôlées communes.
Comparaison du masque des18 entrées, des allocations/indices partagés,
des32 octets des deux objets (inactifs inclus), sprites6445 (Y/X/motif),
énergie de Simon et absence de sons.
Ce sont des scénarios RAM contrôlés, aucun parcours naturel revendiqué.
Les autres contrôles du LOT14 sont rejoués intégralement.

La preuve d'alias directe couvre C8001E uniquement. Le chemin de rendu
commun0C utilise les mêmes poses ; sa saturation D700 avec alias n'est pas
prouvée ici. Autres espèces à Y=E0, SAT des boss, repasses panthère/poisson3
et toutes les combinaisons score/son09 restent provisoires. Ces réserves ne
sont pas levées par le succès de cette preuve bornée ; poursuivre LOT14C
avant de déclarer la saturation générale validée, puis LOT15/16.
