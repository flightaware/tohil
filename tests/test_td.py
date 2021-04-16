import unittest

import tohil

from tohil import tclobj, tcldict


class TestTD(unittest.TestCase):
    def test_td1(self):
        """tohil.tcldict get """
        x = tohil.eval("list a 1 b 2 c 3", to=tohil.tcldict)
        self.assertEqual(x["a"], "1")
        x.to = int
        self.assertEqual(x["a"], 1)
        with self.assertRaises(KeyError):
            x.get("z")
        x.to = str
        self.assertEqual(x.get("z", default="bar"), "bar")
        self.assertEqual(x.get("z", default="bar", to=list), ["bar"])
        with self.assertRaises(RuntimeError):
            x.get("z", default="bar", to=int)
        self.assertEqual(x.get("z", default="1", to=int), 1)

    def test_td2(self):
        """tohil.tcldict get and delete """
        x = tohil.eval("list a 1 b 2 c 3", to=tohil.tcldict)
        self.assertEqual(x["b"], "2")
        self.assertEqual(x.get("b", to=int), 2)
        del x["c"]
        self.assertEqual(str(x), 'a 1 b 2')

    def test_td3(self):
        """tohil.tcldict set """
        x = tohil.tcldict()
        x["foo"] = "bar"
        x["hey"] = "you"
        self.assertEqual(x["foo"], "bar")
        self.assertEqual(str(x), 'foo bar hey you')
        del x["foo"]
        self.assertEqual(str(x), 'hey you')

    def test_td4(self):
        """tohil.tcldict set, get and remove """
        x = tcldict()
        x["foo"] = 5
        x["foo"] = 5
        self.assertEqual(x["foo"], "5")
        self.assertEqual(x.get("foo", to=int), 5)
        self.assertEqual(repr(x), "<tohil.tcldict: 'foo 5'>")
        del x["foo"]
        self.assertEqual(repr(x), "<tohil.tcldict: ''>")

    def test_td5(self):
        """tohil.tcldict list remove """
        x = tcldict()
        x["a"] = 1
        x["b"] = 2
        x["c"] = 3
        del x["a"]
        del x["c"]
        del x["c"]
        self.assertEqual(str(x), 'b 2')

    def test_td6(self):
        """tohil.tcldict td_set with list of keys"""
        x = tcldict()
        x[["a", "b", "c", "d"]] = "bar"
        self.assertEqual(repr(x), "<tohil.tcldict: 'a {b {c {d bar}}}'>")

    def test_td7(self):
        """tohil.tcldict access with list of keys"""
        x = tcldict()
        x[["a", "b", "c", "d"]] = "foo"
        x["b"] = "bar"
        self.assertEqual(x[["a", "b", "c", "d"]], "foo")

    def test_td8(self):
        """tohil.tcldict nested-in checks"""
        x = tcldict()
        x[["a", "b", "c"]] = "foo"
        x["b"] = "bar"
        self.assertEqual(x[["a", "b", "c"]], "foo")
        self.assertEqual(["a", "b", "c"] in x, True)
        self.assertEqual(["a", "d", "d"] in x, False)
        with self.assertRaises(TypeError):
            ["a", "b", "c", "d"] in x

    def test_td9(self):
        """tohil.tcldict access and manipulation of nested dictionaries"""
        t = tcldict()
        t[["a", "b", "c", "d"]] = 1
        t["b"] = "bar"
        self.assertEqual(t.get(["a", "b", "c", "d"]), "1")
        self.assertEqual(t.get(["a", "b"]), "c {d 1}")
        x = t.get(["a", "b"], to=tohil.tcldict)
        with self.assertRaises(KeyError):
            x["d"]
        self.assertEqual(x[["c", "d"]], "1")
        self.assertEqual(["c", "d"] in x, True)
        self.assertEqual(["c"] in x, True)
        self.assertEqual("c" in x, True)
        self.assertEqual("zzz" in x, False)
        self.assertEqual(["zzz"] in x, False)
        self.assertEqual(["zzz","zzz2"] in x, False)


if __name__ == "__main__":
    unittest.main()
