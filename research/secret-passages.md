# LOT09 — passages secrets : table BBCD et état de Simon7

## Résultat principal

**La ROM de référence ne définit aucun point de passage secret aux stages13..18.** Les dix entrées de BBCD concernent uniquement le stage12. Ce n'est pas un manque à combler par de nouvelles destinations. Le LOT09 vérifie cette absence en exécutant le chargeur de toutes les salles concernées et branche le mécanisme réellement présent, jusque-là isolé dans stage12_secret.c.

Chargement de la ROM : `probe.load_rom(probe.reference_rom())`, SHA256 validé par probe.py. Aucune instruction ROM remplacée. Le générateur `tools/build_secret_data.py` exécute BB9A sur228 couples stage/salle (19×12), lit BBCD jusqu'au terminator FF, et compare la table exécutée avec les enregistrements bruts. Les dix entrées du générateur stage12 historique sont conservées à l'identique. Le nouveau générateur propose `--check`.

|Stage|Source|Destination|X|Y de la table|Y de Simon admissible|
|---|---|---|---|---|---|
|12|0|3|88|168|172..187|
|12|1|4|184|136|140..155|
|12|2|11|152|168|172..187|
|12|3|0|88|168|172..187|
|12|4|1|184|136|140..155|
|12|5|8|88|168|172..187|
|12|7|10|56|168|172..187|
|12|8|5|88|168|172..187|
|12|10|7|56|168|172..187|
|12|11|2|152|168|172..187|

BBCD en banque13, offset1BBCD, dix enregistrements de4 octets puis FF en1BBF5 : stage, source | destination<<4, Y, X. Stages13..18 :62 salles, zéro entrée. Stage12 : pas de point dans les salles6 et9.

## Routines réellement exécutées

- **BB31→BB9A** : pose C5B1=1, C5B2=Y, C5B3=X, C5B4=destination si stage et salle correspondent. Ne remet pas ces octets à zéro quand rien ne correspond : c'est le LDIR **6382..638D** qui efface C470..C6FF avant le chargement. Le test part d'un point stage12 actif, exécute ce vrai nettoyage puis BB31 pour chacune des62 salles tardives ; tous les points restent nuls.
- **85F4** : C5B1 non nul, puis deux différences **sur8 bits**, `(C425-4-Y)&255 <16` et `(C427-X)&255 <16`. Les bornes supérieures sont exclues. Pas de centre géométrique inventé.
- **6DA9** est l'entrée du joueur **déjà accroupi, C420=2**. Le prédicat 85F4 doit réussir et **C006 bit5**, HAUT nouvellement pressé, doit être présent. C007 (touches tenues) ne remplace pas ce front. 6DB5 écrit C420=7, C421=0, C42D=40h et demande **509F avec A=15h, soit21 décimal**. Le prototype utilisait15 décimal : corrigé.
- **70FB** : ne décrémente rien. Tant que C42D est non nul, retour. À zéro : C420=0, C43D=0, C41B=FF. **75C0→75CC/75CF**, après la routine de Simon, décrémente C42D une fois par passe commune. La passe d'activation réduit déjà64 à63 ; la sortie est demandée à la passe relative64.
- **B963→B98C** : la requête FF sélectionne la destination C5B4, efface C41B, écrit D001 et retourne carry=1. Ni X ni Y ni stage ni direction ne changent. Ce n'est pas une sortie latérale qui replace Simon à un bord.
- **7837/790C/78CC** : réutilisent les poses accroupies. C420=7 garde le décalage supérieur de6 pixels du corps accroupi ; le clignotement cache Simon lorsque C42D après baisse n'est pas nul et C003 bit1 est mis. Les sprites sont ceux de Simon, pas un objet ennemi6445.
- **7E77 et7EF4** : les contacts ordinaires sont interdits pour C420>=5, y compris7 quand le compteur vient d'atteindre zéro. Garde ajoutée à `Enemies_frame`, en conservant l'exception C434 du code existant.
- **85A6 a une autre règle** : les broyeurs excluent l'état6 seulement, puis vérifient C42D et C43A. Avec C420=7, C42D=0 et broyeur au contact, le vrai85C6 remplace l'état par5 ; le passage est interrompu. Le test exécute cette situation (broyeur type2, Y144 X80, Simon88/172 : énergie32→24). Le callback de contact annule maintenant l'attente du passage. La garde générale C420>=5 n'a donc pas été ajoutée au helper de dégâts partagé par les broyeurs.

## Port et branchement

`secret.c/.h` est indépendant de SGDK. VKSecret est un nouvel état de8 octets, extérieur à VKGame, VKCandles et EnemyState. Le module porte les quatre octets du point, l'état7, C421, la requête C41B et l'analogue C43D. La minuterie est celle d'EnemyState, déjà partagée par les contacts ; aucune seconde horloge ni temporisation arbitraire.

Le chargement commun de salle dans main.c initialise le point et efface un éventuel front HAUT en attente. Avant la physique, `secret_controls` consomme HAUT nouveau seulement si le joueur était accroupi dans la zone. Les fronts reçus entre deux passes30Hz sont mémorisés jusqu'à la suivante ; le joueur reste accroupi pendant cette demi-passe. Durant l'état7, ses contrôles de déplacement ne sont pas exécutés, tandis que le monde, les attaques et les minuteries communes continuent. À la sortie, le changement de salle existant affiche la destination et conserve sa position. actors.c fournit le vrai état7 aux boîtes de contact tout en gardant l'image accroupie.

