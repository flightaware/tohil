import unittest

import tohil

def d4(depth: int, width: int, in_list: list = list()):
    """ return a list of lists of tcl subinterpreter names for the
    specified depth and width.  note that to make an "x0 x0 x0" one
    must have made "x0 x0" a 3 x 3 makes 39 subinterpreters; a
    4x4 makes 390. the ordering will be correct that no tcl child
    interpreter will be created without all needed parents having
    been created first"""
    depth -= 1
    new = list()
    for w in range(width):
        piece = in_list.copy()
        piece.append('x' + str(w))
        new.append(piece)
        if depth > 0:
            new.extend(d4(depth, width, in_list = piece))
    return new


def make_a_bunch(depth: int, width: int):
    for interp in d4(depth, width):
        #print(f"creating {interp}")
        tohil.call('interp', 'create', interp)
        tohil.call('interp', 'eval', interp, 'package require tohil')


class TestSubinterps(unittest.TestCase):
    def test_subinterp1(self):
        """exercise loading tohil into a child interpreter, and make
        sure no child interpreters are left over"""
        interp = tohil.eval("interp create")
        tohil.call(interp, 'eval', 'package require tohil')
        self.assertEqual(tohil.call('interp', 'delete', interp), '')
        self.assertEqual(tohil.call('interp', 'slaves'), '')

    def test_subinterp2(self):
        """exercise loading tohil into a child and grandchild interpreter
        and delete the grandchild first then the child"""
        make_a_bunch(2, 1)
        self.assertEqual(tohil.call('interp', 'delete', 'x0 x0'), '')
        self.assertEqual(tohil.call('interp', 'delete', 'x0'), '')
        self.assertEqual(tohil.call('interp', 'slaves'), '')

    def test_subinterp3(self):
        """exercise loading tohil into a child and grandchild interpreter
        and delete the child, implicitly deleting the grandchild, and
        make sure no child interpreters are left over"""
        make_a_bunch(2, 1)
        self.assertEqual(tohil.call('interp', 'delete', 'x0'), '')
        self.assertEqual(tohil.call('interp', 'slaves'), '')

    def test_subinterp4(self):
        """the goodie, make 39 nested tcl interpreters, each
        loading tohil, then delete various ones while expecting
        to get all their children too and end up with no child
        interpreters extant
        """
        make_a_bunch(3, 3)
        #print(f'''"{tohil.eval('interp slaves')}"''')
        #print("deleting x2 x0 x1")
        tohil.call('interp', 'delete', ['x2', 'x0', 'x1'])
        #print("deleting x2 x1")
        tohil.call('interp', 'delete', ['x2', 'x1'])
        #print("deleting x2")
        tohil.call('interp', 'delete', ['x2'])
        #print("deleting x2 x1")
        tohil.call('interp', 'delete', ['x1', 'x1'])
        #print("deleting x1")
        tohil.call('interp', 'delete', ['x1'])
        #print("deleting x0")
        tohil.call('interp', 'delete', ['x0'])
        #print('done')
        self.assertEqual(tohil.call('interp', 'slaves'), '')


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


if __name__ == "__main__":
    unittest.main()
