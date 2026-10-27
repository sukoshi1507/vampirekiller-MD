# Entrées clavier en openMSX : état de la mesure de vitesse

Objectif : mesurer la vitesse de marche de Simon dans la ROM MSX2 (position `C427`) pour la comparer au portage MD.

## Vérifié

- Modèle `C-BIOS_MSX2`, ROM de référence, `keymatrixdown 8 1` (SPACE) déclenche l'attaque à chaque appui : séquence `C42F` 6, 7, 8 puis retour à 0, comme dans `simon-assets/ATTACK_TRACE_OPENMSX.md`.
- Le jeu passe en partie (`C000=5`) vers 19 s après le démarrage, sans entrée.
- Balayage de la matrice : rangées 0 à 10, colonnes 0 à 7, chacune maintenue 0,4 s pendant la partie. Aucune combinaison ne déplace Simon (`C427` reste à 16, `C425` à 176), y compris les colonnes de la rangée 8 où se trouvent d'ordinaire les curseurs. Le saut n'a pas non plus été observé.
- La ROM lit le clavier directement par les ports `A9` (données) et `AA` (sélection de ligne), et non par les appels BIOS `GTSTCK` (`00D5`) ou `GTTRIG` (`00D8`), absents de la ROM.
- Aucune commande de contrôle joystick n'est exposée par ce build openMSX (`info commands *joy*` vide). `get_pluggable_for_connector joyporta` ne renvoie aucun périphérique.

## Non résolu

- Quelle combinaison d'entrée fait marcher Simon. Hypothèses restantes : une ligne de matrice non testée, un mode d'entrée choisi au titre, ou une lecture par le port joystick (`A9` bits de direction) qu'openMSX ne pilote pas depuis le script.
- Mesure de la vitesse dynamique : bloquée tant que l'entrée n'est pas trouvée.

## Vitesse estimée (non mesurée)

Le code de marche `6C2F/6C53` applique 0200 en 8.8, soit 2 px par tick (voir `player-format.md`). Le portage MD déplace Simon de 2 px par image. Si la boucle MSX tourne à 60 Hz, les deux vitesses coïncident. Cette équivalence reste déduite du code.
