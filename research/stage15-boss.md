# Boss du stage 15 : exécution Z80 contre C

Le boss16h est créé par BBF3/BC54 dans la salle9 (CE00=5), porte en salle8. BC6D attend30 passes puis applique les vitesses initiales -0280h/-0200h, gravité8, rebonds latéraux, plafond et sondes de sol7B98. Le sol est celui extrait de la salle, sans hauteur inventée. Les déplacements utilisent99B9/99E5.

BC29 avance CF20 même si la création échoue ; le compteur persiste entre les salles et les vies, remis à zéro par l'initialisation de partie. Les huit positions de BC44 alternent ; quatre créations sont tentées à l'entrée. Les douze entrées SAT du corps occupent deux places de la zone allouée aux projectiles : seuls trois projectiles simultanés sont créés. La temporisation CE02 est initialement0 puis décrémentée modulo256 ; les essais suivants sont espacés de128 passes. Le script de salle continue pendant D010.

Les tirs08h, A06A/9F22/A0E7, attendent60 passes, visent Simon avec une base40h +8*difficulté, se déplacent30 passes et recommencent. Les poses7D..80 sont calculées avant décrémentation. Leur pose initiale3 reste visible si le temps est arrêté dès la création. Le C garde les huit emplacements D700, en étendant VKBoss à la fin de sa structure et les boucles de collision/rendu d'actors.c. Les tableaux historiques de deux tirs restent à leurs offsets.

Le corps est de classe7 : H=30h,L=8. Projectile classe2 : H=12,L=6. Contacts, fouets, états de Simon, deux regards et cuir/chaîne sont comparés aux routines8290/827C et824C/8270. Le corps inflige6 points, ou3 avec armure, par8179. L'énergie globale est128 : dégâts4/6 via7E0C/7E17, soit32/22 coups ; score7000 par la vraie mise à jour BCD.

Mort : corps changé en flamme1Eh avec Y-24, projectiles changés en effets0Ch par9A1A, premier emplacement libre respecté. Les effets continuent pendant D010. Le script commun B6AB/BE47/B6C5 et l'orbe B7A1 sont exécutés : l'orbe s'arrête sur le sol de salle9 à Y=144. Ramassage7E67, soin, sortie et requêtes509F sont comparés : entrée87, mort1C, fin8C, arrêt00 puis8C au ramassage, soin01. Aucun son de tir.

Générateur build_boss15_md.py : chargeur5780 puis dessinateur6445 réellement exécutés ; six cellules du corps, quatre poses de projectile et pose initiale3, soit11 cellules/44 tuiles MD. Couches couleur et ordre SAT contrôlés, dont CC42/05 ; flammes communes chargées ensuite. Pas de modification de ROM.

## Preuves rejouables

`tests/run_boss15_trace.sh` compile le C de production puis exécute `tools/verify_boss15_trace.py` :
- six situations de1600 images : gauche, droite, proche, difficulté2, arrêt du temps, arrêt dès la naissance ;
- deux destructions de projectile (emplacements0 et2), avec arrêt du temps :240 images ;
- trois entrées successives avec CF20 conservé :1020 images ;
- deux fins complètes :2168 et1800 images, orbe abandonnée/ramassée ;
- 73442 comparaisons de contact/fouet/armure.

Total14828 images. Tous les octets IX+00..1F des objets actifs, projectiles, flammes et orbe sont comparés ; sprites6445 avant apparition de l'orbe, états de fin, énergie, score, Simon, sons et compteurs de salle également. La comparaison avance par passes de deux images : l'image intermédiaire maintient l'état ; ce n'est pas une mesure d'interruptions vidéo MSX. IntroHarness adapte les entrées/sorties BIOS. La trace JSON locale n'est pas distribuée.

## Limites

Pas de scénario émulateur Mega Drive effectué pour ce lot. Les étapes de rendu, collisions et combat sont branchées dans les acteurs, mais l'accès normal aux stages12..18 reste au LOT11. Les armes lancées réutilisent les boîtes communes ; leur parcours complet contre ce boss n'a pas été retracé ici. Dracula et la fin du jeu relèvent du LOT07.
