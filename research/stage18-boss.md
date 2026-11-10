# Dracula : logique exécutée et limites du lot07

## État

Logique, rendu des deux formes, manteau, palette/fondu, défilement et effacement des crédits raccordés dans `dracula_view.c`, `actors.c`, `main.c`. Combat isolé validé dans Genesis Plus GX. La progression normale des stages12..18 reste au LOT11 : ce lot ne valide pas leur traversée.

## Méthode

`tests/run_dracula_trace.sh` compile le C de production en bibliothèque native. `tools/verify_dracula_trace.py` exécute la ROM originale via le mapper d'IntroHarness, avec l'adaptateur de lecture VRAM de BossGraphics. Comparaison de tous les octets IX+00..1F des sept objets C800 et huit projectiles D700, y compris les octets conservés après disparition, et des cellules visibles de6445. Comparaison des compteurs de salle, états du générique, copie de palette CE60..CE7F, Simon C425/C427, énergie, score BCD réel, commandes509F.

Une passe équivaut à deux images avec maintien de l'état entre passes, comme les tests des lots05/06 ; ce n'est pas une mesure du timing vidéo MSX. Les entrées/sorties VDP et les appels BIOS dessin sont adaptés explicitement. Les contrôles de rendu comparent maintenant les indices de pixels des copies de décor et des crédits, en plus des objets et des sprites.

La gerbe de six particules utilise les vrais LD A,R à6854/6860. Les résultats sont observés après ces instructions, puis donnés au fournisseur de hasard du C dans le pont de test, sans modifier la ROM. La relation `(R2-R1)&127 == 8` et le nombre exact de12 consommations sont vérifiés. En production, le module consomme le générateur commun Enemies_randomR ; Y respecte le couplage de huit incréments, sans prétendre reproduire le timing matériel de R.

## Entrée et première forme

77D1 : porte salle8 vers salle9, X246, Y préservé, **musique86** (les autres boss utilisent87). Stage18 appartient au groupe D002=5. Dispatcher B6AB, CE00=6, script65B0.65CC/65D3 crée l'objet11h à X192,Y208. Initialisation AB22, mise à jour AB42.

États : attente30 passes ; élévation de64 pixels à vitesse -0200h ; apparition clignotante ; ouverture ; trois tirs après4 passes, avec VX +/-0280h et VY0,+0180h,-0180h ; maintien30 ; disparition ; attente90 ; téléportation vers la table AC76. Les tirs réellement créés sont de type05h dans D700, pose3, par AC05/9F6D. Leur déplacement est constant et ils s'arrêtent pendant D010. Le corps a128 points, dégâts4/6 et score30000.

Contact du corps11h : H=30h,L=10h. Le fouet et les armes visent **la tête**, H=18h,L=5,Y objet-32 ; ils ne réutilisent pas la boîte de contact du corps. Les routines8290/827C/82A4 et les paramètres de84F0 sont exécutés pour le prouver.

Mort :7E0C/7E17 puis7806. L'objet11h est une exception dans9AA3 : passage à l'état8, pas une flamme générique. ACAE efface les projectiles, attend60 passes et crée une tête2Dh à Y-43. Cette création change IX : le reste de la passe déplace la tête sans appliquer son incrément de gravité. Le C reproduit ce changement d'objet. Les passes suivantes de la tête exécutent AD12, gravité50h ; les particules2Eh ultérieures ne font pas de gravité.

## Transformation et seconde forme

CE16 déclenche65F3 : gerbe de six objets2Eh, vitesses de69E4, positions autour de CE0F et Y98h. Attente120 et disparition de C800, arrêt sonore00.6619 démarre88, remet128 points et crée l'objet17h à **X128,Y73** ; il sert de cible fixe au monstre dessiné dans le décor. Ce n'est pas un marcheur ordinaire.

68C4/68A8 pilotent la visibilité, avec phases16/32 et temporisations192/64.67E4 anime la couleur6 ;6818 alterne des copies de tête ;6A49 crée2Ch à X128,Y152 lorsque le monstre est visible et C003&7=0.6A5D choisit les vitesses via le compteur persistant CF32 ;6A9C anime les volatiles et soustrait30h à VY avant l'état4. Leur boîte est1008h ; celle de la cible17h est également1008h. Les tirs D70005h ont0603h.

7E2C inflige16/24 à la seconde forme avec cuir/chaîne ; les armes C416>=2 divisent ces valeurs par4, soit4/6. Score50000. Mort, disparition du monstre, musique8D, attente180, fondu de128 passes (6A0E sur la palette conservée en VRAM F680), attente8, puis CE40=1.

## Générique et suite

66BA et6712..678B exécutés : musique8E, défilement commandé toutes les quatre valeurs de C003, textes lus via la vraie table678E (banque8 en A000 et banque5 en8000).41 lignes, puis marqueur terminal. Celui-ci demande509F=FF ;5309 attend réellement C0A6=F8. Le test représente explicitement cette entrée du pilote audio ; il ne valide pas la durée musicale réelle. Attente30, C409=1, difficulté accrue avec plafond2.

4326 est également exécuté jusqu'à sa frontière de chargement : premier parcours, CF34 devient1, D002 revient0 et D000 passe parFF puis0 (nouveau cycle depuis la cour) ; deuxième parcours, CF34 devient2 et la ROM passe à l'état0 de la séquence initiale/titre avant l'incrément de stage. Le chargement62D5 est adapté dans le test Z80 de transition ; le test Mega Drive exécute ensuite réellement les retours à la cour et au titre.

