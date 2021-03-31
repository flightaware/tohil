

import unittest

import tohil

class TestCall(unittest.TestCase):
    def test_call1(self):
        """exercise tohil.call"""
        self.assertEqual(tohil.call("expr", "5 + 5", to=int), 10)
        self.assertEqual(tohil.call("expr", "[clock seconds] > 1000000000", to=bool), True)

    def test_call2(self):
        """make sure tohil.call doesn't expand its arguments"""
        self.assertEqual(tohil.call("return", "[info globals]"), "[info globals]")
        self.assertEqual(tohil.call("return", "$secretVariable"), "$secretVariable")

        with self.assertRaises(RuntimeError):
            tohil.call("better_not_be_this_asdfjhk")

    def test_call3(self):
        self.assertEqual(tohil.call("clock", "format", 1, "-gmt", 1, "-locale", "fr_FR"), "jeu. janv. 01 00:00:01 GMT 1970")
        self.assertEqual(tohil.call("clock", "format", 1, "-gmt", 1, "-locale", "fr_FR"), tohil.eval("clock format 1 -gmt 1 -locale fr_FR"))


if __name__ == "__main__":
    unittest.main()
