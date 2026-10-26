# Integration de start.zip

Archive fournie le 26 octobre 2026 : `src/main.c`, `src/input.c`,
`src/phys.c`, `src/fix32.c`, `res/resources.res` et une ROM MSX2.
Le code adapte reste en C99 et utilise SGDK 2.11 / GCC m68k-elf.

## Routines prises en compte

| Element fourni | Integration / adaptation |
| --- | --- |
| `handleInput()` | Integre dans `sgdk/src/input.c`, avec declaration dans `input.h` et etat explicite `VKInput`. Appele une fois par image par `main.c`. |
| Gauche, droite, bouton C | Direction et demande de saut transmises au controleur existant. Gauche + droite donne une direction nulle. Le saut utilise le front du bouton pour eviter un nouveau saut automatique en le maintenant. |
| `updatePhysics()` / `getTileAt()` | Le principe commandes puis physique puis affichage est conserve. Les sondes originales dans `player.c` remplacent la matrice fictive `levelData`, qui n'est pas fournie. La carte originale utilise des tuiles de 8 pixels et un ancrage aux pieds, contre 16 pixels et un ancrage en haut a gauche dans l'exemple. |
| `fix32.c` | Sa macro `FIX32(v)` utilise 8 bits fractionnaires, tandis que SGDK 2.11 utilise 10 bits pour `fix32` et `FIX32`. Ne pas redefinir cette macro : toute future valeur Q8 devra avoir un nom distinct. Les deplacements actuels conservent les tables MSX2 et la vitesse originale de 2 pixels/image. |
| `SPR_init`, `SPR_addSprite`, `SPR_update` | Le projet emploie actuellement les sprites VDP directement, y compris les groupes composites de l'intro. Le moteur SPR n'est pas initialise en parallele : les deux gestionnaires partageraient la table de sprites et la VRAM. |
| `resources.res` | Les PNG references (`sprites/simon.png`, `tiles/castle_tiles.png`) sont absents de l'archive. Le projet conserve les ressources C extraites et deja versionnees, qui se compilent sans ces fichiers. |

Les exemples de physique et de virgule fixe ont aussi besoin de declarations
communes et d'inclusions SGDK pour compiler en unites C separees. Les coller
tels quels dans `src/` ajouterait des symboles non declares et modifierait
les collisions et le saut deja compares aux routines originales.

## ROM alternative

La ROM `Vampire Killer (Japan, Europe) (En) (Alt 3).rom` de l'archive a
l'empreinte SHA256
`377d253d285b8a4776de1dc6bacaad9f326e0d9207433269441eac25fa776a4b`.
Elle differe sur 35 556 octets du dump precedemment etudie, malgre la meme
taille de 131 072 octets. Ses adresses et tables ne sont donc pas supposees
compatibles. L'extraction et les tests restent rattaches au dump original
`99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`.
La ROM alternative n'est pas ajoutee au depot.
