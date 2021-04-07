import unittest

import tohil

from tohil import tclobj


class Test_td_iter(unittest.TestCase):
    def test_td_iter1(self):
        """tohil.tclobj td_iter """
        t = tohil.tclobj("a 1 b 2 c 3 d 4 e 5 f 6")
        self.assertEqual(list(t.td_iter()), ["a", "b", "c", "d", "e", "f"])

    def test_td_iter2(self):
        """tohil.tclobj td_iter with to= conversion """
        t = tohil.tclobj("a 1 b 2 c 3 d 4 e 5 f 6")
        self.assertEqual(
            list(t.td_iter(to=int)),
            [("a", 1), ("b", 2), ("c", 3), ("d", 4), ("e", 5), ("f", 6)],
        )
        self.assertEqual(
            list(t.td_iter(to=str)),
            [("a", "1"), ("b", "2"), ("c", "3"), ("d", "4"), ("e", "5"), ("f", "6")],
        )


if __name__ == "__main__":
    unittest.main()
