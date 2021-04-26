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
        """exercise tohil.tclobj remainder math ops"""
        t = tohil.tclobj(9)

        self.assertEqual(t % 7, 2)
        self.assertEqual(11 % t, 2)
        self.assertEqual(-7 % t, 2)

        self.assertEqual(t % -7., -5)
        self.assertEqual(-7 % 9, 2)
        self.assertEqual(t % t, 0)
        self.assertEqual(-6. % t, 3)



if __name__ == "__main__":
    unittest.main()
