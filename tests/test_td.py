import unittest

import tohil

from tohil import tclobj, tcldict


class TestTD(unittest.TestCase):
    def test_td1(self):
        """tohil.tclobj td_get """
        x = tohil.eval("list a 1 b 2 c 3", to=tohil.tclobj)
        self.assertEqual(x.td_get("a"), "1")
        self.assertEqual(x.td_get("a", to=int), 1)
        with self.assertRaises(KeyError):
            x.td_get("z")
        self.assertEqual(x.td_get("z", default="bar"), "bar")
        self.assertEqual(x.td_get("z", default="bar", to=list), ["bar"])
        with self.assertRaises(RuntimeError):
            x.td_get("z", default="bar", to=int)
        self.assertEqual(x.td_get("z", default="1", to=int), 1)

    def test_td2(self):
        """tohil.tcldict get and delete """
        x = tohil.eval("list a 1 b 2 c 3", to=tohil.tcldict)
        self.assertEqual(x["b"], "2")
        self.assertEqual(x.get("b", to=int), 2)
        del x["c"]
        self.assertEqual(str(x), 'a 1 b 2')

    def test_td3(self):
        """tohil.tclobj td_set """
        x = tohil.tcldict()
        x["foo"] = "bar"
        x["hey"] = "you"
        self.assertEqual(x["foo"], "bar")
        self.assertEqual(str(x), 'foo bar hey you')
        del x["foo"]
        self.assertEqual(str(x), 'hey you')

    def test_td4(self):
        """tohil.tclobj td_set, get and remove """
        x = tclobj()
        x["foo"] = 5
        x["foo"] = 5
        self.assertEqual(x["foo"], "5")
        self.assertEqual(x.get("foo", to=int), 5)
        self.assertEqual(repr(x), "<tohil.tcldict: 'foo 5'>")
        x.td_remove("foo")
        self.assertEqual(repr(x), "<tohil.tcldict: ''>")

    def test_td4(self):
        """tohil.tclobj list remove """
        x = tcldict()
        x.td_set("a", 1)
        x.td_set("b", 2)
        x.td_set("c", 3)
        del x["a"]
        del x["c"]
        del x["c"]
        self.assertEqual(str(x), 'b 2')

    def test_td5(self):
        """tohil.tclobj td_set with list of keys"""
        x = tclobj()
        x.td_set(["a", "b", "c", "d"], "bar")
        self.assertEqual(repr(x), "<tohil.tclobj: 'a {b {c {d bar}}}'>")

    def test_td6(self):
        """tohil.tclobj td_get with list of keys"""
        x = tclobj()
        x.td_set(["a", "b", "c", "d"], "foo")
        x.td_set("b", "bar")
        self.assertEqual(x.td_get(["a", "b", "c", "d"]), "foo")

    def test_td7(self):
        """tohil.tclobj td_exists"""
        x = tclobj()
        x.td_set(["a", "b", "c"], "foo")
        x.td_set("b", "bar")
        self.assertEqual(x.td_get(["a", "b", "c"]), "foo")
        self.assertEqual(x.td_exists(["a", "b", "c"]), True)
        self.assertEqual(x.td_exists(["a", "d", "d"]), False)
        x.set("monkey")
        with self.assertRaises(TypeError):
            x.td_exists(["a", "b", "c"])

    def test_td8(self):
        """tohil.tclobj td_get of nested dictionaries"""
        t = tclobj()
        t.td_set(["a", "b", "c", "d"], 1)
        t.td_set("b", "bar")
        self.assertEqual(t.td_get(["a", "b", "c", "d"]), "1")
        self.assertEqual(t.td_get(["a", "b"]), "c {d 1}")
        x = t.td_get(["a", "b"], to=tohil.tclobj)
        with self.assertRaises(KeyError):
            x.td_get("d")
        self.assertEqual(x.td_get(["c", "d"]), "1")
        self.assertEqual(x.td_exists(["c", "d"]), True)


if __name__ == "__main__":
    unittest.main()
