

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

    def test_tclobj11(self):
        """exercise tohil.tclobj lappend()"""
        x = tohil.eval("list 1 2 3 4 5", to=tohil.tclobj)
        x.lappend('6')
        x.lappend(7)
        self.assertEqual(str(x), '1 2 3 4 5 6 7')

    def test_tclobj12(self):
        """exercise tohil.tclobj as_byte_array()"""
        x = tohil.eval("list 1 2 3 4 5", to=tohil.tclobj)
        x.lappend('6')
        x.lappend(7)
        self.assertEqual(x.as_byte_array(), bytearray(b'1 2 3 4 5 6 7'))

    def test_tclobj13(self):
        """tohil.tclobj slice stuff, 4 to the end"""
        x = tohil.eval("list 1 2 3 4 5 6 7", to=tohil.tclobj)
        self.assertEqual(repr(x[4:]),"[<tohil.tclobj: '5'>, <tohil.tclobj: '6'>, <tohil.tclobj: '7'>]");

    def test_tclobj14(self):
        """tohil.tclobj slice stuff, the beginning until 4"""
        x = tohil.eval("list 1 2 3 4 5 6 7", to=tohil.tclobj)
        self.assertEqual(repr(x[:4]),"[<tohil.tclobj: '1'>, <tohil.tclobj: '2'>, <tohil.tclobj: '3'>, <tohil.tclobj: '4'>]");

    def test_tclobj15(self):
        """tohil.tclobj slice stuff, from the beginning to 4 from the end"""
        x = tohil.eval("list 1 2 3 4 5 6 7", to=tohil.tclobj)
        self.assertEqual(repr(x[:-4]),"[<tohil.tclobj: '1'>, <tohil.tclobj: '2'>, <tohil.tclobj: '3'>]");

    def test_tclobj16(self):
        """tohil.tclobj slice stuff, from 4 from the end to the end"""
        x = tohil.eval("list 1 2 3 4 5 6 7", to=tohil.tclobj)
        self.assertEqual(repr(x[-4:]),"[<tohil.tclobj: '4'>, <tohil.tclobj: '5'>, <tohil.tclobj: '6'>, <tohil.tclobj: '7'>]");

    def test_tclobj17(self):
        """tohil.tclobj slice stuff, the whole thing with a :"""
        x = tohil.eval("list 1 2 3 4 5 6 7", to=tohil.tclobj)
        self.assertEqual(repr(x[:]),"[<tohil.tclobj: '1'>, <tohil.tclobj: '2'>, <tohil.tclobj: '3'>, <tohil.tclobj: '4'>, <tohil.tclobj: '5'>, <tohil.tclobj: '6'>, <tohil.tclobj: '7'>]");

    def test_tclobj18(self):
        """tohil.tclobj comparing tclobjs with stuff"""
        x = tohil.eval("list 1 2 3 4 5 6 7", to=tohil.tclobj)
        self.assertTrue(x[2] == x[2])
        self.assertTrue(x[2] == 3)
        self.assertTrue(x[2] == '3')
        self.assertTrue(x[2] < 4.0)
        self.assertFalse(x[2] > 4.0)

    def test_tclobj19(self):
        """tohil.tclobj lappend_list"""
        x = tohil.eval("list 1 2 3 4 5 6", to=tohil.tclobj)
        y = tohil.eval("list 7 8 9", to=tohil.tclobj)
        x.lappend(y)
        self.assertEqual(repr(x), "<tohil.tclobj: '1 2 3 4 5 6 {7 8 9}'>")
        x = tohil.eval("list 1 2 3 4 5 6", to=tohil.tclobj)
        x.lappend_list(y)
        self.assertEqual(repr(x), "<tohil.tclobj: '1 2 3 4 5 6 7 8 9'>")
        l = [10, 11, 12, 13]
        x.lappend_list(l)
        self.assertEqual(repr(x), "<tohil.tclobj: '1 2 3 4 5 6 7 8 9 10 11 12 13'>")

    def test_tclobj20(self):
        """tohil.incr tests"""
        t = tohil.tclobj(0)
        self.assertEqual(t.incr(), 1)
        self.assertEqual(t.incr(), 2)
        self.assertEqual(t.incr(), 3)
        self.assertEqual(t.incr(), 4)
        self.assertEqual(t.incr(), 5)
        self.assertEqual(t.incr(2), 7)
        self.assertEqual(t.incr(-1), 6)
        self.assertEqual(t.incr(incr=-1), 5)
        with self.assertRaises(TypeError):
            t.set('foo')
            t.incr()

if __name__ == "__main__":
    unittest.main()


