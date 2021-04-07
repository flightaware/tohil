import unittest

import tohil


def no_arg_kw(**kwlist):
    return str(kwlist)


def one_arg_kw(arg1, **kwlist):
    return str(arg1), str(kwlist)


def two_arg_kw(arg1, arg2, **kwlist):
    return str(arg1), str(arg2), str(kwlist)


class TestCall(unittest.TestCase):
    def test_call1(self):
        """exercise tohil.call"""
        self.assertEqual(tohil.call("expr", "5 + 5", to=int), 10)
        self.assertEqual(
            tohil.call("expr", "[clock seconds] > 1000000000", to=bool), True
        )

    def test_call2(self):
        """make sure tohil.call doesn't expand its arguments"""
        self.assertEqual(tohil.call("return", "[info globals]"), "[info globals]")
        self.assertEqual(tohil.call("return", "$secretVariable"), "$secretVariable")

        with self.assertRaises(RuntimeError):
            tohil.call("better_not_be_this_asdfjhk")

    def test_call3(self):
        """test with some arguments; compare to eval's output"""
        self.assertEqual(
            tohil.call("clock", "format", 1, "-gmt", 1, "-locale", "fr_FR"),
            "jeu. janv. 01 00:00:01 GMT 1970",
        )
        self.assertEqual(
            tohil.call("clock", "format", 1, "-gmt", 1, "-locale", "fr_FR"),
            tohil.eval("clock format 1 -gmt 1 -locale fr_FR"),
        )

    def test_call4(self):
        """test what happens when we cause an error on the tcl side"""
        with self.assertRaises(RuntimeError):
            tohil.call("cant", "run", "this")

    def test_call5(self):
        """test """
        self.assertEqual(no_arg_kw(foo="bar"), "{'foo': 'bar'}")
        with self.assertRaises(RuntimeError):
            tohil.call("")

    def test_call6(self):
        with self.assertRaises(TypeError):
            no_arg_kw(5, foo="bar")

    def test_call7(self):
        self.assertEqual(one_arg_kw(42, foo="bar"), ("42", "{'foo': 'bar'}"))

    def test_call8(self):
        self.assertEqual(two_arg_kw(42, 77, foo="bar"), ("42", "77", "{'foo': 'bar'}"))


if __name__ == "__main__":
    unittest.main()
