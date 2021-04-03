

import unittest

import tohil

class TestTD(unittest.TestCase):
    def test_td1(self):
        """tohil.tclobj td_get """
        x = tohil.eval("list a 1 b 2 c 3", to=tohil.tclobj)
        self.assertEqual(x.td_get('a'), '1')
        self.assertEqual(x.td_get('a',to=int), 1)

    def test_td2(self):
        """tohil.tclobj td_remove """
        x = tohil.eval("list a 1 b 2 c 3", to=tohil.tclobj)
        self.assertEqual(x.td_get('b'), '2')
        self.assertEqual(x.td_get('b', to=int), 2)
        x.td_remove('c')
        self.assertEqual(repr(x), "<tohil.tclobj: 'a 1 b 2'>")

    def test_td3(self):
        """tohil.tclobj td_set """
        x = tohil.tclobj()
        x.td_set('foo','bar')
        x.td_set('hey','you')
        self.assertEqual(x.td_get('foo'), 'bar')
        self.assertEqual(repr(x), "<tohil.tclobj: 'foo bar hey you'>")
        x.td_remove('foo')
        self.assertEqual(repr(x), "<tohil.tclobj: 'hey you'>")

if __name__ == "__main__":
    unittest.main()
