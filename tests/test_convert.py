
import hypothesis
from hypothesis import given, assume, strategies as st
import unittest

import tohil


class TestMethods(unittest.TestCase):
    @given(st.integers(-2000000, 2000000))
    def test_convert1(self, i):
        """exercise tohil.convert with no to= and with to=str"""
        assert(tohil.convert(i) == str(i))
        assert(tohil.convert(i, to=str) == str(i))
        assert(tohil.convert(str(i)) == str(i))

    @given(st.integers(-2000000, 2000000))
    def test_convert2(self, i):
        """exercise tohil.convert and to=int and to=float"""
        assert(tohil.convert(i, to=int) == i)
        assert(tohil.convert(i, to=float) == float(i))

    def test_convert3(self):
        """exercise tohil.convert to=bool"""
        assert(tohil.convert(True, to=bool) == True)
        assert(tohil.convert("t", to=bool) == True)
        assert(tohil.convert("1", to=bool) == True)
        assert(tohil.convert(1, to=bool) == True)
        assert(tohil.convert(False, to=bool) == False)
        assert(tohil.convert("f", to=bool) == False)
        assert(tohil.convert("0", to=bool) == False)
        assert(tohil.convert(0, to=bool) == False)

    @given(st.lists(st.integers(-1000000000, 1000000000)))
    def test_convert4(self, ilist):
        """exercise tohil.convert to=list"""
        assert(tohil.convert(ilist, to=list) == ilist)

    def test_convert5(self):
        """exercise tohil.convert and to=dict"""
        assert(
            tohil.convert("a 1 b 2 c 3 d 4", to=dict),
            {"a": "1", "b": "2", "c": "3", "d": "4"},
        )

    def test_convert6(self):
        """exercise tohil.convert and to=tuple"""
        assert(
            tohil.convert("a 1 b 2 c 3 d 4", to=tuple),
            ("a", "1", "b", "2", "c", "3", "d", "4"),
        )

    def test_convert7(self):
        """exercise tohil.convert and to=set"""
        assert(
            sorted(tohil.convert("1 2 3 4 5 6 6", to=set)),
            ["1", "2", "3", "4", "5", "6"],
        )

    def test_convert8(self):
        """exercise tohil.convert and to=tohil.tclobj"""
        assert(
            repr(tohil.convert("1 2 3", to=tohil.tclobj)), "<tohil.tclobj: '1 2 3'>"
        )


if __name__ == "__main__":
    unittest.main()
