import unittest

import tohil

class TestSubinterpreters(unittest.TestCase):
    def test_subinterpreter_explicit_delete(self):
        subinterp = tohil.call("interp", "create", to=str)
        # This can often cause spooky behavior since it creates a python
        # subinterpreter which is then deleted when we delete the Tcl
        # subinterpreter
        tohil.call(subinterp, "eval", "package require tohil")
        self.assertEqual(len(tohil.call("interp", "slaves", to=list)), 1)
        tohil.call("interp", "delete", subinterp)
        self.assertEqual(tohil.call("interp", "slaves", to=list), [])

    def test_subinterpreter_gc_delete(self):
        class TohilInterp:
            deleted = False

            def __init__(self):
                self.interp = tohil.call("interp", "create")
            def __del__(self):
                type(self).deleted = True
                tohil.call("interp", "delete", self.interp)
            def call(self, command, *args):
                return tohil.call(self.interp, "eval", [command, *args])

        subinterp = TohilInterp()
        # This can often cause spooky behavior since it creates a python
        # subinterpreter which is then deleted when we delete the Tcl
        # subinterpreter
        subinterp.call("package", "require", "tohil")
        self.assertEqual(len(tohil.call("interp", "slaves", to=list)), 1)
        del subinterp
        self.assertEqual(tohil.call("interp", "slaves", to=list), [])
        self.assertTrue(TohilInterp.deleted)

    def test_subinterpreter_gc_delete_loop(self):
        class TohilInterp:
            deleted = False

            def __init__(self):
                self.interp = tohil.call("interp", "create")
            def __del__(self):
                type(self).deleted = True
                tohil.call("interp", "delete", self.interp)
            def call(self, command, *args):
                return tohil.call(self.interp, "eval", [command, *args])

        for i in range(5):
            TohilInterp.deleted = False
            subinterp = TohilInterp()
            subinterp.call("package", "require", "tohil")
            self.assertEqual(len(tohil.call("interp", "slaves", to=list)), 1)
            del subinterp
            self.assertEqual(tohil.call("interp", "slaves", to=list), [])
            self.assertTrue(TohilInterp.deleted)
