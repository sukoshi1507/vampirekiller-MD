# Audio MSX2 de Vampire Killer : traces PSG et conversion Mega Drive (candidat)

Statut global : **candidat**. Les traces sont des captures d'exécution réelle de la ROM sous openMSX ; la conversion vers SN76489 et le XGM2 sont des approximations non écoutées ni comparées à un rendu de référence.

## Fichiers

| Fichier | Contenu | Statut |
|---|---|---|
| `psg_ay_msx_long1_150s.vgm` | Trace PSG brute (AY-3-8910), 150 s, 28 542 écritures | decoded |
| `psg_sn76489_candidate.vgm` | Conversion vers SN76489 (PSG Mega Drive) | candidate |
| `psg_sn76489_candidate.xgm` | XGM2 généré par `SGDK/bin/xgm2tool.jar -s` | candidate |
| `conversion-report.json` | Statistiques de conversion et limites | - |
| `long1-bios-giwrte.csv.gz` | Appels BIOS GIWRTE (0093) avec appelant, pour recoupement | decoded |
| `long1-game-state.csv` | Transitions de C000/C001 pendant la capture | decoded |
| `scan-row8-game-state.csv` | Transitions d'état pendant le balayage de la ligne 8 | decoded |
| `manifest.json` | Offsets, adresses CPU, banques, statuts, zones non vérifiées | - |

## Méthode

1. Analyse statique : aucun `OUT (A0/A1)` direct dans la ROM. Le pilote passe par le BIOS GIWRTE (`0093`, registre dans A, valeur dans E). Le pilote est en banque 14 (fenêtre 8000), appelé depuis le gestionnaire VBLANK `4028`.
2. Trace dynamique : `tools/trace_psg_openmsx.tcl` journalise chaque appel à `0093` avec l'appelant (lecture de l'adresse de retour dans la pile). `vgm_rec start PSG` (script openMSX) enregistre les écritures PSG au niveau puce.
3. Conversion : `tools/convert_psg_vgm.py` (AY vers SN). Tests : `tests/test_convert_psg_vgm.py`.
4. Lancement : `research/audio/run_psg_openmsx.sh OUTDIR SECONDES "t,ligne,colonne,durée ..."`.

## Points établis

- Le son est produit par un pilote à 4 canaux logiques (ton A, B, C et bruit), mis à jour à chaque image.
- La ligne 8 de la matrice clavier, colonne 1, produit une transition d'état (1 puis 3) et l'apparition de musique sur les trois canaux de ton. Observé une fois, à confirmer.
- Sans touche, la ROM reste dans les états 0 à 2, avec des rafales de bruit et de volume périodiques (environ toutes les 10 s).
- Aucune écriture FM : une tentative avec un MSX-MUSIC de substitution (ROM C-BIOS) n'a produit aucune écriture 7C/7D sur les états observés.
- Enveloppe AY non utilisée dans la trace (0 écriture R11-13).

## Limites et points à vérifier

- Musique et effets sonores ne sont pas séparés : les routines `8A03`/`8B3C`/`8B4D` servent aux trois canaux de ton, et les rafales utilisent les mêmes registres. La séparation demande de désassembler la file de commandes du pilote.
- Déclencheurs (saut, fouet, dégâts, ramassage) : non identifiés. Le vrai rôle de la touche ESPACE dans la matrice n'est pas confirmé.
- Cadence : 8745 appels à `4028` sur 150 s (58,3 Hz) pour 60 Hz supposés par le recorder. Les attentes du VGM sont donc nominales.
- Le VGM brut diffère de 4 écritures du journal BIOS (28 542 contre 28 538 attendues), non expliqué.
- Conversion : volumes approximés (1,5 dB par pas AY vers 2 dB par pas SN) ; 410 écritures de ton bornées à N=1023 ; bruit réduit à trois taux en bruit blanc ; ton et bruit simultanés sur un même canal : seul le ton est gardé.
- Aucune écoute ni rendu de référence ; lecture du XGM2 par le pilote Mega Drive non testée.
- Une capture ultérieure (mémoire de variables sonores) a été interrompue : elle est trop lente avec la surveillance d'écriture RAM large, et une interaction manuelle avec la fenêtre openMSX a eu lieu pendant ce run. Ses données ne sont pas utilisées.

## Effet du fouet : extraction tentée, non concluante (sans fichier whip_sfx)

Objectif : isoler dans `psg_ay_msx_long1_150s.vgm` les écritures PSG du fouet pour en faire un flux de lecture (`whip_sfx.bin`). **Résultat : non réalisé.** Aucun fichier `whip_sfx.bin` n'est produit, car aucun segment n'a pu être attribué au fouet.

Méthode essayée (analyse seule, sans nouvelle capture) :
1. Décodage du VGM AY brut (`tools/convert_psg_vgm.parse_ay_vgm`), regroupement des écritures par image de 735 échantillons.
2. Recherche des instants où les volumes (R8 à R10) ou le bruit (R6) changent, puis groupement en segments.
3. Recoupement avec la chronologie d'entrée de la capture (ESPACE toutes les 2 s de 14 s à 60 s) et avec `long1-game-state.csv` (`C000=5`, partie en cours, à partir de 22,34 s).

Constats :
- Les écritures de volume et de bruit couvrent presque toute la capture à partir de 22 s : la musique occupe les trois canaux de ton et le bruit en permanence. Aucun motif n'est isolable de façon sûre.
- Le motif le plus net est une rampe de volume du canal B (R9 de 0E à 09, deux balayages) de 12 images, sans autre registre. Il est identique à chaque occurrence (45,25 s, 48,98 s, 75,15 s, 78,90 s, 105,07 s, 108,80 s, 134,97 s, 138,70 s), par paires espacées d'environ 3,7 s et de 30 s entre paires. Cet écart ne suit ni la cadence de 2 s ni celle de 5 s de l'entrée, et la voie B n'est pas la voie 4 (`C016`) que le fouet utilise. **Ce n'est donc pas attribué au fouet.** Il peut s'agir d'un effet d'ambiance ou d'une partie de la musique.
- Les instants du fouet dans cette capture (C096 et C016) ne sont pas journalisés. Le seul instant de fouet connu avec certitude vient d'une autre exécution (`scratchpad/sound/run3`, ESPACE à 22,24 s, C42F passe à 6 à 22,26 s), dont la chronologie ne se superpose pas à la capture longue.

Ce qu'il faudrait pour isoler le fouet :
- Une capture PSG courte avec ou sans une seule pression de l'attaque (B), même entrée et même moment, puis comparaison des deux VGM (différence = effet). Les runs sont déterministes pour des entrées identiques (scratchpad/sound).
- Ou un journal PSG synchronisé sur les écritures de `C096` et `C016` (point d'arrêt en écriture), pour dater le début du fouet au niveau image.
- openMSX n'est pas installé dans l'environnement actuel (`tools/run_psg_openmsx.sh` ne peut donc pas être relancé).

Statut : **non résolu**. Le lecteur PSG (`sgdk/src/psg_player.c`) et le générateur de musique ne sont pas modifiés par cette tentative.
