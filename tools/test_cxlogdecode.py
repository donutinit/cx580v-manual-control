import unittest

import cxlogdecode


class SummaryTest(unittest.TestCase):
    def test_values_reported_by_the_camera(self):
        self.assertEqual(cxlogdecode.summarize('6.3a4 NF fno=118 min=64 mode=f3').split(None, 1)[1],
                         'camera reports F2.8')
        self.assertEqual(cxlogdecode.summarize('13.10 NF shut=1/b4 mode=f4').split(None, 1)[1],
                         'camera reports shutter 1/180')
        self.assertEqual(cxlogdecode.summarize('24.2b0 NI m=8588 p9706:7=1ec').split(None, 1)[1],
                         'camera reports gain 12 dB')

    def test_time_is_decimal_seconds(self):
        self.assertTrue(cxlogdecode.summarize('1a.200 NF fno=b4 min=64 mode=0').startswith('  26.50 '))

    def test_injections_and_rejections(self):
        ok = cxlogdecode.summarize('1a.0 IN 421ac500 c=3100 m=75b6 n=2 p0:a=43b917e0 p96d6:7=1cd ret=0')
        self.assertIn('cxg sets shutter manual', ok)
        self.assertNotIn('REJECTED', ok)
        bad = cxlogdecode.summarize('1a.0 IN 421ac500 c=3100 m=7610 n=2 p0:a=43d278d8 p9795:7=2d8 ret=ffff')
        self.assertIn('cxg sets AE lock off', bad)
        self.assertIn('REJECTED ret=ffff', bad)
        cap = cxlogdecode.summarize('23.0 IN 421ac500 c=3100 m=75ba n=3 p0:a=43b91828 p96dc:3=12 p96dd:8=1d0 ret=0')
        self.assertIn('gain cap 18 dB', cap)
        off = cxlogdecode.summarize('23.0 IN 421ac500 c=3100 m=75ba n=3 p0:a=43b91828 p96dc:3=0 p96dd:8=1d1 ret=0')
        self.assertIn('gain cap off', off)

    def test_exposure_mode_requests(self):
        line = '12.100 OA 421ac500 c=3100 m=7558 n=2 p0:a=43b91770 p95c8:7=f4'
        self.assertIn('exposure mode: shutter priority', cxlogdecode.summarize(line))

    def test_opened_screens_and_menu_rules(self):
        self.assertEqual(cxlogdecode.summarize('75.73 OV 4235ff80 "view/IRIS_L" n=0').split(None, 1)[1],
                         'screen opens view/IRIS_L')
        self.assertIn('MN show menu=c347c id=c',
                      cxlogdecode.summarize('38.271 MN show menu=c347c id=c state 0->2 valid 1->1'))
        self.assertIn('TI icon 2200025 -> 19b', cxlogdecode.summarize('8a.196 TI icon 2200025 -> 19b'))

    def test_uninteresting_lines_are_dropped(self):
        self.assertIsNone(cxlogdecode.summarize('5.0 GI 42198980 "model/CAMERA" q=f out k12b:1=6 ret=1'))
        self.assertIsNone(cxlogdecode.summarize('not a log line'))


if __name__ == '__main__':
    unittest.main()
