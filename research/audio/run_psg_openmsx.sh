#!/bin/sh
# Capture PSG/BIOS-PSG de Vampire Killer MSX2 sous openMSX (lecture seule de la ROM).
# Usage: run_psg_openmsx.sh OUTDIR SECONDS "t,row,col,hold t,row,col,hold ..."
# Variables: OPENMSX (binaire), OPENMSX_SYSTEM_DATA (share), VK_ROM_PATH (ROM, via tools/probe.py), EXT (ex: "-ext ...").
# Ne pas utiliser -control stdio avec -cart dans cet environnement : la capture reste bloquee.
set -e
OUT=$1; SECS=$2; KEYS=$3
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OM=${OPENMSX:-openmsx}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
python3 -I -c "
import sys; sys.path.insert(0, '$ROOT/tools')
import probe; open('$TMP/game.rom', 'wb').write(probe.load_rom(probe.reference_rom()))"
mkdir -p "$OUT"
{
  echo "set vk_output {$OUT}"
  echo "set vk_seconds $SECS"
  echo "set vk_keys {$KEYS}"
  echo "set vk_mem_watch ${MEMWATCH:-0}"
  cat "$ROOT/tools/trace_psg_openmsx.tcl"
} > "$TMP/trace.tcl"
SDL_AUDIODRIVER=dummy OPENMSX_SYSTEM_DATA="${OPENMSX_SYSTEM_DATA:-}" OPENMSX_USER_DATA="$OUT/user" \
  $OM -machine C-BIOS_MSX2 ${EXT:-} -cart "$TMP/game.rom" -script "$TMP/trace.tcl" </dev/null > "$OUT/openmsx.log" 2>&1
echo "fini: $OUT/psg-writes.csv, $OUT/vgm_recordings/music0001.vgm"
