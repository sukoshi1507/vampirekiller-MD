#!/usr/bin/env python3
"""Build every Vampire Killer (MSX2) sound for the Mega Drive PSG from the ROM.

Usage: python3 tools/build_msx_sound_md.py [ROM_OR_ZIP]   (default: VK_ROM_PATH)

1. tools/extract_msx_sound.py runs the ROM sound driver (Z80 emulation) and
   returns the AY writes of each 60 Hz frame for effects 01..1D, music 80..8E,
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
NAME_MAX = 16               # sound test column
EFFECT_SLOTS = 0x1E          # 01..1D used, 00 is the stop request
MUSIC_SLOTS = 0x10           # 80..8E used, 8F points at an empty sequence

# Every sound request of the ROM, proven from the Z80 code (research/audio-driver.md
# section 3). id -> (name, event, callers). Names: capitals, at most 16 characters
# (sound test menu, MSX_TEXT_TILE_MAX). A caller is (kind, address): "call" = the CALL
# or JP 509F whose A is loaded just before (LD A,id two bytes earlier), "wrap" = CALL A4E8
# (bank 3 wrapper: 509F unless C420 = 6), "ret" = the LD A,id of an 8D30 pickup handler
# (the value returns to 509F, pushed by 8D2F), "table" = 7972[D000] (796F), "cond" = LD A,id
# followed by a JR that selects between two ids (77F7). tests/test_extract_msx_sound.py scans
# the ROM and fails if a caller is missing here or listed without being in the ROM.
SOUNDS = {
    0x01: ('SOIN ENERGIE', 'stage cleared after a boss: C415 refilled step by step (B736)', [('call', 0xB744)]),
    0x02: ('REFUS MARCHAND', 'merchant answer no, not enough hearts or timeout (94FC)', [('call', 0x94FE)]),
    0x03: ('CROIX TOURNE', 'thrown cross (type 4) spins, every 8 frames (72DE)', [('call', 0x72F5)]),
    0x04: ('DAGUE LANCEE', 'dagger (type 2) leaves Simon (7436)', [('call', 0x7470)]),
    0x05: ('COUP DE FOUET', 'whip stroke starts, C42F = 6 (7213)', [('call', 0x7225)]),
    0x06: ('HACHE TOURNE', 'thrown axe (type 3) spins, every 8 frames (7473)', [('call', 0x7489)]),
    0x07: ('ATTERRISSAGE', 'Simon lands (6D38 jump, 6F7E fall) or a fish man lands (A310)',
           [('call', 0x6D38), ('call', 0x6F7E), ('wrap', 0xA34F)]),
    0x08: ('POISSON SAUTE', 'a fish man (object 02/03, init A2E0) leaps out of the water',
           [('wrap', 0xA2FB)]),
    0x09: ('PLONGEON EAU', 'Simon falls into the water (stages 2, 10) or a fish man dives (A310)',
           [('call', 0x76E5), ('wrap', 0xA388)]),
    0x0A: ('TIR BOSS 9', 'stage 9 boss (object 14h) throws its ball (B985)', [('wrap', 0xB9AB)]),
    0x0B: ('PROJECTILE PARE', 'shield (object 04) stops an enemy projectile (8610)', [('call', 0x8631)]),
    0x0C: ('COUP TOUCHE', 'whip or thrown weapon touches a candle, enemy or object',
           [('call', 0x7DD6), ('call', 0x7F67), ('call', 0x7F9B), ('call', 0x7FD8), ('call', 0x804E),
            ('call', 0x80D1), ('call', 0x8118), ('call', 0x8160)]),
    0x0D: ('TUE AU CONTACT', 'an enemy touched by Simon is destroyed while C434 is set (object 09, 7E67)',
           [('call', 0x7EBE)]),
    0x0E: ('BLOC CASSE', 'a hit candle is a block, mode 2 or 3 (87D2)', [('call', 0x87D8)]),
    0x0F: ('COEUR CLE ROUGE', 'hearts picked up (objects 01, 02), red key (object 12), wall hearts + 5',
           [('ret', 0x8DB4), ('ret', 0x8E90), ('call', 0x9346)]),
    0x10: ('BONUS POINTS', 'points: objects 13, 14 picked up (8E42, 8E4B) and the wall man gone (9358)',
           [('ret', 0x8E48), ('call', 0x936F)]),
    0x11: ('COFFRE OUVERT', 'chest opened with a gold key (8F68)', [('call', 0x8F7D)]),
    0x12: ('OBJET PRIS', 'item picked up (8D30 handlers) or bought from the merchant (94EF)',
           [('ret', 0x8D8A), ('ret', 0x8E30), ('call', 0x94F1)]),
    0x13: ('SIMON BLESSE', 'start of the recoil after a contact, C002 bit 6 (6F93)', [('call', 0x6F9A)]),
    0x14: ('CLE PRISE', 'gold key (object 17) or white key (object 18) picked up',
           [('ret', 0x8E5D), ('ret', 0x8E68)]),
    0x15: ('PORTE PASSAGE', 'castle door (C5AC = 5, 7772) or secret passage entered (6DB5)',
           [('call', 0x6DC5), ('call', 0x7774)]),
    0x16: ('INVULNERABLE', 'object 08 picked up: C43A = 96h, no contact (8DCD)', [('ret', 0x8DDB)]),
    0x17: ('FIN INVULNERABLE', 'C43A reaches 10h: the invulnerability is ending (75E2)', [('call', 0x75EF)]),
    0x18: ('EAU BENITE SOL', 'thrown holy water (type 5) lands on the floor (73DE)', [('call', 0x7416)]),
    0x19: ('MARCHAND CARTE', 'merchant window opens (938F) or the stage map opens, F2 (959E)',
           [('call', 0x9391), ('call', 0x95A0)]),
    0x1A: ('PORTE STAGE', 'inner door of a stage passed, C5AC != 5 (772A)', [('call', 0x776F)]),
    0x1B: ('ENNEMIS DETRUITS', 'object 05 picked up: every enemy and projectile destroyed (8DBB)',
           [('ret', 0x8DCA)]),
    0x1C: ('MORT DU BOSS', 'boss gauge C418 reaches 0 (7E17)', [('call', 0x7E1E)]),
    0x1D: ('COEURS PERDUS', 'wall man result: hearts - 5 (934E)', [('call', 0x9350)]),
    0x80: ('STAGES 0-3', 'stage music 7972[D000], D000 = 0..3 (cour, hall, 2, 3)', [('table', 0x796F)]),
    0x81: ('STAGES 4-6 11 12', 'stage music 7972[D000], D000 = 4, 5, 6, 11, 12', [('table', 0x796F)]),
    0x82: ('STAGES 7-9', 'stage music 7972[D000], D000 = 7, 8, 9', [('table', 0x796F)]),
    0x83: ('STAGES 16-17', 'stage music 7972[D000], D000 = 16, 17', [('table', 0x796F)]),
    0x84: ('STAGES 13-15', 'stage music 7972[D000], D000 = 13, 14, 15', [('table', 0x796F)]),
    0x85: ('STAGES 10 18', 'stage music 7972[D000], D000 = 10, 18', [('table', 0x796F)]),
    0x86: ('ENTREE DRACULA', 'Dracula room entered, D000 = 12h (77D1)', [('call', 0x77FD)]),
    0x87: ('ENTREE BOSS', 'boss room entered, D000 != 12h (77D1)', [('cond', 0x77F7)]),
    0x88: ('DRACULA FORME 2', 'Dracula first form gone: second form starts (CE01 = 3, 6619)', [('call', 0x661B)]),
    0x89: ('MORT SIMON', 'Simon dies, after the stop 00 (70AA)', [('call', 0x70AC)]),
    0x8A: ('DEPART', 'start of a game, state 3 (4222)', [('call', 0x4224)]),
    0x8B: ('GAME OVER', 'no life left, state 6 (42AB)', [('call', 0x42AD)]),
    0x8C: ('VICTOIRE BOSS', 'boss orb taken: stage cleared (B720, B72A)', [('call', 0xB722), ('call', 0xB72C)]),
    0x8D: ('DRACULA VAINCU', 'Dracula second form killed (CE01 = 6, 667F)', [('call', 0x6681)]),
    0x8E: ('GENERIQUE', 'credits start (CE40 = 1, 66C9)', [('call', 0x66CB)]),
    0xFB: ('ARRET DU TEMPS', 'time stop used: hearts - 5, C43B = 5Ah/96h (718F)', [('call', 0x7191)]),
    0xFD: ('PAUSE', 'pause key in play (5C57)', [('call', 0x5C59)]),
}
# The other requests of 509F, not extracted as sounds: 00 stop, FC end of FB, FE end of pause, FF fade.
CONTROLS = {
    0x00: [('call', 0x43BE), ('call', 0x6613), ('call', 0x70A7), ('call', 0xB727)],
    0xFC: [('call', 0x7601)],
    0xFE: [('call', 0x43E8)],
    0xFF: [('call', 0x6788), ('call', 0x7765)],
}
NAMES = {sid: entry[0] for sid, entry in SOUNDS.items()}


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
    for sid, name in NAMES.items():
        if len(name) > NAME_MAX or name != name.upper() or not name.replace(' ', '').replace('-', '').isalnum():
            raise ValueError(f'sound {sid:02X}: bad label {name!r}')
    c += ['/* Sound test labels, one per proven event (SOUNDS in tools/build_msx_sound_md.py). */',
          'const char *const vk_msx_effect_names[VK_MSX_EFFECT_SLOTS] = {']
    c += [f'    "{NAMES.get(sid, "")}",' for sid in range(EFFECT_SLOTS)]
    c += ['};', 'const char *const vk_msx_music_names[VK_MSX_MUSIC_SLOTS] = {']
    c += [f'    "{NAMES.get(0x80 + i, "")}",' for i in range(MUSIC_SLOTS)]
    c += ['};', '']
    c += ['const char *const vk_msx_special_names[2] = {', f'    "{NAMES[0xFB]}",', f'    "{NAMES[0xFD]}",', '};', '']
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
         f'#define VK_MSX_EFFECT_SLOTS {EFFECT_SLOTS}  /* index = MSX effect id 00..1D (00: none) */',
         f'#define VK_MSX_MUSIC_SLOTS {MUSIC_SLOTS}   /* index = MSX music id - 0x80 (8F: none) */',
         'extern const VKSoundStream vk_msx_effects[VK_MSX_EFFECT_SLOTS];',
         'extern const VKSoundStream vk_msx_music[VK_MSX_MUSIC_SLOTS];',
         'extern const char *const vk_msx_effect_names[VK_MSX_EFFECT_SLOTS];',
         'extern const char *const vk_msx_music_names[VK_MSX_MUSIC_SLOTS];',
         'extern const char *const vk_msx_special_names[2];  /* labels of FB, FD */',
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
