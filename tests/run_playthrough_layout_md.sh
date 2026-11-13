#!/bin/sh
# The frame-exact replay of stages 12-18 must not depend on the layout of the compiled code. Two builds of a copy of sgdk/ with the layout
# changed without any change of logic are replayed, with the tool checks of run_playthrough_12_18_md.sh (checkpoints, no lost frame):
#   dead  = a function added to main.c (different LTO inlining and addresses, a ROM that is bigger)
#   pad   = 4 bytes of padding at the end of VKBoss (every later RAM address and the code that uses them move)
# (research/room-draw-budget.md: before the frame budget work, either of them made the replay lose a frame and diverge.)
# VK_REPLAY=file replays another file than tests/md_playthrough/playthrough_12_18.txt. Needs SGDK: GDK=/path/to/SGDK (default /home/sukoshi/Documents/githubsuko/SGDK). Slow: two builds (~40 s each) and two replays.
set -eu
cd "$(dirname "$0")/.."
: "${VK_MD_CORE:?Set VK_MD_CORE}"
GDK="${GDK:-/home/sukoshi/Documents/githubsuko/SGDK}"
WORK="${VK_LAYOUT_OUT:-scratchpad/layout}"
rm -rf "$WORK"; mkdir -p "$WORK"
for variant in dead pad; do
    dir="$WORK/$variant"
    mkdir -p "$dir/sgdk"
    cp -r sgdk/src sgdk/res sgdk/Makefile "$dir/sgdk/"
    ln -s "$PWD/tools" "$dir/tools"
    case $variant in
        dead) printf '\n__attribute__((used, noinline)) unsigned long vk_layout_dead(unsigned long x) { return x * 3 + 7; }\n' >> "$dir/sgdk/src/main.c" ;;
        pad)  sed -i 's/^} VKBoss;/    unsigned char vk_layout_pad[4];\n} VKBoss;/' "$dir/sgdk/src/boss.h" ;;
    esac
    (cd "$dir/sgdk" && make GDK="$GDK" >"../build.log" 2>&1) || { tail -n 20 "$dir/build.log"; exit 1; }
    cmp -s sgdk/out/rom.bin "$dir/sgdk/out/rom.bin" && { echo "$variant: the ROM did not change, no layout test"; exit 1; }
    echo "== variant $variant: ROM $(stat -c %s "$dir/sgdk/out/rom.bin") bytes"
    VK_MD_ROM="$PWD/$dir/sgdk/out/rom.bin" VK_MD_ELF="$PWD/$dir/sgdk/out/rom.out" \
        ${PYTHON:-python3} tools/playthrough_12_18_md.py --core "$VK_MD_CORE" --out "$dir/play" ${VK_REPLAY:+--replay "$VK_REPLAY"}
done
echo "layout variants OK"
