# Effet sonore du fouet : provenance (castlevania-md, SFX_0A)

Statut : **prouvé** pour l'identifiant du SFX et sa place dans le pilote (lecture du code source). **Non écouté** : la conversion a été vérifiée par lecture et par simulation, pas à l'oreille.

## Source

- Projet : `castlevania-md`, portage Mega Drive (SGDK) de Castlevania NES. Lecture seule ; rien n'y a été modifié.
- Déclencheur : `src/main.c`, `whip_tick()`. Au 11e image du coup de fouet, sans arme lancée, le jeu appelle `sfx_play(SFX_0A)`. Avec le poignard lancé, il appelle `SFX_08` ; avec la montre, `SFX_0A`.
- Identifiant : `SFX_0A` = 24 (`inc/music.h`). C'est l'index 24 de `sounds[]` (`src/music.c`), entrée `{ sfx_0a, 63, 65535 }`, commentée « NES sound $0A ».
- Données : tableau `sfx_0a[63]` dans `src/music.c`. Ce sont des écritures de registres APU NES (paires registre/valeur, `0xFE n` = attente, `0xFF` = fin des écritures de l'image, sans boucle). Seuls les registres $400C à $400F (bruit) sont écrits.
- Autres SFX écartés : `SFX_14` (`sfx_14`) est le coup porté (`main.c`, « a hit »), pas le fouet.

## Licence

- castlevania-md : aucun fichier LICENSE ni mention GPL/copyleft n'a été trouvé dans le dépôt (recherche sur tout le dépôt). La mention « copyleft » vient de l'utilisateur et n'est pas vérifiée ici.
- vampirekiller-MD : pas de fichier LICENSE.
- Réserve : les données sonores d'origine (séquences APU) viennent du jeu NES, qui n'est pas sous la licence du portage. La licence d'un portage ne couvre pas forcément les données du jeu d'origine.

## Méthode de conversion

Script : `tools/build_sfx_md.py` (remplace le placeholder synthétisé). Il lit `sfx_0a` dans `../castlevania-md/src/music.c` (par défaut ; `--source-dir` pour un autre chemin), vérifie l'index `SFX_0A` et la taille, puis rejoue le tableau comme `src/sound.c` :

1. `player_step` : lecture des écritures APU, attentes `0xFE`, fin de l'image `0xFF`, arrêt sans boucle.
2. `apu_clock` : enveloppes et compteur de longueur du bruit (pas de balayage, pas de triangle pour ce son).
3. `sound_out`, canal bruit uniquement : la période du bruit `$400E` est mise sur le ton 2 de la PSG (horloge du bruit), le volume est converti avec la table d'atténuation de `sound.c` (+2 pas, plafond 15). Le bruit est écrit en bruit blanc, horloge du ton 2 (`0xE7`), comme `sound.c`.
4. `psg_set` : écriture seulement si la valeur change, comme `psg_last` dans `sound.c`.

Le résultat est un flux par image à 60 Hz au format de `psg_player.c` (un octet de compte, puis les octets PSG ; `0xFF` termine le flux). Le son dure 15 images (environ 250 ms). Il n'y a pas de boucle.

Vérifications faites : le script échoue si le tableau change de taille, si l'index ne correspond pas à `SFX_0A`, ou si le son écrit un autre canal que le bruit. Le flux a été relu image par image contre les écritures `sfx_0a`.

## Limites

- Le pilote NES du jeu d'origine joue le fouet avec un mixage de voies (le pilote de `castlevania-md` gère aussi la musique en dessous). Ici le fouet est seul sur la PSG : pas de musique concurrente, donc pas de mixage.
- Le flux écrase le ton 2 et le bruit de la PSG pendant le son, comme dans `castlevania-md`. Il ne gère pas les autres voies de la musique de `vk_psg_play`, qui remplace déjà le flux en cours (comportement existant, non changé).
- La mode du bruit (bit 7 de `$400E`) est ignorée, comme dans `sound.c`.
- Non écouté dans un émulateur ni sur matériel.

## Fichiers

- `tools/build_sfx_md.py` : générateur.
- `sgdk/src/generated/sfx_md.c` et `sfx_md.h` : sortie générée, symbole `vk_sfx_whip` inchangé.
