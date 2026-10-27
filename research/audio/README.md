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
