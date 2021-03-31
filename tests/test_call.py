

import unittest

import tohil

class TestCall(unittest.TestCase):
    def test_call1(self):
        """exercise tohil.call"""
        self.assertEqual(tohil.call("expr", "5 + 5", to=int), 10)



if __name__ == "__main__":
    unittest.main()
