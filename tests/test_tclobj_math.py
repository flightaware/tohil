
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
    def test_tclobj_math10(self, i, j):
        """exercise tohil.tclobj "or" math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        assert(ti | j == i | j)
        assert(ti | tj == i | j)
        assert(i | tj == i | j)

    @given(st.integers(0, 2000000000), st.integers(0, 2000000000))
    def test_tclobj_math11(self, i, j):
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
    def test_tclobj_math13(self, i, j):
        """exercise tohil.tclobj "inplace subtract" math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        ti -= j
        assert(ti == i - j)

        ti = tohil.tclobj(i)
        ti -= tj
        assert(ti == i - j)

    @given(st.integers(-40000, 40000), st.integers(-40000, 40000))
    def test_tclobj_math14(self, i, j):
        """exercise tohil.tclobj "inplace multiply" math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        ti *= j
        assert(ti == i * j)

        ti = tohil.tclobj(i)
        ti *= tj
        assert(ti == i * j)


    @given(st.integers(0, 1024), st.integers(0, 23))
    def test_tclobj_math15(self, i, j):
        """exercise tohil.tclobj "inplace left shift" math ops"""

        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        ti <<= 2
        assert(ti == i << 2)

        # shift left by some bits
        ti = tohil.tclobj(i)
        ti <<= j
        assert(ti == i << j)

        # shift left by some bits, the count provided by a tclobj
        ti = tohil.tclobj(i)
        ti <<= tj
        assert(ti == i << j)

    @given(st.integers(0, 2000000000), st.integers(0, 23))
    def test_tclobj_math16(self, i, j):
        """exercise tohil.tclobj right shift math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        ti >>= j
        assert(ti == i >> j)

        ti = tohil.tclobj(i)
        ti >>= tj
        assert(ti == i >> j)

    @given(st.integers(0, 2000000000), st.integers(0, 2000000000))
    def test_tclobj_math16(self, i, j):
        """exercise tohil.tclobj "inplace bitwise or" math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        ti |= j
        assert(ti == i | j)

        ti = tohil.tclobj(i)
        ti |= tj
        assert(ti == i | j)

    @given(st.integers(0, 2000000000), st.integers(0, 2000000000))
    def test_tclobj_math17(self, i, j):
        """exercise tohil.tclobj "inplace bitwise and" math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        ti &= j
        assert(ti == i & j)

        ti = tohil.tclobj(i)
        ti &= tj
        assert(ti == i & j)

    @given(st.integers(0, 2000000000), st.integers(0, 2000000000))
    def test_tclobj_math17(self, i, j):
        """exercise tohil.tclobj "inplace bitwise xor" math ops"""
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        ti ^= j
        assert(ti == i ^ j)

        ti = tohil.tclobj(i)
        ti ^= tj
        assert(ti == i ^ j)

    @given(st.integers(-2000000000, 2000000000), st.integers(-2000000000, 2000000000))
    def test_tclobj_math18(self, i, j):
        """exercise tohil.tclobj "true divide" math ops"""
        assume(j != 0)
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        assert(ti / j == i / j)
        assert(ti / tj == i / j)
        assert(i / tj == i / j)

    @given(st.integers(-2000000000, 2000000000), st.integers(-2000000000, 2000000000))
    def test_tclobj_math19(self, i, j):
        """exercise tohil.tclobj "inplace true divide" math ops"""
        assume(j != 0)
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        ti /= j
        assert(abs(ti - i / j) < 0.000000001)

        ti = tohil.tclobj(i)
        ti /= tj
        assert(abs(ti - i / j) < 0.000000001)

    @given(st.integers(-2000000000, 2000000000), st.integers(-2000000000, 2000000000))
    def test_tclobj_math20(self, i, j):
        """exercise tohil.tclobj "floor divide" math ops"""
        assume(j != 0)
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        assert(ti // j == i // j)
        assert(ti // tj == i // j)
        assert(i // tj == i // j)

    @given(st.integers(-2000000000, 2000000000), st.integers(-2000000000, 2000000000))
    def test_tclobj_math21(self, i, j):
        """exercise tohil.tclobj "inplace floor divide" integer math ops"""
        assume(j != 0)
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        ti //= j
        assert(ti == i // j)

        ti = tohil.tclobj(i)
        ti //= tj
        assert(ti == i // j)

    @given(st.floats(-2000000000, 2000000000), st.floats(-2000000000, 2000000000))
    def test_tclobj_math22(self, u, v):
        """exercise tohil.tclobj "inplace floor divide" float math ops"""
        assume(v != 0)
        tu = tohil.tclobj(u)
        tv = tohil.tclobj(v)

        tu //= v
        assert(tu == u // v)

        tu = tohil.tclobj(u)
        tu //= tv
        assert(tu == u // v)

    @given(st.integers(-2000000000, 2000000000), st.integers(-2000000000, 2000000000))
    def test_tclobj_math23(self, i, j):
        """exercise tohil.tclobj "inplace remainder" integer math ops"""
        assume(j != 0)
        ti = tohil.tclobj(i)
        tj = tohil.tclobj(j)

        ti %= j
        assert(ti == i % j)

        ti = tohil.tclobj(i)
        ti %= tj
        assert(ti == i % j)

    @given(st.floats(-2000000000, 2000000000), st.floats(-2000000000, 2000000000))
    def test_tclobj_math24(self, u, v):
        """exercise tohil.tclobj "inplace remainder" float math ops"""
        assume(v != 0)
        tu = tohil.tclobj(u)
        tv = tohil.tclobj(v)

        tu %= v
        assert(abs(tu - u % v) < 0.000001)

        tu = tohil.tclobj(u)
        tu %= tv
        assert(abs(tu - u % v) < 0.000001)

    @given(st.integers(-2000000000, 2000000000), st.floats(-2000000000, 2000000000))
    def test_tclobj_math25(self, i, v):
        """exercise tohil.tclobj "inplace remainder" mixed-type math ops"""
        assume(v != 0)
        ti = tohil.tclobj(i)
        tv = tohil.tclobj(v)

        ti %= v
        assert(abs(ti - i % v) < 0.000001)

        ti = tohil.tclobj(i)
        ti %= tv
        assert(abs(ti - i % v) < 0.000001)

        assume(i != 0)
        ti = tohil.tclobj(i)
        tv %= i
        assert(abs(tv - v % i) < 0.000001)


if __name__ == "__main__":
    unittest.main()
