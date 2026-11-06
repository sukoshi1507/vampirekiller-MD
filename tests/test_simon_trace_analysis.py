import csv
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from analyze_simon_trace import analyze

class TraceTest(unittest.TestCase):
    def test_segments_and_markers(self):
        with tempfile.TemporaryDirectory() as d:
            file=Path(d)/'test.csv'
            with file.open('w',newline='') as f:
                w=csv.writer(f); w.writerow(['tick','marker','lower_c42e','upper_c42f'])
                w.writerows([[1,'boot',0,0],[2,'boot',0,0],[3,'experimental_input_held',0,6],[4,'experimental_input_held',0,9],[5,'experimental_input_released',0,9]])
            result=analyze(file)
            self.assertEqual(result['sample_count'],5)
            self.assertEqual(result['segment_count'],4)
            self.assertEqual(result['markers']['boot']['samples'],2)
            self.assertEqual(result['markers']['experimental_input_held']['segments'],2)
    def test_missing_data_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            file=Path(d)/'test.csv'; file.write_text('tick,marker\n1,boot\n')
            with self.assertRaises(ValueError): analyze(file)
if __name__=='__main__':unittest.main()

class TraceQualityTest(unittest.TestCase):
    def test_no_false_attack_claim(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'t.csv'
            p.write_text('tick,marker,lower_c42e,upper_c42f\n1,boot,0,0\n2,boot,0,0\n')
            r=analyze(p)
            self.assertFalse(r['quality']['attack_confirmed'])
            self.assertFalse(r['quality']['gameplay_marker_seen'])
            self.assertTrue(r['quality']['warnings'])
    def test_missing_interrupts_flagged(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'t.csv'
            p.write_text('tick,marker,lower_c42e,upper_c42f\n1,gameplay_unclassified,0,0\n4,experimental_input_held,0,6\n')
            r=analyze(p)
            self.assertEqual(r['tick_gaps'],2)
            self.assertTrue(r['quality']['probe_marker_seen'])
    def test_invalid_index_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'t.csv'
            p.write_text('tick,marker,lower_c42e,upper_c42f\n1,boot,0,256\n')
            with self.assertRaises(ValueError): analyze(p)
