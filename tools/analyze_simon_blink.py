#!/usr/bin/env python3
"""Analyse d'une trace de tools/run_simon_blink_trace.py (clignotement de Simon).

Usage : analyze_simon_blink.py OUTDIR [--png DIR]

Image N = l'image affichee entre les VBLANK N et N+1. Les tampons D400 (couleurs des
sprites de Simon) et D600 (SAT) sont envoyes en VRAM par 658A / 6560, ~1 ms apres le
VBLANK, au debut de chaque passe : l'image N montre les tampons du dernier envoi fait au
VBLANK N ou avant (une passe de retard sur le remplissage 790C / 78CC). Le script le
verifie contre la VRAM lue au VBLANK N+1 (colonnes vram_col / vram_y de blink.csv).

Pour chaque minuterie lancee (ecriture d'une valeur > 0 hors des baisses 75D2 / 75E8) :
image et passe de l'ecriture, C003 a cet instant, premiere et derniere image touchee
(silhouette 08h / 0Eh pour C434 / C43A, Y de sprite DEh = cache pour C42D), nombre
d'images touchees et alternance. Avec --png, une planche par episode (zone de Simon,
rendue depuis les instantanes VRAM shots/fNNNNN, NNNNN = N + 1).
"""
import argparse
import csv
from pathlib import Path

DECREMENTS = {'75D2', '75E8'}
TIMERS = {'C434': 'c434', 'C43A': 'c43a', 'C42D': 'c42d'}


def load(out):
    frames = {int(r['frame']): r for r in csv.DictReader((out / 'blink.csv').open())}
    events = list(csv.DictReader((out / 'blink_ev.csv').open()))
    return frames, events


def displayed(events, first, last):
    """Etat affiche (d400, d600) de chaque image de first a last, d'apres les envois 658A."""
    shown = {}
    cur = None
    uploads = {}
    for e in events:
        if e['what'] == 'col_upload':
            uploads[int(e['frame'])] = (int(e['d400']), int(e['d600']))
    for n in range(first, last + 1):
        if n in uploads:
            cur = uploads[n]
        shown[n] = cur
    return shown


def check_vram(frames, shown):
    bad = []
    for n, st in shown.items():
        r = frames.get(n + 1)
        if st is None or r is None or int(r['vram_idx']) < 0:
            continue
        if int(r['vram_col']) != st[0] or int(r['vram_y']) != st[1]:
            bad.append(n)
    return bad


def episodes(frames, events, shown):
    res = []
    starts = [e for e in events if e['what'].startswith('write_') and e['pc'] not in DECREMENTS
              and int(e[TIMERS[e['what'][6:]]]) > 0]
    passes = sorted(int(e['frame']) for e in events if e['what'] == 'c003_inc')
    for e in starts:
        name = e['what'][6:]
        col = TIMERS[name]
        w = int(e['frame'])
        if not any(p <= w for p in passes):
            continue                 # remise a zero de l'initialisation (830F)
        pass_start = max(p for p in passes if p <= w)
        zero = next(int(x['frame']) for x in events
                    if x['what'] == 'write_' + name and int(x['frame']) > w and int(x[col]) == 0)
        if name == 'C42D':
            hit = lambda st: st[1] == 0xDE
        else:
            color = 8 if name == 'C434' else 14
            hit = lambda st, c=color: st[0] == c
        imgs = [n for n in range(w, zero + 12) if shown.get(n) and hit(shown[n])]
        runs = []
        for n in range(w, zero + 12):
            v = bool(shown.get(n) and hit(shown[n]))
            if runs and runs[-1][0] == v:
                runs[-1][1] += 1
            else:
                runs.append([v, 1])
        res.append(dict(timer=name, value=int(e[col]), pc=e['pc'], write_frame=w, write_us=int(e['us']),
                        pass_start=pass_start, c003=int(frames[pass_start + 1]['c003']) if pass_start + 1 in frames else None,
                        zero_frame=zero, first=imgs[0] if imgs else None, last=imgs[-1] if imgs else None,
                        count=len(imgs), runs=runs))
    return res


