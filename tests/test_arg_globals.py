import importlib
import sys
import unittest

import tohil


class TestArgGlobals(unittest.TestCase):
    def test_empty_argv(self):
        sys.argv = []
        importlib.reload(tohil)
        self.assertEqual(tohil.call("info", "exists", "::argv"), 0)
        self.assertEqual(tohil.call("info", "exists", "::argv0"), 0)
        self.assertEqual(tohil.call("info", "exists", "::argc"), 0)

    def test_argv_blank(self):
        sys.argv = [""]
        importlib.reload(tohil)
        self.assertEqual(tohil.call("set", "::argv", to=list), [])
        self.assertEqual(tohil.call("set", "::argv0"), "")
        self.assertEqual(tohil.call("set", "::argc"), 0)

    def test_argv_multiple(self):
        sys.argv = ["script.py", "arg1", "arg2"]
        importlib.reload(tohil)
        self.assertEqual(tohil.call("set", "::argv", to=list), sys.argv[1:])
        self.assertEqual(tohil.call("set", "::argv0"), "script.py")
        self.assertEqual(tohil.call("set", "::argc"), 2)
