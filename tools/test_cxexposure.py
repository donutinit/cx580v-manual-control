"""Tests for the exposure summary."""

from io import BytesIO
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from cxexposure import exposure_values, sony_std_exposure, sony_std_records


def packet(stream_id, payload):
    return b'\x00\x00\x01' + bytes([stream_id]) + len(payload).to_bytes(2, 'big') + payload


def sony_packet(camera_data, white_balance=0xff, focus=0xff):
    record = bytearray(0x196)
    signature = b'ARI_DATA\x01\x05\xff\xffSONY\x00'
    record[:len(signature)] = signature
    record[0x18b:] = b'HDR-CX580V\x00'
    record[0xa8:0xb0] = bytes.fromhex(camera_data)
    record[0xb8] = white_balance
    record[0xba] = focus
    return packet(0xbd, b'\x81\x00\x00\xff' + record)


class ExposureValuesTest(unittest.TestCase):
    def test_keeps_distinct_h264_values_across_documents(self):
        values = exposure_values({
            'SourceFile': 'clip.MTS',
            'H264:Main:FNumber': 1.8,
            'H264:Doc2:Copy1:FNumber': 2.8,
            'H264:Doc3:Copy2:FNumber': 1.8,
            'H264:Main:Gain': '0 dB',
            'H264:Doc2:Copy1:Gain': '3 dB',
            'H264:Main:Focus': 'Auto (0.2)',
            'Composite:Main:FNumber': 8,
        })
        self.assertEqual(values['FNumber'], '1.8, 2.8')
        self.assertEqual(values['Gain'], '0 dB, 3 dB')
        self.assertEqual(values['Focus'], 'Auto (0.2)')
        self.assertEqual(values['ExposureTime'], '')

    def test_keeps_numeric_zero(self):
        values = exposure_values({'H264:Main:Gain': 0})
        self.assertEqual(values['Gain'], '0')


class SonyStdExposureTest(unittest.TestCase):
    def test_decodes_sony_records_and_ignores_embedded_start_codes(self):
        manual = sony_packet('0190 0001 00fa 0100', white_balance=0x02, focus=0x01)
        auto = sony_packet('00b4 0001 0064 0206', white_balance=0x11, focus=0x00)
        video = packet(0xe0, sony_packet('00b4 0001 007d 0206'))
        with TemporaryDirectory() as directory:
            clip = Path(directory) / 'clip.MPG'
            clip.write_bytes(manual + video + auto + manual)
            values = sony_std_exposure(clip)
        self.assertEqual(values['Model'], 'HDR-CX580V')
        self.assertEqual(values['FNumber'], '4.0, 1.8')
        self.assertEqual(values['ExposureTime'], '1/250, 1/100')
        self.assertEqual(values['Gain'], '0 dB, 6 dB')
        self.assertEqual(values['ExposureProgram'], 'Manual, Auto')
        self.assertEqual(values['WhiteBalance'], 'One Push, Indoor')
        self.assertEqual(values['Focus'], 'Manual, Auto')

    def test_rejects_truncated_packet(self):
        data = sony_packet('0190 0001 00fa 0100')
        with self.assertRaisesRegex(ValueError, 'Truncated MPEG packet'):
            list(sony_std_records(BytesIO(data[:-1])))
