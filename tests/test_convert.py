
import hypothesis
from hypothesis import given, assume, strategies as st
from string import printable
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

    @given(st.lists(st.text(printable)))
    def test_convert4(self, ilist):
        """exercise tohil.convert to=list"""
        assume(ilist != '\x00')
        assert(tohil.convert(ilist, to=list) == ilist)

    @given(st.dictionaries(st.text(printable), st.text(printable)))
    def test_convert5(self, idict):
        """exercise tohil.convert and to=dict"""
        assert(tohil.convert(idict, to=dict) == idict)

    @given(st.tuples(st.text(printable)))
    def test_convert6(self, ituple):
        """exercise tohil.convert and to=tuple"""
        assert(tohil.convert(ituple, to=tuple) == ituple)

    @given(st.sets(st.text(printable)))
    def test_convert7(self, iset):
        """exercise tohil.convert and to=set"""
        assume(not any('{' in v or '}' in v for v in iset))
        assert(sorted(tohil.convert(iset, to=set)) == sorted(iset))

    def test_convert8(self):
        """exercise tohil.convert and to=tohil.tclobj"""
        assert(
            repr(tohil.convert("1 2 3", to=tohil.tclobj)) == "<tohil.tclobj: '1 2 3'>"
        )


if __name__ == "__main__":
    unittest.main()
