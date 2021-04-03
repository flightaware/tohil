

import unittest

import tohil

class TestTclObj(unittest.TestCase):
    def test_tclobj1(self):
        """exercise tohil.tclobj data type"""
        x = tohil.eval("list 1 2 3 4 5", to=tohil.tclobj)

        self.assertEqual('3' in x, True)
        self.assertEqual('6' in x, False)

    def test_tclobj2(self):
        """exercise tohil.tclobj lindex method"""
        x = tohil.eval("list 1 2 3 4 5", to=tohil.tclobj)

        self.assertEqual(x.lindex(0), '1')
        self.assertEqual(x.llength(), 5)

        with self.assertRaises(IndexError):
            x.lindex(-1)

        with self.assertRaises(IndexError):
            x.lindex(6)

    def test_tclobj3(self):
        """exercise tohil.tclobj str()"""
        x = tohil.eval("list 1 2 3 4 5", to=tohil.tclobj)

        self.assertEqual(str(x), '1 2 3 4 5')

    def test_tclobj4(self):
        """exercise tohil.tclobj as_list()"""
        x = tohil.eval("list 1 2 3 4 5", to=tohil.tclobj)

        self.assertEqual(x.as_list(), ['1', '2', '3', '4', '5'])

    def test_tclobj5(self):
        """exercise tohil.tclobj as_set()"""
        x = tohil.eval("list 1 2 3 4 5 5", to=tohil.tclobj)

        self.assertEqual(sorted(x.as_set()), ['1', '2', '3', '4', '5'])

    def test_tclobj6(self):
        """exercise tohil.tclobj as_tuple()"""
        x = tohil.eval("list 1 2 3 4 5 5", to=tohil.tclobj)

        self.assertEqual(sorted(x.as_set()), ['1', '2', '3', '4', '5'])

    def test_tclobj7(self):
        """exercise tohil.tclobj as_tuple()"""
        x = tohil.expr("5", to=tohil.tclobj)

        self.assertEqual(x.as_int(), 5)
        self.assertEqual(x.as_float(), 5.0)
        self.assertEqual(x.as_bool(), True)

    def test_tclobj8(self):
        """exercise tohil.tclobj setvar()"""
        x = tohil.eval("list 1 2 3 4 5", to=tohil.tclobj)
        x.setvar("foo")
        self.assertEqual(str(x), tohil.getvar("foo"))

    def test_tclobj9(self):
        """exercise tohil.tclobj reset()"""
        x = tohil.eval("list 1 2 3 4 5", to=tohil.tclobj)
        self.assertEqual(str(x), "1 2 3 4 5")
        x.reset()
        self.assertEqual(str(x), '')

    def test_tclobj10(self):
        """exercise tohil.tclobj subscripting, str(), and repr()"""
        x = tohil.eval("list 1 2 3 4 5", to=tohil.tclobj)
        self.assertEqual(str(x[2]), '3')
        self.assertEqual(repr(x[2]), "<tohil.tclobj: '3'>")


if __name__ == "__main__":
    unittest.main()
