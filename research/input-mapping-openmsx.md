# Entrées clavier en openMSX : mapping et vitesse de marche

Détails complets et scripts : `research/input-gate.md`.

## Correction

Une version précédente de cette note concluait à tort que Simon ne se déplaçait pas. La cause était une mauvaise lecture de `keymatrixdown` : la commande prend un **masque de bits** par ligne, et non un index de colonne. `keymatrixdown 8 1` appuie donc sur SPACE (bit 0 du masque 1), et `keymatrixdown 8 7` appuie sur SPACE + HOME + INS. Les appuis « impairs » déclenchaient le fouet pour cette raison.

## Masques utiles (ligne 8, matrice MSX standard)

| Touche | Masque |
| --- | --- |
| SPACE (attaque, B) | 1 |
| LEFT | 16 |
| UP | 32 |
| DOWN | 64 |
| RIGHT | 128 |

Chacun produit l'effet attendu sur `C427`, `C425`, `C42E`/`C42F` et `C007`.

## Vitesse de marche mesurée

- Simon avance de **1 px par VBLANK** (60 px/s) à droite, mesuré sur 120 images.
- Le compteur logique `C003` avance toutes les 2 images, donc 2 px par tick logique : c'est la valeur 0200 de `6C2F/6C53`.
- Cadence d'animation : `C42E` suit 0, 1, 2, 1 avec 8 images par pose, soit un cycle de 32 images (environ 533 ms).

Le portage MD a été aligné sur ces mesures : 1 px par image et cycle `{0,1,2,1}` tenu 8 images. Le vérificateur `tools/verify_play_mode.py` a été mis à jour en conséquence (30 px en 30 images, 10 px en 10 images vers la gauche).

## Non établi

- La routine exacte qui lit la ligne 8 (`IN A,(A9)`) n'a pas été tracée. Le mapping vient de la matrice standard et du comportement observé.
- La marche au-delà de 2 s et près des murs n'a pas été mesurée.
