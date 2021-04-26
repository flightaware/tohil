import unittest

import tohil


class TestTclObj(unittest.TestCase):
    def test_tclobj_math1(self):
        """exercise tohil.tclobj plus math ops"""
        t = tohil.tclobj(5)

        self.assertEqual(t + 4, 9)
        self.assertEqual(t + t, 10)
        self.assertEqual(6 + t, 11)

        self.assertEqual(t + 4., 9.)
        self.assertEqual(t + float(t), 10.)
        self.assertEqual(6. + t, 11.)

    def test_tclobj_math2(self):
        """exercise tohil.tclobj plus math ops"""
        t = tohil.tclobj('5')

        self.assertEqual(t - 3, 2)
        self.assertEqual(t - t, 0)
        self.assertEqual(6 - t, 1)

        self.assertEqual(t - 4., 1.)
        self.assertEqual(t - float(t), 0.)
        self.assertEqual(6. - t, 1.)

    def test_tclobj_math3(self):
        """exercise tohil.tclobj plus float math ops"""
        t = tohil.tclobj('5.0')

        self.assertEqual(t - 3, 2.0)
        self.assertEqual(t - t, 0.0)
        self.assertEqual(6 - t, 1.0)

        self.assertEqual(t - 4., 1.)
        self.assertEqual(t - float(t), 0.)
        self.assertEqual(6. - t, 1.)

    def test_tclobj_math4(self):
        """exercise tohil.tclobj multiply math ops"""
        t = tohil.tclobj('6')

        self.assertEqual(t * 3, 18)
        self.assertEqual(t * t, 36)
        self.assertEqual(7 * t, 42)
        self.assertEqual(t * 7, 42)

        self.assertEqual(t * 4., 24.)
        self.assertEqual(t * float(t), 36.)
        self.assertEqual(t * t, 36.)
        self.assertEqual(8. * t, 48.)

    def test_tclobj_math5(self):
        """exercise tohil.tclobj multiply float math ops"""
        t = tohil.tclobj('6.')

        self.assertEqual(t * 3, 18.)
        self.assertEqual(t * t, 36.)
        self.assertEqual(7 * t, 42.)

        self.assertEqual(t * 4., 24.)
        self.assertEqual(t * t, 36.)
        self.assertEqual(-6. * t, -36.)

    def test_tclobj_math6(self):
        """exercise tohil.tclobj remainder ops"""
        t = tohil.tclobj(1)

        self.assertEqual(t % 7, 2)
        self.assertEqual(11 % t, 2)
        self.assertEqual(-7 % t, 2)

        self.assertEqual(t % -7., -5)
        self.assertEqual(-7 % 9, 2)
        self.assertEqual(t % t, 0)
        self.assertEqual(-6. % t, 3)

    def test_tclobj_math7(self):
        """exercise tohil.tclobj left shift math ops"""
        t = tohil.tclobj(2)

        self.assertEqual(t << 4, 32)
        self.assertEqual(t << t, 8)
        self.assertEqual(16 << 2, 64)

    def test_tclobj_math8(self):
        """exercise tohil.tclobj right shift math ops"""
        t = tohil.tclobj(8)

        self.assertEqual(t >> 1, 4)
        self.assertEqual(t >> t, 0)
        self.assertEqual(65536 >> t, 256)

    def test_tclobj_math9(self):
        """exercise tohil.tclobj "and" math ops"""
        t = tohil.tclobj(31)

        self.assertEqual(t & 16, 16)
        self.assertEqual(t & t, 31)
        self.assertEqual(8 & t, 8)

    def test_tclobj_math10(self):
        """exercise tohil.tclobj "or" math ops"""
        t = tohil.tclobj(16)

        self.assertEqual(t | 8, 24)
        self.assertEqual(4 | t, 20)
        self.assertEqual(t | t, 16)

    def test_tclobj_math11(self):
        """exercise tohil.tclobj "xor" math ops"""
        t = tohil.tclobj(31)

        self.assertEqual(t ^ 15, 16)
        self.assertEqual(15 ^ t, 16)
        self.assertEqual(t ^ t, 0)

    def test_tclobj_math12(self):
        """exercise tohil.tclobj "inplace add" math ops"""
        t = tohil.tclobj(7)
        t5 = tohil.tclobj(5)

        t += 5
        self.assertEqual(t, 12)
        t += t
        self.assertEqual(t, 24)
        t += t5
        self.assertEqual(t, 29)
        t5 += t
        self.assertEqual(t5, 34)

    def test_tclobj_math13(self):
        """exercise tohil.tclobj "inplace subtract" math ops"""
        t = tohil.tclobj(7)
        t5 = tohil.tclobj(5)

        t -= -1
        self.assertEqual(t, 8)
        t -= t
        self.assertEqual(t, 0)
        t -= t5
        self.assertEqual(t, -5)
        t5 -= t
        self.assertEqual(t5, 10)

    def test_tclobj_math14(self):
        """exercise tohil.tclobj "inplace multiply" math ops"""
        t = tohil.tclobj(7)
        t5 = tohil.tclobj(5)

        t *= -1
        self.assertEqual(t, -7)
        t *= t
        self.assertEqual(t, 49)
        t *= t5
        self.assertEqual(t, 245)
        t5 *= t
        self.assertEqual(t5, 1225)

    def test_tclobj_math15(self):
        """exercise tohil.tclobj "inplace left shift" math ops"""
        t = tohil.tclobj(7)
        t5 = tohil.tclobj(2)

        t <<= 2
        self.assertEqual(t, 28)
        t5 <<= t5
        self.assertEqual(t5, 8)

    def test_tclobj_math16(self):
        """exercise tohil.tclobj "inplace right shift" math ops"""
        t = tohil.tclobj(32)
        t5 = tohil.tclobj(2)

        t >>= 1
        self.assertEqual(t, 16)
        t >>= t5
        self.assertEqual(t, 4)

    def test_tclobj_math16(self):
        """exercise tohil.tclobj "inplace bitwise or" math ops"""
        t = tohil.tclobj(16)
        t2 = tohil.tclobj(8)

        t |= 32
        self.assertEqual(t, 48)
        t |= t2
        self.assertEqual(t, 56)

    def test_tclobj_math17(self):
        """exercise tohil.tclobj "inplace bitwise and" math ops"""
        t = tohil.tclobj(31)
        t2 = tohil.tclobj(8)

        t &= 24
        self.assertEqual(t, 24)
        t &= t2
        self.assertEqual(t, 8)

    def test_tclobj_math17(self):
        """exercise tohil.tclobj "inplace bitwise xor" math ops"""
        t = tohil.tclobj(31)

        t ^= 15
        self.assertEqual(t, 16)
        t ^= 17
        self.assertEqual(t, 1)

    def test_tclobj_math18(self):
        """exercise tohil.tclobj "true divide" math ops"""
        t = tohil.tclobj(66)

        self.assertEqual(t / 6, 11.0)
        self.assertEqual(726 / t, 11.0)
        self.assertEqual(t / t, 1.0)

    def test_tclobj_math19(self):
        """exercise tohil.tclobj "inplace true divide" math ops"""
        t = tohil.tclobj(66)

        t /= 2
        self.assertEqual(t, 33.0)
        j = tohil.tclobj(3)
        t /= j
        self.assertEqual(t, 11.0)
        j /= j
        self.assertEqual(j, 1.0)

if __name__ == "__main__":
    unittest.main()
