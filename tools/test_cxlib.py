"""Offline tests for cxlib. Run: .venv/bin/python -m unittest discover -s tools

These need no camera. They pin down the one thing the project builds itself
on the PC side: updater payloads for model ids PMCA does not ship.
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cxlib


class PayloadTest(unittest.TestCase):
    def test_rebuild_is_byte_identical_for_every_shipped_model(self):
        # If our builder reproduces every header PMCA ships, bit for bit,
        # then the header it emits for a new model id is trustworthy too.
        ids = cxlib.model_ids()
        self.assertGreater(len(ids), 30)
        for name, (model, region) in ids.items():
            with self.subTest(model=name):
                self.assertEqual(cxlib.build_payload(model, region), cxlib.vendor_payload(name))

    def test_only_the_header_depends_on_the_model(self):
        a = cxlib.build_payload(0x21010001)
        b = cxlib.build_payload(0x21010099)
        self.assertNotEqual(a[:cxlib.HDR_SIZE], b[:cxlib.HDR_SIZE])
        self.assertEqual(a[cxlib.HDR_SIZE:], b[cxlib.HDR_SIZE:])

    def test_new_model_payload_decodes_with_the_requested_ids(self):
        arch, f, _ = cxlib.decode_payload(cxlib.build_payload(0x21010099, region=1))
        self.assertEqual(arch, cxlib.ARCH)
        self.assertEqual((f.model, f.region, f.version), (0x21010099, 1, '9.99'))

    def test_sibling_ids(self):
        ids = cxlib.model_ids()
        self.assertEqual(ids['HDR-PJ590V'], (0x21010001, 0))
        self.assertEqual(ids['HDR-PJ760V'], (0x21010031, 0))
        self.assertEqual(ids['HDR-TD20'], (0x21010052, 0))


if __name__ == '__main__':
    unittest.main()
