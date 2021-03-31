

import unittest

import tohil

class TestEval(unittest.TestCase):
    def test_eval1(self):
        """exercise tohil.eval"""
        self.assertEqual(tohil.eval("return 10", to=int), 10)
        self.assertEqual(tohil.eval("expr {[clock seconds] > 1000000000}", to=bool), True)


if __name__ == "__main__":
    unittest.main()
