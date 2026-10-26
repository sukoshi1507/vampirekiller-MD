#!/usr/bin/env python3
"""Analyse statique reproductible du dump fourni; aucune dependance externe."""
import argparse
from collections import Counter
import csv
import hashlib
import json
import math
from pathlib import Path
import struct
import zipfile

EXPECTED_SHA256 = '99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2'
BANK_SIZE = 0x2000
ROOT = Path(__file__).resolve().parents[1]


def load_rom(path):
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as archive:
            candidates = [x for x in archive.infolist()
                          if not x.is_dir() and x.filename.lower().endswith('.rom')]
            if len(candidates) != 1:
                raise ValueError('Le ZIP doit contenir exactement une ROM .rom')
            if candidates[0].file_size != 0x20000:
                raise ValueError('Taille de ROM inattendue dans le ZIP')
            data = archive.read(candidates[0])
    else:
        if path.stat().st_size != 0x20000:
            raise ValueError('Taille attendue: 131072 octets')
        data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != EXPECTED_SHA256:
        raise ValueError('SHA256 different du dump de reference: annotations non applicables')
    return data


def physical_offset(bank, address, window):
    """Resolution explicite; ne jamais confondre adresse CPU et offset du fichier."""
    if not 0 <= bank < 16 or window not in (0x4000, 0x6000, 0x8000, 0xA000):
        raise ValueError('Banque ou fenetre invalide')
    if not window <= address < window + BANK_SIZE:
        raise ValueError('Adresse hors fenetre')
    if window == 0x4000 and bank != 0:
        raise ValueError('Hypothese Konami: premiere fenetre fixe en banque 0')
    return bank * BANK_SIZE + address - window


def validate_annotations(data):
    count = byte_count = 0
    covered = set()
    for line in (ROOT / 'research/known.asm').read_text().splitlines():
        if not line.strip() or line.lstrip().startswith(';'):
            continue
        address, raw, instruction, comment = line.split('|', 3)
        address = int(address.strip(), 16)
        expected = bytes.fromhex(raw)
        offset = physical_offset(0, address, 0x4000)
        if offset + len(expected) > BANK_SIZE:
            raise ValueError('Annotation depassant la banque fixe')
        if data[offset:offset + len(expected)] != expected:
            raise ValueError(f'Annotation incorrecte a {address:04X}: {instruction}')
        span = set(range(offset, offset + len(expected)))
        if covered & span:
            raise ValueError(f'Annotations superposees a {address:04X}')
        covered |= span
        count += 1
        byte_count += len(expected)
    return {'records': count, 'bytes': byte_count, 'byte_validation': 'passed',
            'note': 'Valide les octets, pas la semantique des commentaires.'}


def analyze(data):
    annotations = validate_annotations(data)
    banks = []
    for bank in range(16):
        chunk = data[bank * BANK_SIZE:(bank + 1) * BANK_SIZE]
        entropy = -sum((n / BANK_SIZE) * math.log2(n / BANK_SIZE)
                       for n in Counter(chunk).values())
        banks.append({'bank': f'{bank:02X}', 'file_offset': f'{bank * BANK_SIZE:05X}',
                      'size': BANK_SIZE, 'entropy_bits_per_byte': round(entropy, 6),
                      'sha256': hashlib.sha256(chunk).hexdigest()})
    signatures = []
    targets = {0x5000, 0x6000, 0x6800, 0x7000, 0x7800, 0x8000, 0x9000, 0xA000, 0xB000}
    for offset in range(len(data) - 2):
        if data[offset] != 0x32:
            continue
        target = data[offset + 1] | data[offset + 2] << 8
        if target in targets:
            signatures.append({'file_offset': f'{offset:05X}', 'bank': f'{offset // BANK_SIZE:02X}',
                               'offset_in_bank': f'{offset % BANK_SIZE:04X}',
                               'candidate_instruction': f'LD (${target:04X}),A',
                               'status': 'raw_pattern_unless_annotated'})
    table_offset = physical_offset(0, 0x4161, 0x4000)
    targets14 = struct.unpack_from('<14H', data, table_offset)
    header = dict(zip(('init', 'statement', 'device', 'text'),
                      (f'{x:04X}' for x in struct.unpack_from('<4H', data, 2))))
    return {'rom': {'size': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
                    'sha1': hashlib.sha1(data).hexdigest(), 'signature': data[:2].decode('ascii'),
                    'header': header},
            'mapper': {'working_model': 'Konami non-SCC, 8 KiB banks',
                       'status': 'static evidence; confirm with emulator trace',
                       'initialization_routine': '5336',
                       'windows_after_5336': {'4000': '00', '6000': '01', '8000': '02', 'A000': '03'}},
            'annotations': annotations, 'banks': banks, 'mapper_write_candidates': signatures,
            'dispatch': {'routine': '414D', 'table': '4161', 'index_ram': 'C000',
                         'count': 14, 'runtime_index_range_verified': False,
                         'targets': [f'{x:04X}' for x in targets14]}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path, help='Fichier .rom ou archive ZIP du dump de reference')
    parser.add_argument('--out', type=Path, default=ROOT / 'analysis')
    args = parser.parse_args()
    try:
        result = analyze(load_rom(args.rom))
    except (OSError, ValueError, zipfile.BadZipFile) as error:
        parser.exit(2, f'Erreur: {error}\n')
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / 'rom.json').write_text(json.dumps(result, indent=2, ensure_ascii=False) + '\n')
    for name, records in [('banks', result['banks']), ('mapper_candidates', result['mapper_write_candidates'])]:
        with (args.out / (name + '.csv')).open('w', newline='') as output:
            writer = csv.DictWriter(output, fieldnames=list(records[0]))
            writer.writeheader()
            writer.writerows(records)
    print(f"ROM validee; {result['annotations']['records']} annotations / "
          f"{result['annotations']['bytes']} octets verifies; 16 banques; "
          f"{len(result['mapper_write_candidates'])} signatures candidates")
    print(args.out.resolve())


if __name__ == '__main__':
    main()