## Ressources et initialisation complète

`tools/dracula_graphics.py` exécute548D,53B6,564C,570D,5768,5780, puis **6333**. La branche spécifique CE00=6 appelle59EC (palette),5880 (décor/mirroirs/tuiles supplémentaires) et57DF (quatre images du manteau).4F83 décode la carte,4F91 l'affiche et632D initialise les deux régions animées du visage. Les sources absentes du premier prototype sont ainsi chargées sans les inventer.

`build_dracula_background.py` exécute6984/69A0 et6914/68DC/694C pour les six combinaisons du visage ;283 tuiles dédupliquées et quatre images32×32 du manteau. Les pixels provenant des anciens indices192..245 de la salle18/9 sont inclus et vérifiés. Les copies transparentes du manteau **s'accumulent** : le module garde l'ordre des poses depuis le dernier effacement, puis compose16 tuiles de sprite32×32 à la mise à jour.

Les283 tuiles de fond sont réparties entre16..195, le bloc d'ennemis ordinaires596..643,660..691 et la fin libre du bloc des sprites du boss. Les zones du HUD196..274, mort/escaliers280.., fouets404..595 et objets700..751 sont préservées, avec assertions de compilation. Le manteau occupe644..659 ; les92 tuiles de sprites commencent à760.

`build_dracula_md.py` exécute5768/5780/57B4/6445 :26 poses,92 tuiles après déduplication, dont les disparitions D700. `build_dracula_data.py` extrait les41 lignes/temporisations et décalages de mort. Aucun fichier generated n'est édité à la main.

`build_dracula_credits.py` exécute66BA/6743 : image initiale et41 bandes de texte,27 tuiles dédupliquées. Le renderer attribue une tuile VRAM à chaque cellule du plan256×256 pendant les crédits et conserve une petite table de références en RAM.682A efface une ligne de pixels, pas une rangée entière de8 pixels : les masques d'effacement sont appliqués avant chaque transfert. Le défilement vertical reproduit R23. Au retour, la file DMA est vidée avant de changer de scène et le cache des textures du HUD est invalidé.

## Branchement et combat

Le boss18 est le kind5 de la navigation, tandis que sa logique reste dans `dracula.c`. `actors.c` possède l'instance, conserve CF32 entre les entrées et CF34 entre les cycles, et les remet à zéro au début d'une partie. Dégâts/contacts, boîtes de tête, premiers objets C800 touchés par le fouet, tirs D700, volatiles, armes lancées, demandes sonores, score et barre sont raccordés. Les effets de disparition continuent pendant D010.

La première fin relance la cour : score, vies et cœurs conservés ; énergie rechargée ; objets/armes/clefs remis selon70DC, objet de carte conservé, difficulté accrue sans dépasser2. La deuxième mène au titre. Pendant les crédits, les contrôles de déplacement et la pause ne perturbent pas la séquence. La fin musicale est reliée au vrai compteur de fondu du lecteur PSG (huit pas), puis aux30 passes d'attente.

## Validation supplémentaire

- 66888 images de logique/objets/sprites :8000 de mouvement,57288 de fins complètes,1600 de destructions de tirs/volatiles (35 coups, gel inclus).
- 146868 contrôles de contact/fouet et métadonnées de cibles des armes lancées.
- 23066 images contrôlées pour les pixels de décor et42230 pour les pixels des crédits (les contrôles de l'image noire finale se recoupent).
- Genesis Plus GX :24746 images exécutées,17408 pixels RVB de la salle comparés et57344 pixels du masque blanc des crédits/défilement comparés, avec tolérance d'une ligne pour la frontière vidéo. Deux combats par les callbacks réels de fouet, score80000 puis160000, crédits et retours cour/titre, palette du titre incluse.

Le test MD entre dans la salle par écriture RAM et maintient Simon à une position contrôlée, avec invulnérabilité ; il réduit l'énergie du boss à4 avant le coup de fouet pour accélérer ces deux parcours. Il **ne prouve pas** la traversée ni les collisions de sol des stages tardifs. Les128 points et les quatre catégories de dégâts sont vérifiés par les tests Z80/C sans cette accélération. Le test Z80 fournit explicitement l'entrée C0A6=F8 ; le test MD attend le vrai pilote PSG. Le parcours MD des armes secondaires et des objets spéciaux reste à compléter dans les scénarios des LOT10/12.

Core de contrôle :Genesis Plus GX libretro, commit f27655b592ef43f317272d79883bcaa10d3ac8ad, bibliothèque SHA256 f5ddba640e70fc69b664af89a247ca5889be5721bc0a048a9fb7c0408c7ed750, compilée hors du projet. Ni le core ni la ROM ne sont distribués.

## Suite

LOT08 marchands/murs tardifs, LOT09 secrets, LOT10 objets, LOT11 progression et collisions complètes12..18, LOT12 scénarios de traversée. Les captures de ce lot sont des exécutions Mega Drive, pas des aperçus statiques.

## Correctif du lot06 découvert pendant ce travail

Le motif initial3 des tirs utilise F0, chargé par5768 avant5780. Le générateur du boss15 avait omis ce chargement commun et produisait quatre tuiles vides pour la naissance sous arrêt du temps. `build_boss15_md.py` exécute désormais5768 et vérifie que F0 est non vide ; les44 tuiles sont régénérées. Aucun déplacement, compteur ou callback du boss15 ne change. Le test différentiel du lot06 est rejoué après ce correctif.
