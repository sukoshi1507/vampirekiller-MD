# LOT08 — marchands, murs et blocs cachés, stages12..18

La référence est chargée par `probe.load_rom(probe.reference_rom())`, SHA256 vérifié par probe.py. Aucune instruction de la ROM n'est modifiée. Générateur : `tools/build_trade_data.py`. Comparaison exécutée : `tools/verify_trade_trace.py`, ponts natifs `tests/trade_bridge.c` / `trade_view_bridge.c`.

## Tables et inventaire

`940C: LD B,09` borne la recherche de **neuf produits**, partagés par tous les stages. Les neuf lignes de quatre octets en9428 sont donc conservées : ajouter des prix fictifs serait contraire à la ROM. La limite tardive était dans les tables de salles et leurs index, désormais19 stages ×12 salles. Les16 sélecteurs en5B0B sont générés, y compris les sept zéros réservés après les neuf produits.

5A49 décode le flux du stage ;5B1B construit les objets de la salle ;8671 initialise les bougies/blocs. Les sept blocs ci-dessous sont réellement cassés par8671 (contact+3), qui appelle8814→9179. L'initialisation de91BE→924C est exécutée ensuite.

|Stage|Salle|Origine|X,Y|Code|Produit|Index bougie|
|---|---|---|---|---|---|---|
|12|4|direct|128,112|DF|1D|—|
|13|0|bloc caché|32,48|E3|1B|3|
|13|4|direct|32,144|C6|12|—|
|13|6|bloc caché|192,48|D1|0A|1|
|13|10|bloc caché|192,144|D8|1E|2|
|14|6|direct|96,144|C1|0E|—|
|15|0|bloc caché|192,48|D6|16|3|
|15|2|bloc caché|192,48|DC|1D|5|
|17|1|bloc caché|128,144|DB|1E|4|
|17|6|bloc caché|192,144|E0|1B|5|
|17|8|direct|32,48|CA|03|—|
|17|9|direct|32,48|D4|16|—|
|18|1|direct|192,144|E3|1B|—|
|18|8|direct|32,48|D4|16|—|

74 salles tardives décodées,14 occurrences, aucune au stage16. Au plus un mur direct OU caché par salle tardive, toujours emplacement DE00 numéro0 : invariant vérifié par le générateur. Les tables cumulatives contiennent16 murs directs et17 blocs marchands, stages0..18 compris. Les ressources des bougies tardives existent déjà dans les fichiers stage1X_objects ; ce lot n'en duplique pas le chargeur général.

## Contrat de comparaison

Les murs MSX occupent **deux enregistrements de16 octets**, C5B5 etC5C5, pas deux blocs ennemis de32 octets. Les32 octets contigus sont comparés après chaque passe. Le pont C reconstitue les adresses statiques du flux (+7/+8) et le sélecteur (+4) depuis le décodage ROM ; le port conserve un index de persistance à la place d'un pointeur RAM MSX. Ces métadonnées ne sont pas des sorties calculées à injecter dans le comportement. +0,+1,+2,+3,+5,+9,+A et tous les octets nuls sont vérifiés.

91F2/9221 : coups et compteur DE00 ;92BB : résultats, tables9300/9320 ;9229 : animation32 passes ;92A7 : marchand, difficulté bornée0..3, cœurs bornés0..99, rien, disparition/5000 points. Le vrai LD A,R de92E8 est exécuté ; sa valeur observée en92EA est fournie au C, sans changer le Z80. Cela prouve la transition conditionnelle, pas une émulation matérielle de R.

9387/93FF/9418 : produit, prix BCD et priorité du livre bit7 sur bit6 ;440A conserve C709 ;94BA/951F ne prennent que les nouveaux boutons ;43FD compte15 images de fermeture. C003 continue pendant le magasin ; main.c synchronise maintenant les horloges du monde et du magasin. 8D30 **retourne** le son dans A, il n'appelle pas509F :94EF remplace ce retour par12. Le port ne doit donc pas émettre deux sons d'achat. Les requêtes509F sont comparées en totalité et dans l'ordre, sans comparaison par suffixe.

