import unittest

import tohil


class TestVars(unittest.TestCase):
    def test_getvar1(self):
        """exercise setvar and getvar of a variable"""
        tohil.setvar("z", "frammistan")
        self.assertEqual(tohil.getvar("z"), "frammistan")
        self.assertEqual(tohil.eval("info exists z", to=int), 1)
        self.assertEqual(tohil.eval("return $z"), "frammistan")

    def test_getvar2(self):
        """exercise getvar of an array element"""
        tohil.eval("array unset x; array set x [list a 1 b 2 c 3 d 4]")
        self.assertEqual(tohil.getvar("x(a)"), "1")
        self.assertEqual(tohil.getvar("x(c)", to=int), 3)
        self.assertEqual(tohil.getvar("x(d)", to=float), 4.0)

    def test_unset1(self):
        """unset of scalar"""
        tohil.unset()
        tohil.unset("z")
        tohil.unset("z", "z", "zz")
        self.assertEqual(tohil.eval("info exists z", to=int), 0)

    def test_unset2(self):
        """unset of array element"""
        self.assertEqual(tohil.eval("info exists x(d)", to=int), 1)
        self.assertEqual(tohil.exists("x(d)"), True)
        tohil.unset("x(c)", "x(d)")
        self.assertEqual(tohil.eval("info exists x(d)", to=int), 0)
        self.assertEqual(tohil.exists("x(d)"), False)

    def test_unset3(self):
        # make sure unset doesn't do anything if the var or element doesn't exist
        tohil.unset("x(d)")
        with self.assertRaises(NameError):
            tohil.getvar("x(d)")


if __name__ == "__main__":
    unittest.main()
