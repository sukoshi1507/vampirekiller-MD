#!/usr/bin/env python3
"""Build every Vampire Killer (MSX2) sound for the Mega Drive PSG from the ROM.

Usage: python3 tools/build_msx_sound_md.py [ROM_OR_ZIP]   (default: VK_ROM_PATH)

1. tools/extract_msx_sound.py runs the ROM sound driver (Z80 emulation) and
   returns the AY writes of each 60 Hz frame for effects 01..1C, music 80..8E,
   the timed item loop FB and the pause jingle FD.
2. tools/convert_psg_vgm.py (FrameConverter) turns them into SN76489 writes:
   effects keep AY channel C (SN tone 2) and the noise, FB and FD keep channel A,
   music keeps the three channels and the noise.
3. Output sgdk/src/generated/msx_sound_md.{c,h}, read by sgdk/src/psg_player.c.
   Stream format, one entry per frame:
       header: bits 0-5 = number of PSG bytes that follow,
               bit 6 = a music note starts on channel C this frame (8AF1),
               bit 7 = effects are locked after this frame (C0A8, music E6/EB);
       0xFF ends the stream (the player jumps to `loop`, or stops when 0xFFFF).
   A summary goes to research/audio/msx_sounds.json.
Never hand edit the generated files.
"""
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))

from convert_psg_vgm import convert_frames  # noqa: E402
from extract_msx_sound import extract  # noqa: E402
from probe import load_rom, reference_rom  # noqa: E402

ROM_SHA256 = '99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2'
END = 0xFF
NOTE_C = 0x40
LOCK = 0x80
NO_LOOP = 0xFFFF
EFFECT_SLOTS = 0x1D          # 01..1C used, 00 is the stop request
MUSIC_SLOTS = 0x10           # 80..8E used, 8F points at an empty sequence

# Names from the callers of 509F (research/audio-driver.md). PROVISIONAL unless
# the note there says proven.
NAMES = {
    0x01: 'ENERGIE', 0x02: 'OBJET 94FC', 0x03: 'ESCALIER A', 0x04: 'ARME LANCEE',
    0x05: 'FOUET', 0x06: 'ESCALIER B', 0x07: 'ATTERRISSAGE', 0x08: 'ENNEMI A2F9',
    0x09: 'EAU', 0x0A: 'ENNEMI B9A9', 0x0B: 'ARME TOUCHE', 0x0C: 'COUP',
    0x0D: 'CONTACT 7EBC', 0x0E: 'BLOC CACHE', 0x0F: 'COEUR', 0x10: 'POINTS',
    0x11: 'COFFRE', 0x12: 'OBJET', 0x13: 'DEGATS', 0x14: 'CLE', 0x15: 'PORTE',
    0x16: 'OBJET 08', 0x17: 'FIN MINUTERIE', 0x18: 'ARME 7414', 0x19: 'OBJET 938F',
    0x1A: 'PORTE 1A', 0x1B: 'OBJET 05', 0x1C: 'SON 7E1C',
    0x80: 'STAGES 0-3', 0x81: 'STAGES 4-6', 0x82: 'STAGES 7-9', 0x83: 'STAGES 16-17',
    0x84: 'STAGES 13-15', 0x85: 'STAGES 10 18', 0x86: 'BOSS FINAL', 0x87: 'BOSS',
    0x88: 'MUSIQUE 88', 0x89: 'MORT', 0x8A: 'DEPART', 0x8B: 'GAME OVER',
    0x8C: 'MUSIQUE 8C', 0x8D: 'MUSIQUE 8D', 0x8E: 'MUSIQUE 8E',
    0xFB: 'MINUTERIE FB', 0xFD: 'PAUSE FD',
}


def channels_of(sound):
    if sound['kind'] == 'effect':
        return (2,)          # the effect channel is AY C (89BF: C094 = 2)
    if sound['kind'] in ('special', 'pause'):
        return (0,)          # FB and FD use AY A (89F6, 520B: C094 = 0)
    return (0, 1, 2)


def encode(sound):
    """Stream bytes and loop offset for one extracted sound."""
    frames, stats = convert_frames(sound['frames'], channels_of(sound), sound['loop'])
    note_c, lock = set(sound['note_c']), set(sound['lock'])
    stream = bytearray()
    offsets = []
    for index, writes in enumerate(frames):
        if len(writes) > 0x3F:
            raise ValueError(f"sound {sound['id']:02X}: {len(writes)} PSG bytes in frame {index}")
        offsets.append(len(stream))
        header = len(writes)
        if index in note_c:
            header |= NOTE_C
        if index in lock:
            header |= LOCK
        stream.append(header)
        stream += writes
    stream.append(END)
    loop = NO_LOOP if sound['loop'] is None else offsets[sound['loop']]
    return bytes(stream), loop, stats


def c_array(name, data):
    lines = [f'static const VKPsgByte {name}[{len(data)}] = {{']
    for i in range(0, len(data), 16):
        lines.append('    ' + ', '.join('0x%02X' % b for b in data[i:i + 16]) + ',')
    lines.append('};')
    return lines


