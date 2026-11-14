#!/usr/bin/env python3
"""AY-3-8910 music frames -> YM2612 register writes, three FM voices (channels 1 to 3).

This is the converter validated by ear in the prototype (scratchpad/fm_proto/src/fm.py,
instrument set 'A_cuivres'), moved into the build chain. Same rules, same write order:

- voice = AY channel A, B, C -> YM2612 channel 1, 2, 3 (part 0, registers ..+0, +1, +2);
- a voice is active in a frame when its tone is enabled in the mixer, its volume (R8..R10
  & 15) is above 0 and its period above 1;
- a note start (8AF1, `notes` of tools/extract_msx_sound.py) on an active voice: key off,
  then (re)write frequency, carrier TL, key on;
- the frequency is written when the AY period changes; the carrier TL (instrument TL +
  4 x (15 - volume)) when the volume changes; key off when a keyed voice becomes inactive;
- block/fnum from the AY frequency 1 789 772,5 / 16 / period with the YM2612 sample
  rate 7 670 453 / 144 (fnum = f x 2^21 / rate, block raised until fnum < 2048).

Per frame the writes come in the order the prototype sorts its timed events: every key
off, then every frequency (A4+ch, A0+ch), every carrier TL, every key on, each group by
channel. Differences with the prototype, both deliberate:
- the initial writes (instrument registers) are returned apart (init_writes), and the
  DAC enable register 2B is left out: channel 6 belongs to the PCM driver (cry of Wilhelm);
- the loop frame restates the full state (frequency and TL of every active voice, key
  off of every voice that may still be keyed), as the PSG stream does, so that the
  stream is right whether it arrives from the start or from the end of the music.
"""

FS = 7670453 / 144            # YM2612 sample rate
AYCLK = 1789772.5
NO_LOOP = None


def CAR(mul=1, dt=0, tl=0, d2r=0):
    return (mul, dt, tl, 31, 0, d2r, 0, 12)


# op = (mul, dt, tl, ar, d1r, d2r, sl, rr), order S1, S2, S3, S4 (values of the prototype)
P = {
    'saw_lead': dict(alg=4, fb=6, ops=[(1, 0, 27, 31, 6, 0, 2, 8), CAR(1, 0, 2), (1, 0, 33, 31, 4, 0, 1, 8), CAR(1, 3, 6)]),
    'saw_soft': dict(alg=4, fb=5, ops=[(1, 0, 34, 31, 6, 0, 2, 8), CAR(1, 0, 6), (1, 0, 40, 31, 4, 0, 1, 8), CAR(1, 3, 10)]),
    'bass_slap': dict(alg=4, fb=5, ops=[(1, 0, 18, 31, 12, 2, 3, 8), CAR(1, 0, 0), (0, 0, 30, 31, 8, 0, 2, 8), CAR(1, 0, 6)]),
}
# 'A_cuivres': melody (AY A), bass (AY B), harmony (AY C)
PATCHES = ('saw_lead', 'bass_slap', 'saw_soft')
CARRIERS = {0: [3], 1: [3], 2: [3], 3: [3], 4: [1, 3], 5: [1, 2, 3], 6: [1, 2, 3], 7: [0, 1, 2, 3]}
OPOFF = [0, 8, 4, 12]         # register offsets of S1, S2, S3, S4
MAX_WRITES = 63               # per frame (stream header, 6 bits)


def fnum(hz):
    block = 0
    f = hz * (1 << 21) / FS
    while f >= 2048 and block < 7:
        f /= 2
        block += 1
    return block, min(2047, int(round(f)))


def init_writes(patches=PATCHES):
    """Instrument registers, in the prototype order, without the DAC enable (2B)."""
    ev = [(0x22, 0), (0x27, 0)]
    for ch in range(3):
        p = P[patches[ch]]
        ev.append((0xB0 + ch, p['fb'] << 3 | p['alg']))
        ev.append((0xB4 + ch, 0xC0))
        ev.append((0x28, ch))
        for i, (mul, dt, tl, ar, d1r, d2r, sl, rr) in enumerate(p['ops']):
            o = OPOFF[i] + ch
            ev.append((0x30 + o, dt << 4 | mul))
            ev.append((0x40 + o, 127 if i in CARRIERS[p['alg']] else tl))
            ev.append((0x50 + o, ar))
            ev.append((0x60 + o, d1r))
            ev.append((0x70 + o, d2r))
            ev.append((0x80 + o, sl << 4 | rr))
            ev.append((0x90 + o, 0))
    return ev


def ay_voice(regs, ch):
    per = regs[2 * ch] | (regs[2 * ch + 1] & 15) << 8
    vol = regs[8 + ch] & 15
    tone = not (regs[7] >> ch) & 1
    return per, vol, tone and vol > 0 and per > 1


def convert(frames, loop=None, patches=PATCHES):
    """frames: [{'regs': [14+ AY registers], 'notes': [voices starting a note]}].
    Returns one list of (reg, value) per frame."""
    unknown = [False] * 3
    if loop is not None:
        # Key state on arrival at the loop frame (first pass) and at the end of the
        # stream (wrap): a voice whose two states differ is keyed off and on again there.
        first, end = {}, {}
        _run(frames, None, patches, first, loop)
        _run(frames, None, patches, end, len(frames))
        unknown = [first['keyed'][ch] != end['keyed'][ch] for ch in range(3)]
    return _run(frames, loop, patches, None, None, unknown)


def _run(frames, loop, patches, snapshot, snap_at, unknown=(False, False, False)):
    keyed = [False] * 3
    lastp = [0] * 3
    lastv = [-1] * 3
    out = []
    for index, f in enumerate(frames):
        if index == snap_at:
            snapshot['keyed'] = list(keyed)
        regs = f['regs']
        restate = index == loop
        if restate:
            lastp = [0] * 3
            lastv = [-1] * 3
        g_off, g_freq, g_tl, g_on = [], [], [], []
        for ch in range(3):
            per, v, active = ay_voice(regs, ch)
            p = P[patches[ch]]
            start = ch in f['notes'] and active
            may_key = keyed[ch] or (restate and unknown[ch])
            if not active:
                if may_key:
                    g_off.append((0x28, ch))
                    keyed[ch] = False
                continue
            if (start or (restate and unknown[ch])) and may_key:
                g_off.append((0x28, ch))
                keyed[ch] = False
            if per != lastp[ch] or start:
                b, fn = fnum(AYCLK / 16 / per)
                g_freq.append((0xA4 + ch, b << 3 | fn >> 8))
                g_freq.append((0xA0 + ch, fn & 255))
                lastp[ch] = per
            if v != lastv[ch] or start:
                for c in CARRIERS[p['alg']]:
                    g_tl.append((0x40 + OPOFF[c] + ch, min(127, p['ops'][c][2] + 4 * (15 - v))))
                lastv[ch] = v
            if not keyed[ch]:
                g_on.append((0x28, 0xF0 | ch))
                keyed[ch] = True
        writes = g_off + g_freq + g_tl + g_on
        if len(writes) > MAX_WRITES:
            raise ValueError(f'frame {index}: {len(writes)} FM writes')
        out.append(writes)
    if snapshot is not None and snap_at == len(frames):
        snapshot['keyed'] = list(keyed)
    return out