L'ancien API stage12_secret reste un test autonome de compatibilité ; son son devient0x15. Ses données generated/stage12_foundations.c/.h n'ont pas été modifiées. La production utilise la nouvelle API partagée et le compteur commun.

## Preuves et couverture

`tests/run_secret_trace.sh` compile le C de production et ses ressources, vérifie le générateur, puis exécute `tools/verify_secret_trace.py` :

-156 salles valides initialisées,228 couples sondés par le générateur ; nettoyage/BB31 des62 salles13..18 après un point stage12.
-8192 comparaisons de85F4 : les256 valeurs de chaque axe pour les dix points et six cas de point absent.
-96 scénarios, **360 images chacun**, soit34560 : activation, gauche/bornes hautes incluses, sans front HAUT, état debout, hors zone, invulnérabilité préexistante. Dix passages réels et six stages sans passage.
-32 octets de Simon C420..C43F comparés après chaque étape, plus point, requête, salle, C003 et son509F complet. Le pont conserve explicitement les champs que ce module ne modifie pas ; aucune prétention à réexécuter toute la marche normale des contrôles négatifs.
-HAUT déjà tenu sans front : refus exécuté aux dix points, arrêt au début du chemin normal6DC8, avant la gestion non concernée des escaliers.
-État7 ne possède aucun objet IX. Les gardes7E77/7EF4 sont néanmoins exécutées avec un ennemi présent ; ses32 octets sentinelles restent inchangés. Côté natif, le contact de l'ennemi chevauchant ne blesse pas les états5..8, mais blesse Simon debout : contrôle positif de la fixture. Broyeur : interruption et perte de8 points validées séparément ; ce test ne revendique pas de prouver tous les timings du recul existant.
-**29491200 pixels** de sprites comparés. Le Z80 charge ses motifs par56E1 et construit ses vrais tampons par7837/790C/78CC. Le test reproduit le transfert de ces tampons en VRAM puis compose les couches MSX. Le C reconstruit les tuiles accroupies existantes avec les coordonnées, direction et masquage utilisés par le port. Les images gauche/droite, visibles/cachées, concordent ; aucun nouvel art ou décalage ajouté aux ressources.

`tests/run_secret_md.sh` utilise Genesis Plus GX et la ROM SGDK compilée : vingt activations (dix points ×deux phases d'entrée), **128 images d'attente pour chacune**, les15 octets de VKPlayer conservés, destination exacte, clignotement visible/caché, demande PSG0x15 acceptée une seule fois à l'entrée. Puis320 images de contrôles négatifs pour chacun des six stages sans point. Total4999 images. Les entrées de salle, position/posture initiale, phase d'horloge et invulnérabilité de test sont écrites en RAM ; les contrôles HAUT sont ensuite réels.

## Limites exactes

**Disponibilité provisoire avant LOT11** : l'accès normal, les collisions générales et le chargeur complet des stages12..18 ne sont pas ajoutés ici. L'intégration des passages est testée par entrée RAM dans les salles ; elle ne prouve pas une traversée depuis le stage11. Cette réserve est également dans le code main.c. Rien ne justifie d'ajouter des passages aux stages13..18.

Les comparaisons se font aux limites des passes logiques. Le compteur C003 du port existant avance dans Enemies_frame, après la logique de Simon, alors que414D l'avance avant. Le contrôle avant minuteries attend explicitement cette différence transitoire de-1 ; **après la passe les compteurs doivent être égaux sans décalage**. Aucun décalage des états de Simon, des compteurs, des sons ou des pixels n'est autorisé. Le test ne revendique pas l'équivalence cycle par cycle des interruptions.

Les BIOS/VDP Z80 sont synchrones. Le transfert SAT/couleurs est adapté à partir des vrais tampons ; pas de revendication de timing vidéo MSX ni de comparaison de forme d'onde analogique. Le test de sprites contrôlé ne mélange pas le décor et les ennemis aux pixels de Simon. Le core valide en complément le branchement réel du port. Les limites LOT08 (chargeur des blocs tardifs et contour0E) demeurent celles de son bilan.

## Commandes

```sh
PATH=/workspace/vk-venv/bin:$PATH PYTHON=/workspace/vk-venv/bin/python sh tests/run_secret_trace.sh
(cd sgdk && PATH=/workspace/toolchains/mars/m68k-elf/bin:$PATH make GDK=/workspace/sgdk -j3)
PATH=/workspace/toolchains/mars/m68k-elf/bin:/workspace/vk-venv/bin:$PATH PYTHON=/workspace/vk-venv/bin/python VK_MD_CORE=/workspace/emulators/genesis-plus-gx/genesis_plus_gx_libretro.so sh tests/run_secret_md.sh
```

Les scripts sont exécutables et intégrés à run_native.sh (MD optionnel via VK_MD_CORE). research/secret-trace.json est régénérable et exclu des livraisons.
