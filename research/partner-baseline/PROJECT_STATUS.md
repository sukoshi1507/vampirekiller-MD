# Base stabilisee — BASELINE-01

Etat au 26 octobre 2026. Le prochain travail concerne Simon dans une seule
salle. Ne pas ajouter de nouvelles zones ou de nouveaux ennemis avant ce jalon.

## Criteres de sortie

| Critere | Etat | Preuve |
| --- | --- | --- |
| Une seule ROM MSX2 de reference | Valide | SHA256 controle par `tools/probe.py` et `config/baseline.json` |
| SGDK et GCC identifies | Valide | SGDK 2.11, GCC m68k-elf 13.1.0 ; versions et archives referencees |
| Compilation depuis les sources versionnees | Valide | `make build`, sans ROM MSX2 ni extraction prealable |
| Deux compilations propres identiques | Valide | `validation/build-checks.json` |
| Demarrage MSX2 independant de l'extracteur | Valide pour C-BIOS | ROM complete dans openMSX 19.1 / C-BIOS_MSX2 ; aucun patch ni remplacement de routine |
| Logo, titre, grille et cour de depart | Valide pour les donnees graphiques | Quatre fonds de 27 136 octets et leurs couleurs utilisees identiques |
| Trajectoires, motifs et couleurs de l'intro | Valide | 319 mises a jour consecutives, huit groupes de sprites par mise a jour |
| Parcours du portage titre → intro → cour → hall | Valide | Comparaisons MD et controles d'etat dans `validation/intro-checks.json` |
| Chronologie strictement identique au MSX2 | Adaptee | Differences listees ci-dessous ; ne pas annoncer une equivalence temporelle |
| Son du demarrage | Non porte | Le portage reste muet |

Les donnees de reference sont conservees dans `validation/msx/startup-reference.zip`.
Les PNG `*-vram.png` sont des vues normalisees de ces donnees, **pas des captures
du framebuffer natif d'openMSX**. Les tests comparent les indices et palettes,
puis separement le rendu Mega Drive. Le rendu analogique d'un vrai MSX2 et un
BIOS constructeur restent hors de cette validation.

## Traduction, adaptation et prototype

| Element | Statut actuel |
| --- | --- |
| Graphismes du demarrage | Extraits de la ROM, compares a une execution complete |
| Positions et motifs de l'intro | Issus des routines originales, compares aux 319 mises a jour capturees |
| Primitives de collision et saut | Traduction C comparee aux instructions Z80 |
| Invite du titre | Adaptation `PUSH SPACE KEY` → `PRESS START` |
| Logo | Adaptation : fondu sur 120 images MD, contre copie progressive et attente MSX2 |
| Attente au titre | Adaptation : indefinie ; la demonstration automatique MSX2 n'est pas portee |
| Lancement de la partie | Adaptation : les attentes/etapes intermediaires MSX2 ne sont pas toutes reproduites |
| Intro devant la grille | 320 images MD ; la trace MSX2 capturee effectue 319 mises a jour selon la parite du compteur |
| START pendant logo/intro | Adaptation : permet de passer la scene |
| Controleur de Simon et porte | Prototype, gestionnaire d'etats original incomplet |
| Flammes dans les nouvelles scenes | Statiques et sans degats |

## Chronologie observee

Temps en secondes emulees depuis la mise sous tension de C-BIOS, machine 60 Hz.
Les points ci-dessous correspondent a des passages dans le gestionnaire
d'etats, et non a des mesures du premier pixel affiche. La duree de boot de
C-BIOS ne fait pas partie de la duree du jeu sur Mega Drive.

| Evenement | Temps MSX2 observe |
| --- | ---: |
| Entree dans le demarrage du jeu | 4,071 |
| Copie du logo en cours, sous-etat 1 | 4,305 |
| Copie terminee, sous-etat 2 | 5,940 |
| Titre pret, etat 1 | 10,713 |
| SPACE pris en compte, etat 3 | 10,930 |
| Mise a jour de l'intro, sous-etat 4 | 12,916 |
| Chargement de la cour, etat 4 | 18,657 |

## Regle pour la suite

Un jalon contient une observation MSX2, les routines et donnees identifiees,
l'implementation C, une comparaison et un commit Git coherent. Toute
adaptation doit etre nommee. Un test MD/reference ne suffit pas a valider
la reference elle-meme.

Prochain jalon : marche, orientation, saut, chute, accroupissement et
collisions de Simon dans une salle. Les attaques et l'extension des zones
viennent ensuite.
