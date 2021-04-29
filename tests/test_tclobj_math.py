
import hypothesis
from hypothesis import given, assume, strategies as st
import unittest

import tohil

class TestTclObj(unittest.TestCase):
    @given(st.integers(-40000, 40000), st.integers(-40000, 40000))
    def test_tclobj_math1(self, i, j):
        """exercise tohil.tclobj 'plus' math ops"""
        t = tohil.tclobj(i)

        assert(t + j == i + j)
        assert(t + t == i + i)
        assert(6 + t == 6 + i)

        assert(t + 4. == i + 4.)
        assert(4. + t == i + 4.)
        assert(t + float(t) == i + float(i))
        assert(6. + t == 6. + i)

    @given(st.integers(-40000, 40000), st.integers(-40000, 40000))
    def test_tclobj_math2(self, i, j):
        """exercise tohil.tclobj 'minus' math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        assert(ti - 3 == i - 3)
        assert(ti - ti == 0)
        assert(6 - ti == 6 - i)

        assert(tj - 4. == j - 4.)
        assert(tj - float(tj) == 0.)
        assert(6. - ti == 6 - i)

        assert(tj - ti == j - i)

    @given(st.floats(-40000.0, 40000.0), st.integers(-40000, 40000))
    def test_tclobj_math3(self, f, i):
        """exercise tohil.tclobj plus float math ops"""
        t = tohil.tclobj(f)

        assert(t == f)
        assert(t - i == f - i)
        assert(t - t == 0.0)
        assert(6 - t == 6 - f)

        assert(i - t == i - f)
        assert(int(t) - i == int(t) - i)
        assert(6. - t == 6. - f)

    @given(st.integers(-40000.0, 40000.0), st.integers(-40000, 40000))
    def test_tclobj_math4(self, i, j):
        """exercise tohil.tclobj multiply math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        assert(ti * 3 == i * 3)
        assert(ti * j == i * j)
        assert(j * ti == i * j)
        assert(tj * ti == i * j)
        assert(7 * ti == 7 * i)
        assert(ti * 7 == 7 * i)
        assert(ti * ti == i * i)

        assert(ti * 4. == i * 4.)
        assert(ti * float(ti) == i * float(i))
        assert(8. * ti == 8. * i)

    @given(st.floats(-40000.0, 40000.0), st.floats(-40000, 40000))
    def test_tclobj_math5(self, u, v):
        """exercise tohil.tclobj multiply float math ops"""
        t6 = tohil.tclobj('6.')
        tu = tohil.tclobj(u)
        tv = tohil.tclobj(v)

        assert(tu * 6 == u * 6)
        assert(tu * 6. == u * 6.)
        assert(tu * t6== u * 6.)
        assert(tu * tu == u * u)

        assert(tu * tv == u * v)

    @given(st.integers(-40000, 40000), st.integers(-40000, 40000))
    def test_tclobj_math6(self, i, j):
        """exercise tohil.tclobj remainder ops"""
        assume(i != 0 and j != 0)

        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        assert(ti % 7 == i % 7)
        assert(ti % j == i % j)
        assert(11 % ti == 11 % i)
        assert(-7 % ti == -7 % i)
        assert(ti % -7 == i % -7)

        assert(ti % tj == i % j)
        assert(ti % j == i % j)
        assert(i % tj == i % j)

    @given(st.integers(0, 1024), st.integers(0, 23))
    def test_tclobj_math7(self, i, j):
        """exercise tohil.tclobj left shift math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        assert(ti << j == i << j)
        assert(ti << tj == i << j)
        assert(i << tj == i << j)

        assert(ti << 4 == i << 4)
        assert(4 << tj == 4 << j)

    @given(st.integers(0, 2000000000), st.integers(0, 23))
    def test_tclobj_math8(self, i, j):
        """exercise tohil.tclobj right shift math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        assert(ti >> j == i >> j)
        assert(ti >> tj == i >> j)
        assert(i >> tj == i >> j)

    @given(st.integers(0, 2000000000), st.integers(0, 2000000000))
    def test_tclobj_math9(self, i, j):
        """exercise tohil.tclobj "and" math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        assert(ti & j == i & j)
        assert(ti & tj == i & j)
        assert(i & tj == i & j)

    @given(st.integers(0, 2000000000), st.integers(0, 2000000000))
    def test_tclobj_math10(self, i j):
        """exercise tohil.tclobj "or" math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        assert(ti | j, i | j)
        assert(ti | tj, i | j)
        assert(i | tj, i | j)

    @given(st.integers(0, 2000000000), st.integers(0, 2000000000))
    def test_tclobj_math11(self, i j):
        """exercise tohil.tclobj "xor" math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        assert(ti ^ j == i ^ j)
        assert(ti ^ tj == i ^ j)
        assert(i ^ tj == i ^ j)

    @given(st.integers(0, 2000000000), st.integers(0, 2000000000))
    def test_tclobj_math12(self, i, j):
        """exercise tohil.tclobj "inplace add" math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        ti += j
        assert(ti == i + j)

        ti = tohil.tclobj(i)
        ti += ti
        assert(ti == i + i)

        ti = tohil.tclobj(i)
        ti += tj
        assert(ti == i + j)

    @given(st.integers(0, 2000000000), st.integers(0, 2000000000))
    def test_tclobj_math13(self):
        """exercise tohil.tclobj "inplace subtract" math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        ti -= j
        assert(ti == i - j)

        ti = tohil.tclobj(i)
        ti -= tj
        assert(ti == i - j)

    @given(st.integers(-40000, 40000), st.integers(-40000, 40000))
    def test_tclobj_math14(self, i j):
        """exercise tohil.tclobj "inplace multiply" math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        ti *= j
        assert(ti == i * j)

        ti = tohil.tclobj(i)
        ti *= tj
        assert(ti == i * j)


    def test_tclobj_math15(self):
        """exercise tohil.tclobj "inplace left shift" math ops"""
        t = tohil.tclobj(7)
        t5 = tohil.tclobj(2)

        t <<= 2
        assert(t, 28)
        t5 <<= t5
        assert(t5, 8)

    def test_tclobj_math16(self):
        """exercise tohil.tclobj "inplace right shift" math ops"""
        t = tohil.tclobj(32)
        t5 = tohil.tclobj(2)

        t >>= 1
        assert(t, 16)
        t >>= t5
        assert(t, 4)

    def test_tclobj_math16(self):
        """exercise tohil.tclobj "inplace bitwise or" math ops"""
        t = tohil.tclobj(16)
        t2 = tohil.tclobj(8)

        t |= 32
        assert(t, 48)
        t |= t2
        assert(t, 56)

    def test_tclobj_math17(self):
        """exercise tohil.tclobj "inplace bitwise and" math ops"""
        t = tohil.tclobj(31)
        t2 = tohil.tclobj(8)

        t &= 24
        assert(t, 24)
        t &= t2
        assert(t, 8)

    def test_tclobj_math17(self):
        """exercise tohil.tclobj "inplace bitwise xor" math ops"""
        t = tohil.tclobj(31)

        t ^= 15
        assert(t, 16)
        t ^= 17
        assert(t, 1)

    def test_tclobj_math18(self):
        """exercise tohil.tclobj "true divide" math ops"""
        t = tohil.tclobj(66)

        assert(t / 6, 11.0)
        assert(726 / t, 11.0)
        assert(t / t, 1.0)

    def test_tclobj_math19(self):
        """exercise tohil.tclobj "inplace true divide" math ops"""
        t = tohil.tclobj(66)

        t /= 2
        assert(t, 33.0)
        j = tohil.tclobj(3)
        t /= j
        assert(t, 11.0)
        j /= j
        assert(j, 1.0)

    def test_tclobj_math20(self):
        """exercise tohil.tclobj "floor divide" math ops"""
        t66 = tohil.tclobj(66)
        t10 = tohil.tclobj(10)
        tm7 = tohil.tclobj(-7)

        assert(t66 // t10, 66 // 10)
        assert(66 // t10, 66 // 10)
        assert(t66 // 10, 66 // 10)

        assert(t66 // tm7, 66 // -7)
        assert(66 // tm7, 66 // -7)
        assert(t66 // -7, 66 // -7)

        assert(tm7 // t10, -7 // 10)
        assert(-7 // t10, -7 // 10)
        assert(tm7 // 10, -7 // 10)

    def test_tclobj_math21(self):
        """exercise tohil.tclobj "inplace floor divide" math ops"""
        t66 = tohil.tclobj(66)
        t10 = tohil.tclobj(10)
        tm7 = tohil.tclobj(-7)
        r5 = tohil.tclobj(5)

        t = t66
        t //= 2
        assert(t, 66 // 2)
        t //= r5
        assert(t, 33 // 5)
        t = t66
        t //= tm7
        assert(t, 66 // -7)

    def test_tclobj_math22(self):
        """exercise tohil.tclobj inplace remainder ops"""
        t = tohil.tclobj(5)
        t %= 7
        assert(t, 5 % 7)

        t = tohil.tclobj(11)
        t %= 5
        assert(t, 11 % 5)

        t = tohil.tclobj(77)
        t %= -7
        assert(t, 77 % -7)

        t = tohil.tclobj(-77)
        t %= -7
        assert(t, -77 % -7)

        t = tohil.tclobj(-77)
        t %= 7
        assert(t, -77 % 7)

        t = tohil.tclobj(-77)
        tm7 = tohil.tclobj('-7.')
        t %= tm7
        assert(t, -77 % -7.)

        t = tohil.tclobj(-77.)
        tm7 = tohil.tclobj('-7')
        t %= tm7
        assert(t, -77. % -7)

        t = tohil.tclobj(-77.)
        tm7 = tohil.tclobj(-7.)
        t %= tm7
        assert(t, -77. % -7.)


if __name__ == "__main__":
    unittest.main()
