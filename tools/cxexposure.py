#!/usr/bin/env python3
"""Summarize camera metadata in original AVCHD or HDR-CX580V STD clips."""

import argparse
import csv
import json
import subprocess
import sys
from pathlib import Path


FIELDS = (
    ('Model', 'Camera'),
    ('FNumber', 'Aperture'),
    ('ExposureTime', 'Shutter'),
    ('Gain', 'Gain'),
    ('ExposureProgram', 'Exposure program'),
    ('ApertureSetting', 'Aperture setting'),
    ('WhiteBalance', 'White balance'),
    ('Focus', 'Focus'),
)
EXPOSURE_TAGS = ('FNumber', 'ExposureTime', 'Gain')
STD_EXTENSIONS = {'.mpg', '.mpeg'}
STD_WHITE_BALANCE = {0x00: 'Auto', 0x02: 'One Push', 0x11: 'Indoor'}
STD_FOCUS = {0x00: 'Auto', 0x01: 'Manual'}


def exposure_values(record):
    """Collect distinct values in the order ExifTool reports them."""
    values = {tag: [] for tag, _ in FIELDS}
    for key, value in record.items():
        group, _, tag = key.rpartition(':')
        if group.startswith('H264:') and tag in values and value is not None:
            values[tag].append(str(value))
    return {tag: ', '.join(dict.fromkeys(items)) for tag, items in values.items()}


def sony_std_records(source):
    """Yield CX580V ARI_DATA records from the MPEG-2 program stream."""
    while header := source.read(4):
        if not header.startswith(b'\x00\x00\x01') or len(header) != 4:
            raise ValueError('Invalid MPEG program stream start code')
        stream_id = header[3]
        if stream_id == 0xba:
            pack = source.read(10)
            if len(pack) != 10 or pack[0] & 0xc0 != 0x40:
                raise ValueError('Invalid MPEG-2 pack header')
            stuffing = pack[-1] & 7
            if len(source.read(stuffing)) != stuffing:
                raise ValueError('Truncated MPEG-2 pack')
            continue
        if stream_id == 0xb9:
            continue

        length = source.read(2)
        if stream_id < 0xbb or len(length) != 2:
            raise ValueError('Invalid MPEG packet header')
        size = int.from_bytes(length, 'big')
        packet = source.read(size)
        if len(packet) != size:
            raise ValueError('Truncated MPEG packet')
        if stream_id != 0xbd or len(packet) < 3:
            continue

        payload = packet[3 + packet[2]:]
        if payload.startswith(b'\xffARI_DATA\x01\x05\xff\xffSONY\x00'):
            record = payload[1:]
            if record[0x18b:0x196] == b'HDR-CX580V\x00':
                yield record


def sony_std_exposure(clip):
    """Collect the fields verified on the HDR-CX580V."""
    values = {tag: {} for tag, _ in FIELDS}
    with clip.open('rb') as source:
        for record in sony_std_records(source):
            values['Model']['HDR-CX580V'] = None
            iris = int.from_bytes(record[0xa8:0xaa], 'big')
            denominator = int.from_bytes(record[0xac:0xae], 'big')
            exposure_mode, gain = record[0xae:0xb0]
            if 0 < iris < 0xffff:
                values['FNumber'][str(iris / 100)] = None
            if record[0xaa:0xac] == b'\x00\x01' and 0 < denominator < 0xffff:
                values['ExposureTime'][f'1/{denominator}'] = None
            if gain < 0x80:
                values['Gain'][f'{gain} dB'] = None
            mode = {1: 'Manual', 2: 'Auto'}.get(exposure_mode)
            if mode is not None:
                values['ExposureProgram'][mode] = None
            white_balance = STD_WHITE_BALANCE.get(record[0xb8])
            if white_balance is not None:
                values['WhiteBalance'][white_balance] = None
            focus = STD_FOCUS.get(record[0xba])
            if focus is not None:
                values['Focus'][focus] = None
    return {tag: ', '.join(items) for tag, items in values.items()}


def read_avchd(clips, parser):
    if not clips:
        return {}, False
    command = ['exiftool', '-q', '-ee', '-a', '-G1:3:4', '-json', '-Error']
    command.extend('-H264:' + tag for tag, _ in FIELDS)
    command.extend(str(clip) for clip in clips)

    try:
        result = subprocess.run(command, capture_output=True, text=True, encoding='utf-8')
    except FileNotFoundError:
        parser.exit(1, 'ExifTool is required to read AVCHD metadata.\n')

    if result.stderr:
        print(result.stderr, file=sys.stderr, end='')
    if not result.stdout:
        return {}, True
    try:
        records = json.loads(result.stdout)
    except (ValueError, RecursionError) as exc:
        parser.exit(1, f'Cannot read ExifTool output: {exc}\n')
    return {record['SourceFile']: record for record in records}, result.returncode != 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('clips', metavar='CLIP', type=Path, nargs='+')
    args = parser.parse_args()

    clips = [clip.absolute() for clip in args.clips]
    avchd = [clip for clip in clips if clip.suffix.lower() not in STD_EXTENSIONS]
    records, incomplete = read_avchd(avchd, parser)

    table = csv.writer(sys.stdout, delimiter='\t', lineterminator='\n')
    table.writerow(['Clip', *(label for _, label in FIELDS)])
    for clip in clips:
        if clip.suffix.lower() in STD_EXTENSIONS:
            error = 'No HDR-CX580V STD exposure metadata found'
            try:
                exposure = sony_std_exposure(clip)
            except (OSError, ValueError) as exc:
                exposure = {}
                error = str(exc)
        else:
            record = records.get(str(clip), {})
            exposure = exposure_values(record)
            error = record.get('ExifTool:Main:Error', 'No H.264 exposure metadata found')
        table.writerow([clip, *(exposure.get(tag) or '—' for tag, _ in FIELDS)])
        if not any(exposure.get(tag) for tag in EXPOSURE_TAGS):
            incomplete = True
            print(f'{clip}: {error}', file=sys.stderr)
    return int(incomplete)


if __name__ == '__main__':
    sys.exit(main())