Le rechargement exécute d'abord le vrai LDIR6382..638D, avec arrêt avant638F, puis5B1B. Omettre ce nettoyage laisserait des octets périmés dans les murs disparus. Le retour dans la salle, le refus conservé dans DE00, la croix928D et la reconstruction par5A49 sont testés.

## Résultats

- Initialisation des74 salles : PASS.
- 14 parcours de huit coups jusqu'à disparition, chacun1219 images :17066 images PASS.
- Sept variantes pour chacune des14 occurrences (non, pauvreté, délai, prix réduit, prix augmenté, deux livres, bouton tenu à l'ouverture), retour salle, croix et nouvelle vie :98 scénarios,42098 images PASS. Chaque scénario comprend320 images de monde, en plus du magasin.
- 83A1→84A7 :112480 positions/hauteurs, fouet cuir/chaîne, deux directions et deux postures : PASS. Pas de modification des32 octets d'objet par ce test géométrique.
- Chargements graphiques548D/53B6/564C/570D/5768/5780, puis925E et9397 :1344000 pixels comparés au C de production. Cinq images du mur,504 fenêtres (14 positions ×9 produits ×4 combinaisons de livres), restauration950D : PASS. Maximum29 tuiles uniques, inférieur au budget34. Les pixels transparents du mur sont vérifiés à la source et sur le bitmap après925E.
- Rendu MD : les murs tardifs emploient208..219, hors du décor16..207 et avant le HUD224. Les anciens murs gardent200..223. Une seule image tardive peut occuper ce tampon ; l'invariant est contrôlé à l'extraction.
- Genesis Plus GX : sept murs directs,3732 images, achats et300 images de Simon figé par marchand : PASS. Salles entrées par RAM, contacts injectés dans le verrou+3. Fenêtres visibles et coins du cadre vérifiés. Les sprites matériels peuvent recouvrir la fenêtre, notamment la puce de13/4 ; la comparaison complète des pixels de9397 ci-dessus porte sur le bitmap avant cette composition.

## Limites explicites pour les lots suivants

Les adaptateurs BIOS/VDP Z80 sont synchrones ; le test ne prouve pas le timing vidéo matériel ni la forme d'onde analogique du son. Les murs utilisent925E (copie bitmap), pas6445 (sprites ennemis). Le test natif contrôle la transformation en sprite MD et le core contrôle son affichage visible ; aucune équivalence inventée de ces deux mécanismes.

**Intégration provisoire des blocs tardifs** : VKCandles reste de taille354 et ne charge pas les bougies stages12..18 avant LOT11. Les sept factories cachées et leur persistance C sont prouvées par appels à `vk_wall_block`, relié au callback existant dans main.c ; le parcours jouable qui les casse normalement n'est pas prouvé. LOT11 devra consulter `vk_wall_block_present` avant de recharger un bloc tardif détruit, puis employer le même callback. Aucune affirmation de traversée jouable des stages12..18.

**Objet0E provisoire pour le rendu** : l'achat de14/6 donne maintenant C702 bit0, vérifié même quand un ancien handler renvoyait zéro événement. L'appel870C dessine immédiatement les contours blancs des blocs cachés ; cette partie visuelle manque encore au port (LOT10, puis chargeur LOT11). Les autres effets globaux d'inventaire et les objets incomplets restent dans le périmètre LOT10. Ce lot ne les déclare pas tous achevés.

## Rejouer

```sh
PATH=/workspace/vk-venv/bin:$PATH PYTHON=/workspace/vk-venv/bin/python sh tests/run_trade_trace.sh
PATH=/workspace/vk-venv/bin:$PATH PYTHON=/workspace/vk-venv/bin/python sh tests/run_native.sh
(cd sgdk && PATH=/workspace/toolchains/mars/m68k-elf/bin:$PATH make GDK=/workspace/sgdk -j3)
PATH=/workspace/toolchains/mars/m68k-elf/bin:/workspace/vk-venv/bin:$PATH VK_MD_CORE=/workspace/emulators/genesis-plus-gx/genesis_plus_gx_libretro.so PYTHON=/workspace/vk-venv/bin/python sh tests/run_trade_md.sh
```

Le premier script rejoue aussi `build_trade_data.py --check`. Le JSON research/trade-trace.json est local et régénérable, exclu des livraisons.
