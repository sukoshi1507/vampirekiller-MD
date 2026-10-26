# Compiler et verifier BASELINE-01

Validation effectuee sous Linux x86_64, Python 3.12, Java 17, GNU Make,
SGDK 2.11 et GCC m68k-elf 13.1.0 (Marsdev v1.0.0-rc1).
Les sources du portage sont en C99. D'autres environnements peuvent utiliser
le Makefile SGDK directement, mais ne sont pas couverts par cette baseline.

## Compiler

Depuis la racine du depot, avec SGDK et Marsdev deja installes :

```sh
make build GDK=/chemin/sgdk PREFIX=/chemin/mars/m68k-elf/bin/m68k-elf-
```

La ROM est produite dans `build/vampire-killer-md-viewer.bin` et
`sgdk/out/rom.bin`. Le fichier MSX2 original n'est pas necessaire a cette etape.
Le script controle les versions, active C99 et les avertissements comme
erreurs, isole les fichiers temporaires GCC et copie les objets natifs de
la bibliotheque SGDK dans un cache local. Il ne modifie pas l'installation SGDK.
Une erreur de compilation interrompt le processus et reste dans
`validation/build.log`.

```sh
make verify-build GDK=/chemin/sgdk PREFIX=/chemin/mars/m68k-elf/bin/m68k-elf-
```

Cette commande effectue deux compilations propres, compare leurs SHA256 et
verifie la signature SEGA, la taille, l'adresse de fin et le checksum SGDK.
Il faut un Java fonctionnel dans PATH ; le dossier de ses bibliotheques est
ajoute automatiquement a l'environnement de compilation sous Linux.

## Verifier la base complete

Installer les dependances Python dans un environnement virtuel, puis fournir
le dump MSX2 deja etudie et un core libretro Genesis Plus GX en RGB565 :

```sh
python3 -m pip install -r requirements-validation.txt
make verify GDK=/chemin/sgdk PREFIX=/chemin/mars/m68k-elf/bin/m68k-elf- \
  SOURCE_ROM="/chemin/Vampire Killer (Japan, Europe).zip" \
  LIBRETRO_CORE=/chemin/genesis_plus_gx_libretro.so
```

Le rapport final est `validation/baseline-checks.json`. Toute erreur arrete
la commande. Les tests couvrent les deux builds, la reference MSX2 archivee,
153 cartes originales, 53 872 cas de primitives C/Z80, le parcours du
demarrage MD, les deplacements et les 152 fonds de l'explorateur.

## Recapturer la reference MSX2

La reference utilise la distribution Linux officielle d'openMSX 19.1, avec
ses ROMs libres C-BIOS. Les URL et empreintes sont dans `config/baseline.json`.
Ce choix permet une execution complete sans ajouter de BIOS constructeur au depot.

```sh
python3 tools/capture_msx_startup.py "/chemin/Vampire Killer (Japan, Europe).zip" \
  --openmsx /chemin/openmsx/bin/openmsx --system-data /chemin/openmsx/share
python3 tools/verify_msx_startup.py "/chemin/Vampire Killer (Japan, Europe).zip"
```

Le script lance la ROM depuis la mise sous tension, presse et relache SPACE,
observe les retours des routines via des points d'arret sans suspendre le CPU,
et enregistre VRAM, palettes, registres VDP et RAM C000-FFFF. Aucun registre
CPU ni octet du jeu n'est modifie. Les ROMs du jeu et de C-BIOS ne sont pas
archivees dans le depot. Le resultat est une reference de donnees emulees,
pas une mesure d'une machine physique ou une capture native du framebuffer.

Apres une recapture voulue, comparer les changements avant de mettre a jour
l'empreinte de la reference dans `config/baseline.json`. Ne jamais regenerer
automatiquement cette reference pour faire passer un test en echec.
