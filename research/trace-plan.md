# Prochaine session de trace MSX2

Plan du premier jalon. Au jalon 2, la routine de carte 4F83/4FAF a ete testee
separement sur CPU Z80 et un explorateur de fonds tourne sur Mega Drive.
La trace complete MSX2 decrite ci-dessous reste a effectuer.

Ce plan n'a pas encore ete execute. Aucun journal d'emulation n'est fourni.
Utiliser une machine MSX2 et les BIOS appropries dans un emulateur disposant
d'un debogueur. Les adresses ci-dessous sont des adresses CPU, sauf mention
explicite d'un offset physique. Filtrer les arrets sur le slot de la cartouche.

## Points d'arret et observations

| Adresse / evenement | A relever |
| --- | --- |
| Execution `4075` | Slot de cartouche, registres et banques au demarrage |
| Retour de `5336` | Banques 01/02/03 et miroirs RAM F0F1-F0F3 |
| Ecritures aux selecteurs `6000`, `8000`, `A000` | PC, valeur A, nouvelle banque, numero d'interruption |
| Execution `4028` | Numero d'interruption, C005, E600, mapping courant |
| Execution `8964`, banque 0E en fenetre 8000 | Appels, ports touches, duree et retour |
| Execution `414D` | C000, C001, C003, registres et mapping |
| Ecriture a `C000` et `C001` | PC, ancienne/nouvelle valeur et action utilisateur |
| Appel `005F` depuis la cartouche | Valeur A pour recenser les modes video |

Surveiller aussi les plages de selecteurs que le mapper effectivement choisi
accepte : les seules adresses exactes listees ne couvrent pas necessairement
toutes les ecritures possibles. Un point d'arret sur une adresse bankee doit
toujours etre associe a la banque active.

## Sequence a capturer

Demarrage sans entree, puis acces au menu, lancement d'une partie, immobilite
dans la premiere salle, deplacement a gauche/droite, saut et attaque. Relever
la correspondance des touches et boutons sans la supposer d'avance.

Pour chaque changement d'etat, enregistrer une capture de reference, le numero
d'image, la frequence video, le PC, les registres, le mapping, `C000-F0EF`,
la VRAM, les registres VDP et la palette. Les captures restent des materiaux
de comparaison; elles ne remplacent pas la comprehension des formats ROM.

Pour retrouver les ressources, suivre les routines qui alimentent le VDP et
les commandes de copie, noter les adresses source et le mapping a cet instant,
puis convertir chaque adresse source en offset physique avant d'extraire.

## Critere de sortie

Une trace qui associe les etats traverses a l'ecran observe, identifie le chemin
de chargement d'une salle et permet d'en reproduire l'image a partir de donnees
localisees dans la ROM. Ce sera la base du premier extracteur graphique fiable.
