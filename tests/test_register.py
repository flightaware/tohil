import time
import unittest

import tohil

class TestRegister(unittest.TestCase):
    def test_simple(self):
        def callback(arg):
            return arg
        tohil.register_callback("pycallback", callback)
        self.assertEqual(tohil.call("pycallback", 1), tohil.tclobj(1))

    def test_var_args(self):
        def callback(*args):
            return args
        tohil.register_callback("pycallback", callback)
        self.assertEqual(tohil.call("pycallback", 1, 2, 3), ("1", "2", "3"))

    def test_kw_args(self):
        # tohil.call ignores kwargs and so they'll never make it back to the
        # callback
        def callback(*args, **kwargs):
            return args, kwargs
        tohil.register_callback("pycallback", callback)
        self.assertEqual(tohil.call("pycallback", 1, akwarg=2), ((1,), {}))

    @unittest.skip("segfaults")
    def test_missing_req_arg(self):
        # TODO: This segfaults when it should produce a reasonable error
        def callback(required):
            return required
        tohil.register_callback("pycallback", callback)
        self.assertEqual(tohil.call("pycallback"), "")

    def test_after_func(self):
        flag = False
        def setflag():
            nonlocal flag
            flag = True
        tohil.register_callback("setflag", setflag)
        tohil.call("after", 100, "setflag")
        while not flag:
            tohil.call("update")
            tohil.call("update", "idletasks")
            time.sleep(.05)
        assert flag

