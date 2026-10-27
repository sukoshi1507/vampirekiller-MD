#!/usr/bin/env python3
"""Inspecte un VGM AY-3-8910 (MSX PSG, commande 0xA0) et le convertit en VGM SN76489 (PSG Mega Drive).

Hypothese de conversion (documentee, candidate) :
- Ton : periode AY P (12 bits) -> periode SN N = P. Avec horloge MSX 1789773 Hz (AY, diviseur 16)
  et horloge MD 3579545 Hz (SN, diviseur 32), la frequence est identique pour N = P. N est borne a 1..1023 ;
  les depassements sont comptes (`tone_clipped`), pas inventes.
- Volume : AY 0..15 (environ 1.5 dB par pas) -> attenuation SN 0..15 (2 dB par pas), 15 = silence.
  Volume 0 -> silence. L'enveloppe AY (registres 11-13, bit 4 des volumes) n'est pas reproduite :
  elle est comptee (`envelope_writes`) et doit etre traitee separement si elle apparait.
- Bruit : une seule source SN. Le bruit AY (registre 6) est ramene au taux SN le plus proche (512/1024/2048)
  en bruit blanc. Plusieurs canaux bruites : le plus fort est conserve (`noise_conflicts`).
- Melange ton + bruit sur un meme canal AY : le ton est conserve (`tone_noise_mix`), le bruit est perdu.
- Le SN n'a pas de sortie de bruit independante du ton : aucun canal SN n'est reserve au bruit au-dela de SN3.

Les temps (commandes d'attente VGM) sont recopies tels quels. La precision temporelle est donc celle du
VGM source (voir README dans research/audio/).
"""
import argparse
import json
import struct
import sys
from collections import Counter
from pathlib import Path

AY_CLOCK = 1789773   # horloge PSG MSX (3579545 / 2)
SN_CLOCK = 3579545   # horloge PSG Mega Drive (NTSC)
NOISE_RATES = (512, 1024, 2048)  # diviseurs SN pour les taux 0..2 (taux 3 = suit le ton 2, non utilise)


def parse_ay_vgm(data):
    """Retourne (writes, stats) avec writes = liste de (sample_time, reg, value)."""
    if data[:4] != b'Vgm ':
        raise ValueError('Entete VGM absent')
    data_offset = struct.unpack_from('<I', data, 0x34)[0]
    start = 0x34 + data_offset if data_offset else 0x40
    pos, t, writes, unsupported = start, 0, [], Counter()
    while pos < len(data):
        c = data[pos]
        if c == 0xA0:
            writes.append((t, data[pos + 1], data[pos + 2]))
            pos += 3
        elif c == 0x61:
            t += struct.unpack_from('<H', data, pos + 1)[0]
            pos += 3
        elif c == 0x62:
            t += 735
            pos += 1
        elif c == 0x63:
            t += 882
            pos += 1
        elif 0x70 <= c <= 0x7F:
            t += (c & 15) + 1
            pos += 1
        elif c == 0x66:
            break
        elif c == 0x67:
            pos += 7 + struct.unpack_from('<I', data, pos + 3)[0]
        elif c in (0x4F, 0x50) or 0x30 <= c <= 0x3F:
            unsupported[f'{c:02X}'] += 1
            pos += 2
        elif 0x51 <= c <= 0x5F or 0xB0 <= c <= 0xBF or 0xA1 <= c <= 0xAF:
            unsupported[f'{c:02X}'] += 1
            pos += 3
        elif 0xC0 <= c <= 0xDF:
            unsupported[f'{c:02X}'] += 1
            pos += 4
        elif 0xE0 <= c <= 0xFF:
            unsupported[f'{c:02X}'] += 1
            pos += 5
        else:
            raise ValueError(f'Commande VGM inconnue {c:#x} a {pos:#x}')
    return writes, t, dict(unsupported)


def ay_tone_period(regs, channel):
    return ((regs[channel * 2 + 1] & 0x0F) << 8) | regs[channel * 2]


def sn_latch_tone(channel, n):
    return bytes([0x80 | (channel << 5) | (n & 0x0F), (n >> 4) & 0x3F])


def ay_volume_to_sn_attenuation(volume):
    if volume == 0:
        return 15
    return min(14, round((15 - volume) * 1.5 / 2))


def nearest_noise_rate(np):
    """Taux SN 0..2 dont la frequence est la plus proche de la frequence bruit AY."""
    if np == 0:
        np = 1
    f_ay = AY_CLOCK / (16 * np)
    best, best_err = 0, None
    for rate, div in enumerate(NOISE_RATES):
        f_sn = SN_CLOCK / (16 * div)
        err = abs(f_sn - f_ay)
        if best_err is None or err < best_err:
            best, best_err = rate, err
    return best


