import unittest

import tohil

from tohil import tclobj


class Test_td_iter(unittest.TestCase):
    def test_td_iter1(self):
        """tohil.tcldict iterator """
        t = tohil.tcldict("a 1 b 2 c 3 d 4 e 5 f 6")
        self.assertEqual(list(t), ["a", "b", "c", "d", "e", "f"])

    def test_td_iter2(self):
        """tohil.tcldict iter with to= conversion """
        t = tohil.tcldict("a 1 b 2 c 3 d 4 e 5 f 6")
        t.to=int
        self.assertEqual(
            list(t),
            [("a", 1), ("b", 2), ("c", 3), ("d", 4), ("e", 5), ("f", 6)],
        )
        t.to=str
        self.assertEqual(
            list(t),
            [("a", "1"), ("b", "2"), ("c", "3"), ("d", "4"), ("e", "5"), ("f", "6")],
        )


if __name__ == "__main__":
    unittest.main()