def render_crop(base, cx, cy, half=24):
    from PIL import Image
    vram = (base.with_suffix('.vram')).read_bytes()
    info = {}
    for line in base.with_suffix('.txt').open():
        k, _, v = line.strip().partition(' ')
        info[k] = v
    regs = [int(t) for t in info['regs'].split()]
    p = [int(t) for t in info['pal'].split()]
    pal = []
    for i in range(16):
        b0, b1 = p[2 * i], p[2 * i + 1]
        pal.append((((b0 >> 4) & 7) * 255 // 7, (b1 & 7) * 255 // 7, (b0 & 7) * 255 // 7))
    lines = 212 if regs[9] & 0x80 else 192
    page = ((regs[2] >> 5) & 3) * 0x8000
    img = Image.new('RGB', (256, lines))
    px = img.load()
    for y in range(lines):
        row = page + ((y + regs[23]) & 255) * 128
        for x in range(0, 256, 2):
            v = vram[row + x // 2]
            px[x, y] = pal[v >> 4]
            px[x + 1, y] = pal[v & 15]
    col = ((regs[11] & 3) << 15) | ((regs[5] & 0xF8) << 7)
    sat = col + 0x200
    spg = (regs[6] & 0x3F) << 11
    buf = {}
    if not regs[8] & 2:
        for i in range(32):
            y, x, pat = vram[sat + 4 * i], vram[sat + 4 * i + 1], vram[sat + 4 * i + 2] & 0xFC
            if y == 216:
                break
            for r in range(16):
                sy = (y + 1 + r) & 255
                if sy >= lines:
                    continue
                c = vram[col + 16 * i + r]
                cc, color = c & 0x40, c & 15
                xx = x - 32 if c & 0x80 else x
                bits = (vram[spg + pat * 8 + r] << 8) | vram[spg + pat * 8 + 16 + r]
                for b in range(16):
                    if bits & (0x8000 >> b) and 0 <= xx + b < 256:
                        k = (xx + b, sy)
                        if cc and k in buf:
                            buf[k] = buf[k] | color
                        elif not cc and k not in buf:
                            buf[k] = color
    for (sx, sy), color in buf.items():
        if color:
            px[sx, sy] = pal[color]
    box = (max(cx - half, 0), max(cy - 2 * half, 0), min(cx + half, 256), min(cy + 8, lines))
    return img.crop(box)


def sheet(out, ep, dest, frames):
    from PIL import Image, ImageDraw
    picks = list(range(ep['pass_start'] - 2, ep['pass_start'] + 12)) + \
        list(range(ep['zero_frame'] - 6, ep['zero_frame'] + 6))
    tiles = []
    for n in picks:
        base = out / 'shots' / ('f%05d' % (n + 1))
        if not base.with_suffix('.vram').is_file():
            continue
        r = frames[n + 1]
        t = render_crop(base, int(r['x']), int(r['y']) - 16).resize((96, 112), Image.NEAREST)
        canvas = Image.new('RGB', (96, 126), (40, 40, 40))
        canvas.paste(t, (0, 14))
        ImageDraw.Draw(canvas).text((2, 1), 'img %d' % n, fill=(255, 255, 0))
        tiles.append(canvas)
    if not tiles:
        return None
    per = 14
    rows = (len(tiles) + per - 1) // per
    im = Image.new('RGB', (96 * min(per, len(tiles)), 126 * rows), (0, 0, 0))
    for i, t in enumerate(tiles):
        im.paste(t, (96 * (i % per), 126 * (i // per)))
    path = dest / ('blink_%s_%d.png' % (ep['timer'], ep['write_frame']))
    im.save(path)
    return path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out', type=Path)
    ap.add_argument('--png', type=Path)
    a = ap.parse_args()
    frames, events = load(a.out)
    shown = displayed(events, min(frames), max(frames) - 1)
    bad = check_vram(frames, shown)
    checked = sum(1 for n in shown if n + 1 in frames and int(frames[n + 1]['vram_idx']) >= 0)
    print('VRAM au VBLANK N+1 = envoi du VBLANK N : %d images verifiees, %d ecarts %s'
          % (checked, len(bad), bad[:10]))
    for n in bad[:3]:
        print('  ecart image %d : affiche %s, VRAM %s' % (n, shown[n], frames[n + 1]))
    for ep in episodes(frames, events, shown):
        print('%s = %d (PC %s) ecrit image %d (+%d us), passe commencee a l\'image %d avec C003 = %s'
              % (ep['timer'], ep['value'], ep['pc'], ep['write_frame'], ep['write_us'], ep['pass_start'], ep['c003']))
        print('  minuterie a 0 dans l\'image %d ; images touchees %s a %s (%d), soit +%s a +%s apres le debut de la passe'
              % (ep['zero_frame'], ep['first'], ep['last'], ep['count'],
                 ep['first'] - ep['pass_start'] if ep['first'] is not None else '-',
                 ep['last'] - ep['pass_start'] if ep['last'] is not None else '-'))
        print('  alternance (touche, nb) :', ' '.join('%s%d' % ('X' if v else '.', k) for v, k in ep['runs'][:12]),
              '...', ' '.join('%s%d' % ('X' if v else '.', k) for v, k in ep['runs'][-6:]))
        if a.png:
            a.png.mkdir(parents=True, exist_ok=True)
            p = sheet(a.out, ep, a.png, frames)
            if p:
                print('  planche :', p)


if __name__ == '__main__':
    main()
