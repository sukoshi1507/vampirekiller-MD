import unittest,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from audit_room_state_z80 import scan
class ScanTests(unittest.TestCase):
    def test_writes_and_reads(self):
        data=bytearray(0x20000)
        data[16:19]=bytes([0x32,0x01,0xD0])
        data[0x2000+10:0x2000+14]=bytes([0xED,0x43,0x00,0xD0])
        data[0x4000+10:0x4000+13]=bytes([0x3A,0x01,0xD0])
        matches=scan(bytes(data))
        self.assertEqual(len(matches),3)
        self.assertEqual(sorted(x['candidate_access'] for x in matches),['read','write','write'])
    def test_no_false_certification(self):
        from audit_room_state_z80 import report
        data=bytearray(0x20000)
        data[10:13]=bytes([0x32,0x01,0xD0])
        out=report(bytes(data))
        self.assertTrue(all(x['status']=='raw_pattern_not_confirmed_instruction' for x in out['candidates']))
