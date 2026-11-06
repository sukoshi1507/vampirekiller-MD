# Réinstaller l'environnement de travail (Arch, Omarchy)

Liste relevée sous CachyOS le 2026-11-04. Testée sous Omarchy le 2026-11-05 : mêmes versions, ROM identique octet par octet, tests et scénarios passés.

## Hors du dépôt, à sauvegarder

- SGDK (`Stephane-D/SGDK`, commit `ee6870a`), avec `makefile.gen`, `makelib.gen` et `libmd.a` modifiés
  localement : à copier tel quel, à côté du dépôt.
- La ROM MSX (`scratchpad/rom/`, zip à la racine) et le reste de `scratchpad/` : traces, données lues
  par défaut par certains outils.
- `corrections/`, `walkthrough.txt`, `.claude/settings.local.json`.
- `~/.claude`, `~/.claude.json` (mémoire et historique, rangés selon le chemin du dépôt : le garder
  identique), `~/.openMSX`.

## Paquets

```sh
sudo pacman -S --needed git github-cli base-devel python jdk-openjdk unzip libretro-genesis-plus-gx
yay -S --needed m68k-elf-binutils m68k-elf-gcc m68k-elf-newlib openmsx z80dasm blastem-bin
```

Versions utilisées : `m68k-elf-gcc` 16.1.0, `m68k-elf-binutils` 2.46.1, `m68k-elf-newlib` 4.6.0,
`openmsx` 22.0, `libretro-genesis-plus-gx` 2026-06, Python 3.14. `java` sert à `rescomp` et `sizebnd`,
`/usr/lib/libretro/genesis_plus_gx_libretro.so` à `tools/md_harness.py`, `blastem-bin` est facultatif.

## Mise en route

```sh
python -m venv scratchpad/venv
scratchpad/venv/bin/pip install -r requirements.txt
cd sgdk && make GDK=/chemin/vers/SGDK && cd ..
sh tests/run_native.sh
sh tests/run_md_scenarios.sh /tmp/vk-md
scratchpad/venv/bin/python -m unittest discover -s tests -p 'test_*.py'
```

Attendu : aucun avertissement, `PASS`, `ALL MD SCENARIOS PASSED`, `OK`. Si un scénario échoue, comparer
d'abord la version de `m68k-elf-gcc` : une autre version peut générer un autre code (bug déjà vu avec
`-O3` et LTO, `sgdk/src/actors.c`). Si l'édition de liens échoue, recompiler `libmd.a`
(`make -f makelib.gen` dans SGDK).
