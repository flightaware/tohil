import unittest

import tohil


class TestTrampoline(unittest.TestCase):
    def setUp(self):
        tohil.eval("""proc ab_test {a {b b_default}} {return "a is '$a', b is '$b'"}""")
        tohil.eval(
            """proc abc_test {a {b b_default} {c c_default}} {return "a is '$a', b is '$b', c is '$c'"}"""
        )


    def test_trampoline1(self):
        """create test proc and try different default values"""
        ab_test = tohil.TclProc("ab_test")

        self.assertEqual(ab_test("a_val"), "a is 'a_val', b is 'b_default'")
        self.assertEqual(ab_test("a_val", "b_val"), "a is 'a_val', b is 'b_val'")
        self.assertEqual(ab_test("a_val2", b="b_val2"), "a is 'a_val2', b is 'b_val2'")
        self.assertEqual(
            ab_test(b="b_val3", a="a_val3"), "a is 'a_val3', b is 'b_val3'"
        )

    def test_trampoline2(self):
        """try test proc with incorrect arguments"""

        ab_test = tohil.TclProc("ab_test")

        with self.assertRaises(TypeError):
            ab_test()

        with self.assertRaises(TypeError):
            ab_test("a", "b", "c")

        with self.assertRaises(TypeError):
            ab_test("a", c="c")

    def test_trampoline3(self):
        """different things with a 3-argument test proc"""
        abc_test = tohil.TclProc("abc_test")

        self.assertEqual(
            abc_test("a_val"), "a is 'a_val', b is 'b_default', c is 'c_default'"
        )
        self.assertEqual(
            abc_test("a_val", "b_val"), "a is 'a_val', b is 'b_val', c is 'c_default'"
        )
        self.assertEqual(
            abc_test("a_val2", b="b_val2"),
            "a is 'a_val2', b is 'b_val2', c is 'c_default'",
        )
        self.assertEqual(
            abc_test(b="b_val3", a="a_val3"),
            "a is 'a_val3', b is 'b_val3', c is 'c_default'",
        )
        self.assertEqual(
            abc_test(b="b_val4", a="a_val4", c="c_val4"),
            "a is 'a_val4', b is 'b_val4', c is 'c_val4'",
        )
        self.assertEqual(
            abc_test(c="c_val5", a="a_val5"),
            "a is 'a_val5', b is 'b_default', c is 'c_val5'",
        )
        self.assertEqual(
            abc_test("a_val6", c="c_val6"),
            "a is 'a_val6', b is 'b_default', c is 'c_val6'",
        )
        self.assertEqual(
            abc_test(c="c_val8", a="a_val8"),
            "a is 'a_val8', b is 'b_default', c is 'c_val8'",
        )

    def test_trampoline4(self):
        """raising exceptions on misuse"""
        # with self.assertRaises(SyntaxError):
        #    abc_test(c="c_val4", a="a_val4", c="c_val4")

        abc_test = tohil.TclProc("abc_test")

        with self.assertRaises(TypeError):
            abc_test(c="c_val4", b="b_val4")

        with self.assertRaises(TypeError):
            abc_test(b="b_val7")

    def test_trampoline5(self):
        """calling tcl procs that use the 'args' special behavior"""
        tohil.eval(
            """proc arg_check_ab {a {b default_b} args} {
            return [list $a $b $args]
        }"""
        )
        arg_check_ab = tohil.TclProc("arg_check_ab")
        self.assertEqual(
            arg_check_ab("c_val4", b="b_val4", a="a_val4"),
            "a_val4 b_val4 c_val4",
        )
        self.assertEqual(
            arg_check_ab("c_val5", a="a_val5"),
            "a_val5 c_val5 {}",
        )

    def test_trampoline6(self):
        """confirm TclProc against a C function produces something
        and doesn't break"""
        t = tohil.TclProc("lindex")
        self.assertEqual(
            repr(t),
            "<class 'TclProc' 'lindex', c-function>",
        )

    def test_trampoline7(self):
        """confirm it's an error to create TclProc where the corresponding
        Tcl proc or C function doesn't exist"""
        with self.assertRaises(NameError):
            t = tohil.TclProc("shenanigans")


# add support for to =; be able to coerce output

if __name__ == "__main__":
    unittest.main()
