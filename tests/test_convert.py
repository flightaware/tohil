import unittest

import tohil


class TestMethods(unittest.TestCase):
    def test_convert1(self):
        """exercise tohil.convert with no to= and with to=str"""
        self.assertEqual(tohil.convert(10), "10")
        self.assertEqual(tohil.convert(10, to=str), "10")
        self.assertEqual(tohil.convert("10"), "10")
        self.assertEqual(tohil.convert("10", to=str), "10")

    def test_convert2(self):
        """exercise tohil.convert and to=int and to=float"""
        self.assertEqual(tohil.convert("10", to=int), 10)
        self.assertEqual(tohil.convert("10", to=float), 10.0)

    def test_convert3(self):
        """exercise tohil.convert to=bool"""
        self.assertEqual(tohil.convert(True, to=bool), True)
        self.assertEqual(tohil.convert("t", to=bool), True)
        self.assertEqual(tohil.convert("1", to=bool), True)
        self.assertEqual(tohil.convert(1, to=bool), True)
        self.assertEqual(tohil.convert(False, to=bool), False)
        self.assertEqual(tohil.convert("f", to=bool), False)
        self.assertEqual(tohil.convert("0", to=bool), False)
        self.assertEqual(tohil.convert(0, to=bool), False)

    def test_convert4(self):
        """exercise tohil.convert to=list"""
        self.assertEqual(tohil.convert("1 2 3 4 5", to=list), ["1", "2", "3", "4", "5"])

    def test_convert5(self):
        """exercise tohil.convert and to=dict"""
        self.assertEqual(
            tohil.convert("a 1 b 2 c 3 d 4", to=dict),
            {"a": "1", "b": "2", "c": "3", "d": "4"},
        )

    def test_convert6(self):
        """exercise tohil.convert and to=tuple"""
        self.assertEqual(
            tohil.convert("a 1 b 2 c 3 d 4", to=tuple),
            ("a", "1", "b", "2", "c", "3", "d", "4"),
        )

    def test_convert7(self):
        """exercise tohil.convert and to=set"""
        self.assertEqual(
            sorted(tohil.convert("1 2 3 4 5 6 6", to=set)),
            ["1", "2", "3", "4", "5", "6"],
        )

    def test_convert8(self):
        """exercise tohil.convert and to=tohil.tclobj"""
        self.assertEqual(
            repr(tohil.convert("1 2 3", to=tohil.tclobj)), "<tohil.tclobj: '1 2 3'>"
        )


if __name__ == "__main__":
    unittest.main()
