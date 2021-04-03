

import unittest

import tohil

class TestRun(unittest.TestCase):
    def test_run1(self):
        """exercise tohil.run, tohil.exec equivalent that returns stdout
        emitted while the exec is executing"""
        self.assertEqual(tohil.run("print('icky')"), "icky")

    def test_run2(self):
        """test run with an error"""
        with self.assertRaises(SyntaxError):
            tohil.run("cant run this")

if __name__ == "__main__":
    unittest.main()
