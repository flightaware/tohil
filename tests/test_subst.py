

import unittest

import tohil

class TestSubst(unittest.TestCase):
    def test_subst1(self):
        """exercise tohil.subst"""
        tohil.call("set", "name", "karl")
        self.assertEqual(tohil.subst("hello, $name"), "hello, karl")
        self.assertEqual(tohil.subst("hello, [return $name]"), "hello, karl")

if __name__ == "__main__":
    unittest.main()