def convert(writes, total_samples):
    regs = [0] * 16
    sn_tone = [None, None, None]     # (N,) envoye en dernier
    sn_noise = None                  # taux bruit envoye en dernier
    sn_att = [15, 15, 15, 15]
    out = [(0, bytes([0x9F, 0xBF, 0xDF, 0xFF]))]  # etat initial SN : tout en silence
    stats = Counter()
    events = {}
    for t, reg, val in writes:
        if 0 <= reg < 16:
            regs[reg] = val
        if reg == 11 or reg == 12 or reg == 13 or ((reg in (8, 9, 10)) and (val & 0x10)):
            stats['envelope_writes'] += 1
        events.setdefault(t, set()).add(reg)
    # Recalcule l'etat a chaque instant ou un registre change
    regs = [0] * 16
    times = sorted(events)
    w_by_t = {}
    for t, reg, val in writes:
        w_by_t.setdefault(t, []).append((reg, val))
    for t in times:
        for reg, val in w_by_t[t]:
            regs[reg] = val
        chunk = bytearray()
        mixer = regs[7]
        # Canal bruit : le canal AY le plus fort qui a le bruit active
        noise_candidates = []
        tone_out = {}
        for ch in range(3):
            volume = regs[8 + ch] & 0x0F
            tone_on = not (mixer >> ch) & 1
            noise_on = not (mixer >> (3 + ch)) & 1
            if (regs[8 + ch] & 0x10):
                volume = 15  # enveloppe non reproduite : volume max (approximation signalee)
            if tone_on and noise_on:
                stats['tone_noise_mix'] += 1
            if noise_on and volume > 0:
                noise_candidates.append((volume, ch))
            if tone_on:
                period = ay_tone_period(regs, ch)
                if period > 1023:
                    stats['tone_clipped'] += 1
                n = min(1023, max(1, period))
                tone_out[ch] = (n, ay_volume_to_sn_attenuation(volume) if volume else 15)
            else:
                # Ton AY coupe : le canal SN correspondant est muet (le bruit passe par SN3)
                tone_out[ch] = (None, 15)
        for ch in range(3):
            n, att = tone_out[ch]
            if n is not None and (sn_tone[ch] is None or sn_tone[ch][0] != n):
                chunk += sn_latch_tone(ch, n)
                sn_tone[ch] = (n, None)
                stats['sn_tone_writes'] += 1
            if sn_att[ch] != att:
                chunk += bytes([0x90 | (ch << 5) | att])
                sn_att[ch] = att
                stats['sn_volume_writes'] += 1
        # Bruit : canal SN3 (index 3) en bruit blanc, taux le plus proche
        if noise_candidates:
            if len(noise_candidates) > 1:
                stats['noise_conflicts'] += 1
            volume, ch = max(noise_candidates)
            np = regs[6] & 0x1F
            rate = nearest_noise_rate(np)
            att = ay_volume_to_sn_attenuation(volume)
            if sn_noise != rate:
                chunk += bytes([0xE4 | rate])
                sn_noise = rate
                stats['sn_noise_writes'] += 1
            if sn_att[3] != att:
                chunk += bytes([0xF0 | att])
                sn_att[3] = att
                stats['sn_volume_writes'] += 1
        else:
            if sn_att[3] != 15:
                chunk += bytes([0xFF])
                sn_att[3] = 15
                stats['sn_volume_writes'] += 1
        if chunk:
            out.append((t, bytes(chunk)))
    return out, stats


def build_vgm(events, total_samples):
    """Assemble un VGM SN76489 (commande 0x50) a partir de (time, bytes)."""
    body = bytearray()
    cur = 0
    for t, chunk in events:
        delta = t - cur
        while delta > 0:
            step = min(delta, 0xFFFF)
            body += b'\x61' + struct.pack('<H', step)
            delta -= step
            cur += step
        for byte in chunk:
            body += bytes([0x50, byte])
    remaining = total_samples - cur
    while remaining > 0:
        step = min(remaining, 0xFFFF)
        body += b'\x61' + struct.pack('<H', step)
        remaining -= step
    body += b'\x66'
    header = bytearray(0x40)
    header[0:4] = b'Vgm '
    struct.pack_into('<I', header, 0x04, 0x40 + len(body) - 4)
    struct.pack_into('<I', header, 0x08, 0x00000150)
    struct.pack_into('<I', header, 0x0C, SN_CLOCK)
    struct.pack_into('<I', header, 0x18, total_samples)
    struct.pack_into('<I', header, 0x34, 0x40 - 0x34)
    return bytes(header) + bytes(body)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('input', type=Path, help='VGM AY-3-8910 (openMSX vgm_rec)')
    parser.add_argument('--sn-out', type=Path, help='VGM SN76489 de sortie (candidat)')
    parser.add_argument('--report', type=Path, help='rapport JSON')
    args = parser.parse_args()
    data = args.input.read_bytes()
    writes, total, unsupported = parse_ay_vgm(data)
    events, stats = convert(writes, total)
    report = {
        'input': args.input.name, 'input_bytes': len(data),
        'ay_writes': len(writes), 'ay_registers': sorted({r for _, r, _ in writes}),
        'total_samples': total, 'total_seconds': round(total / 44100, 3),
        'unsupported_commands': unsupported,
        'conversion': dict(stats), 'sn_event_times': len(events),
        'clocks': {'ay_hz': AY_CLOCK, 'sn_hz': SN_CLOCK},
        'status': 'candidate',
        'caveats': [
            'Enveloppe AY non reproduite (volume max applique si bit 4 actif).',
            'Melange ton+bruit : ton conserve.',
            'Bruit reduit a trois taux SN, bruit blanc.',
            'Ecarts de frequence dus au bornage 1..1023 seulement.',
        ],
    }
    if args.sn_out:
        vgm = build_vgm(events, total)
        args.sn_out.parent.mkdir(parents=True, exist_ok=True)
        args.sn_out.write_bytes(vgm)
        report['sn_out'] = {'name': args.sn_out.name, 'bytes': len(vgm)}
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n')
    print(json.dumps(report, indent=2, ensure_ascii=False))
    return 0


if __name__ == '__main__':
    sys.exit(main())
