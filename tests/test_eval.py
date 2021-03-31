

import unittest

import tohil

class TestEval(unittest.TestCase):
    def test_eval1(self):
        """exercise tohil.eval with no to= and with to=str"""
        self.assertEqual(tohil.eval("return 10", to=str), "10")
        self.assertEqual(tohil.eval("return 10"), "10")

    def test_eval2(self):
        """exercise tohil.eval and to=int and to=float"""
        self.assertEqual(tohil.eval("return 10", to=int), 10)
        self.assertEqual(tohil.eval("return 10", to=float), 10)

    def test_eval3(self):
        """exercise tohil.eval to=bool and to=list"""
        self.assertEqual(tohil.eval("expr {[clock seconds] > 1000000000}", to=bool), True)
        self.assertEqual(tohil.eval("list 1 2 3 4 5", to=list), ['1', '2', '3', '4', '5'])

    def test_eval4(self):
        """exercise tohil.eval and to=dict"""
        self.assertEqual(tohil.eval("list a 1 b 2 c 3 d 4", to=dict), {'a': '1', 'b': '2', 'c': '3', 'd': '4'})

    def test_eval5(self):
        """exercise tohil.eval and to=tuple"""
        self.assertEqual(tohil.eval("list a 1 b 2 c 3 d 4", to=tuple), ('a', '1', 'b', '2', 'c', '3', 'd', '4'))


if __name__ == "__main__":
    unittest.main()
