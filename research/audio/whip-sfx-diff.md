# Effet sonore du fouet : différence de deux captures PSG (openMSX)

Statut : **prouvé** pour le déclenchement et la fenêtre temporelle de l'effet ; **non exploitable** en l'état comme flux isolé (voir Limites). Aucun fichier `whip_sfx_diff.vgm` n'est produit.

## Méthode

- Émulateur : openMSX 21.0, machine C-BIOS_MSX2, ROM de référence `scratchpad/speed/game.rom` (ROM non copiée ni committée).
- Séquence commune : `keymatrixdown 8 1` puis relâchement 0,2 s après le point d'arrêt `4194` (titre passé à 10,71 s). Partie en cours (`C000=5`, `C001=0`) atteinte à 19,24 s (image 910).
- Capture A : aucun appui, 45 s. Capture B : une pression SPACE (masque 1) à 40,000 s pendant 0,1 s, rien d'autre.
- Enregistrement : `vgm_rec start PSG MSX-Music` (format AY-3-8910, commande 0xA0, 735 échantillons par image à 44 100 Hz).
- Journal CPU : écritures sur `C096`, `C016`, `C42F`, `C097` avec PC de l'écrivain et image.
- Scripts (hors dépôt) : `scratchpad/whipdiff/cap.tcl` (B) et `scratchpad/whipdiff/cap2.tcl` (appui déclenché depuis le point d'arrêt `4028`, image 2153). Données : `scratchpad/whipdiff/{A,B,E,F,...}/`.

## Résultats

### Déterminisme

- A, A2 et A3 (mêmes entrées) : flux VGM identiques, écriture par écriture.
- F (appui déclenché depuis `4028`, sans appui réel) : identique à A.
- B et B2 : identiques entre eux.

### Chronologie du fouet (journal CPU, prouvé)

| Instant (s) | Image | Événement |
|---|---|---|
| 40,0000 | 2153 | SPACE enfoncé (`keymatrixdown 8 1`) |
| 40,0083 | 2154 | `C42F=06` (PC 721D), `C096=05` (PC 517D) |
| 40,0086 | 2154 | `C016=9C` (PC 519E) |
| 40,0437 | 2156 | `C42F=07` (PC 7243) |
| 40,0763 | 2158 | `C42F=08` (PC 725D) |
| 40,1000 | 2159 | SPACE relâché |
| 40,2100 | 2166 | `C42F=00` (PC 7280) |
| 40,2179 | 2167 | `C096=00` (PC 89D7), `C016=F1` (PC 89E9) |

- Le début de l'attaque est à l'image 2154, soit 1 image après l'appui (2153). La fin est à 2167, soit 13 images après le début. Ces valeurs sont cohérentes avec `research/audio/triggers.json`.
- Les écritures `C096` et `C016` sont donc prouvées, avec leurs PC, pour le fouet au sol.

### Différence VGM (A contre B, ou F contre E selon le run)

| Segment (s) | Écritures A | Écritures B | Écritures différentes (B seule / A seule) |
|---|---|---|---|
| 39,0 à 39,9 | 158 | 158 | 0 / 0 |
| 39,9 à 40,0 (avant l'appui) | 13 | 37 | 29 / 5 |
| 40,0 à 40,3 | 55 | 61 | 26 / 20 |
| 40,3 à 44,9 | 758 | 758 | 0 / 0 |

- Après 40,3 s, le flux est strictement identique : l'effet est local et le pilote musical reprend à l'identique.
- Dans 40,0 à 40,3 s, les différences portent sur : le mélangeur R07 (`9C` au lieu de `B8`, 14 écritures contre 15), le canal C (R04/R05 mis à 0, R10 à 0/3/14 au lieu de 10 à 13) et le bruit R06 (4, 8, 12). Ce sont des registres de la voie 4 du pilote, en accord avec `C016`.

## Points établis / déduits / inconnus

- **Prouvé** : l'appui SPACE produit `C096=05` et `C016=9C` à l'image suivante (journal CPU, PC identifiés). L'effet se termine à l'image 2167.
- **Prouvé** : les états CPU (journaux `C096`, `C016`, `C42F`, `C097`) sont identiques entre les runs sans appui et avec appui jusqu'à 40,000 s.
- **Prouvé** : après 40,3 s, les flux PSG de la capture avec appui et de la capture sans appui sont identiques.
- **Déduit** : l'effet du fouet occupe la fenêtre 40,0 à 40,3 s et touche les registres de la voie 4 (R04/R05/R10), avec des changements de mélangeur R07 et de bruit R06.
- **Inconnu / non résolu** : des écritures PSG diffèrent dès 39,92 s dans les captures avec appui, soit environ 60 ms avant l'appui. Les états CPU ne divergent pas avant 40,0 s, donc ce décalage est probablement un artefact d'horodatage du VGM de openMSX lors d'un événement clavier. Non démontré.

## Limites

- La différence n'est pas un flux isolé : la musique continue en dessous (voies 1 à 3 et mélangeur), et le fouet écrase des registres partagés avec le pilote. Une soustraction donnerait un signal mélangé, sans enveloppe fiable.
- Le décalage pré-appui (39,92 s) empêche de considérer la soustraction comme propre à 100 %.
- Aucune écoute ni rendu de référence. Le lecteur Mega Drive n'a pas été testé.
- Le VGM dure 44,90 s pour une capture de 45 s (fin d'enregistrement à l'arrêt, non corrigé).

## Fichiers

- Captures et journaux : `scratchpad/whipdiff/A/`, `scratchpad/whipdiff/B/`, `scratchpad/whipdiff/E/`, `scratchpad/whipdiff/F/`, `scratchpad/whipdiff/H/`, `scratchpad/whipdiff/I/`
- Analyses : `scratchpad/whipdiff/diff.py`, `scratchpad/whipdiff/win.py`, `scratchpad/whipdiff/win2.py`, `scratchpad/whipdiff/cmp.py`
