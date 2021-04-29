
import tohil

import hypothesis
from hypothesis import given, strategies as st
import unittest


class TclobjNumberTests(unittest.TestCase):
    @given(st.integers(-1000000000, 1000000000), st.integers(-1000000000, 1000000000))
    def test_tclobj_arithmetic1(self, x, y):
        """test tclobj +"""
        tx = tohil.tclobj(x)
        ty = tohil.tclobj(y)
        assert tx + y == x + y
        assert x + ty == x + y
        assert tx + ty == x + y
        assert tx - y == x - y
        assert x - ty == x - y
        assert tx - ty == x - y

    @given(st.integers(-1000000000, 1000000000), st.integers(-1000000000, 1000000000))
    def test_tclobj_arithmetic2(self, x, y):
        """test tclobj +="""
        tx = tohil.tclobj(x)
        ty = tohil.tclobj(y)
        tx += y
        assert tx == x + y
        tx.set(x)
        tx += ty
        assert tx == x + y


    @given(st.integers(-10000, 10000), st.integers(-10000, 10000))
    def test_tclobj_arithmetic3(self, x, y):
        tx = tohil.tclobj(x)
        ty = tohil.tclobj(y)
        tx *= y
        assert tx == x * y
        tx = tohil.tclobj(x)
        tx *= ty
        assert tx == x * ty

    @given(st.integers(-10000, 10000), st.integers(0, 16))
    def test_tclobj_arithmetic4(self, x, y):
        tx = tohil.tclobj(x)
        ty = tohil.tclobj(y)
        assert tx << y == x << y
        assert tx << ty == x << y
        assert x << ty == x << y

if __name__ == "__main__":
    unittest.main()
