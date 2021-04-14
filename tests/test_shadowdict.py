import unittest

import tohil


class TestShadowDicts(unittest.TestCase):
    def test_shadowdict1(self):
        """create and access shadow dict as int and compare"""
        tohil.eval("array set x [list a 1 b 2 c 3 d 4]")
        x = tohil.ShadowDict("x", to=int)
        self.assertEqual(x["a"], 1)

    def test_shadowdict2(self):
        """access shadow dict as string and compare"""
        x = tohil.ShadowDict("x", to=str)
        self.assertEqual(x["d"], "4")

    def test_shadowdict3(self):
        """make a new element and read it"""
        x = tohil.ShadowDict("x", to=int)
        x["e"] = 5
        self.assertEqual(x["e"], 5)

    def test_shadowdict4(self):
        """length of shadow dict"""
        x = tohil.ShadowDict("x", to=int)
        self.assertEqual(len(x), 5)

    def test_shadowdict5(self):
        """delete element from shadow dict and length"""
        x = tohil.ShadowDict("x", to=int)
        del x["d"]
        self.assertEqual(len(x), 4)


if __name__ == "__main__":
    unittest.main()
