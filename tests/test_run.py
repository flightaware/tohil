

import unittest

import tohil

class TestRun(unittest.TestCase):
    def test_run1(self):
        """exercise tohil.run"""
        self.assertEqual(tohil.run("print('icky')"), "icky")

if __name__ == "__main__":
    unittest.main()
