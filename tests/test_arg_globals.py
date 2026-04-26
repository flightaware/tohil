import importlib
import sys
import unittest

import tohil


class TestArgGlobals(unittest.TestCase):
    def setUp(self):
        tohil.eval("unset -nocomplain ::argv ::argv0 ::argc")

    def test_empty_argv(self):
        # This can occur naturally, but only before python3.8
        sys.argv = []
        importlib.reload(tohil)
        self.assertEqual(tohil.call("info", "exists", "::argv"), 0)
        self.assertEqual(tohil.call("info", "exists", "::argv0"), 0)
        self.assertEqual(tohil.call("info", "exists", "::argc"), 0)

    def test_argv_blank(self):
        # This is python's 3.8+ fallback behavior and can generally be equated
        # to "nothing was set"
        sys.argv = [""]
        importlib.reload(tohil)
        self.assertEqual(tohil.call("info", "exists", "::argv"), 0)
        self.assertEqual(tohil.call("info", "exists", "::argv0"), 0)
        self.assertEqual(tohil.call("info", "exists", "::argc"), 0)

    def test_argv_multiple(self):
        sys.argv = ["script.py", "arg1", "arg2"]
        importlib.reload(tohil)
        self.assertEqual(tohil.call("set", "::argv", to=list), sys.argv[1:])
        self.assertEqual(tohil.call("set", "::argv0"), "script.py")
        self.assertEqual(tohil.call("set", "::argc"), 2)