def generate(rom_path=None, out_dir=ROOT / 'sgdk/src/generated', summary=ROOT / 'research/audio/msx_sounds.json'):
    data = load_rom(Path(rom_path) if rom_path else reference_rom())
    digest = hashlib.sha256(data).hexdigest()
    if digest != ROM_SHA256:
        raise ValueError(f'unexpected ROM: {digest}')
    extracted = extract(data)
    body, entries, report = [], {}, []
    total = 0
    for sound in extracted['sounds']:
        stream, loop, stats = encode(sound)
        sid = sound['id']
        name = f'msx_sound_{sid:02x}'
        body += c_array(name, stream)
        entries[sid] = (name, loop)
        total += len(stream)
        report.append({
            'id': f'{sid:02X}', 'kind': sound['kind'], 'name': NAMES.get(sid, ''),
            'frames': len(sound['frames']), 'loop_frame': sound['loop'],
            'loop_offset': None if loop == NO_LOOP else loop, 'bytes': len(stream),
            'note_c_frames': len(sound['note_c']), 'lock_frames': len(sound['lock']),
            'ay_writes': sum(len(f) for f in sound['frames']),
            'conversion': dict(sorted(stats.items())),
            'sha256': hashlib.sha256(stream).hexdigest(),
        })

    def entry(sid):
        if sid not in entries:
            return '    {0, 0xFFFF},'
        name, loop = entries[sid]
        return f'    {{{name}, 0x{loop:04X}}},  /* {sid:02X} {NAMES.get(sid, "")} */'

    c = ['/* Generated from the Vampire Killer MSX2 ROM by tools/build_msx_sound_md.py.',
         ' * The ROM sound driver (bank 14) is run frame by frame (tools/extract_msx_sound.py)',
         ' * and its AY writes are converted for the SN76489 (tools/convert_psg_vgm.py).',
         ' * Format: see msx_sound_md.h. Never hand edit. */',
         '#include "msx_sound_md.h"', '']
    c += body
    c += ['', f'const VKSoundStream vk_msx_effects[VK_MSX_EFFECT_SLOTS] = {{']
    c += [entry(sid) for sid in range(EFFECT_SLOTS)]
    c += ['};', '', f'const VKSoundStream vk_msx_music[VK_MSX_MUSIC_SLOTS] = {{']
    c += [entry(0x80 + i) for i in range(MUSIC_SLOTS)]
    c += ['};', '']
    c += ['/* Sound test labels (PROVISIONAL names, see research/audio-driver.md). */',
          'const char *const vk_msx_effect_names[VK_MSX_EFFECT_SLOTS] = {']
    c += [f'    "{NAMES.get(sid, "")}",' for sid in range(EFFECT_SLOTS)]
    c += ['};', 'const char *const vk_msx_music_names[VK_MSX_MUSIC_SLOTS] = {']
    c += [f'    "{NAMES.get(0x80 + i, "")}",' for i in range(MUSIC_SLOTS)]
    c += ['};', '']
    c += [f'const VKSoundStream vk_msx_timed_item = {{{entries[0xFB][0]}, 0x{entries[0xFB][1]:04X}}};',
          f'const VKSoundStream vk_msx_pause = {{{entries[0xFD][0]}, 0x{entries[0xFD][1]:04X}}};']
    h = ['/* Generated by tools/build_msx_sound_md.py from the Vampire Killer MSX2 ROM. Never hand edit.',
         ' * One stream per MSX sound request (509F). Per 60 Hz frame: a header byte',
         ' * (bits 0-5: count of PSG bytes that follow; bit 6: a music note starts on',
         ' * channel C, which ends the mute left by an effect (8B00); bit 7: effects',
         ' * are refused after this frame (C0A8)), then the PSG bytes. 0xFF ends the',
         ' * stream; the player then jumps to byte offset `loop`, or stops if 0xFFFF.',
         ' * Effects write SN tone 2 and noise only; music writes all four channels. */',
         '#ifndef VK_MSX_SOUND_MD_H', '#define VK_MSX_SOUND_MD_H', '#include "../psg_core.h"', '',
         f'#define VK_MSX_EFFECT_SLOTS {EFFECT_SLOTS}  /* index = MSX effect id 00..1C (00: none) */',
         f'#define VK_MSX_MUSIC_SLOTS {MUSIC_SLOTS}   /* index = MSX music id - 0x80 (8F: none) */',
         'extern const VKSoundStream vk_msx_effects[VK_MSX_EFFECT_SLOTS];',
         'extern const VKSoundStream vk_msx_music[VK_MSX_MUSIC_SLOTS];',
         'extern const char *const vk_msx_effect_names[VK_MSX_EFFECT_SLOTS];',
         'extern const char *const vk_msx_music_names[VK_MSX_MUSIC_SLOTS];',
         'extern const VKSoundStream vk_msx_timed_item;  /* FB: channel A loop until FC */',
         'extern const VKSoundStream vk_msx_pause;       /* FD: pause jingle, channel A */',
         '', '#endif']
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / 'msx_sound_md.c').write_text('\n'.join(c) + '\n')
    (out_dir / 'msx_sound_md.h').write_text('\n'.join(h) + '\n')
    summary.parent.mkdir(parents=True, exist_ok=True)
    summary.write_text(json.dumps({
        'source': extracted['source'], 'rom_sha256': digest, 'frame_rate': 60,
        'effect_pointers': extracted['effect_pointers'],
        'music_pointers': extracted['music_pointers'],
        'total_bytes': total, 'sounds': report}, indent=1, ensure_ascii=False) + '\n')
    return total, len(report)


if __name__ == '__main__':
    size, count = generate(sys.argv[1] if len(sys.argv) > 1 else None)
    print(f'{count} sounds, {size} stream bytes -> sgdk/src/generated/msx_sound_md.c')
