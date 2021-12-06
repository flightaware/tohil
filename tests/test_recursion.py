import unittest

import tohil

class TestRecursion(unittest.TestCase):
    def test_with_callback(self):
        # Can't do this without a callback due to how unittest imports the tests.
        # Results in the global environment of this module not being visible from within Tcl.
        tohil.eval("""
        package require tohil
        proc isodd {num} {
            return [expr {![iseven $num]}]
        }
        """)
        def iseven(num):
            num = int(num)
            if num == 1:
                return False
            elif num == 0:
                return True
            else:
                return not tohil.call("isodd", num - 2, to=bool)
        tohil.register_callback("iseven", iseven)
        self.assertTrue(iseven(0))
        self.assertTrue(iseven(2))
        self.assertTrue(iseven(4))
        self.assertTrue(iseven(6))
        self.assertTrue(iseven(200))

        self.assertFalse(iseven(1))
        self.assertFalse(iseven(3))
        self.assertFalse(iseven(99))
