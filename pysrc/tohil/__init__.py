""" tohil """

import sys

from tohil._tohil import (
    eval, expr, getvar, interp, setvar, subst, call, plug,
)

from tohil.trampoline import Trampoline

from tohil.tcller import TclWriter

class RivetControl:
    def __init__(self):
        self.activated = 0
        self.tclWriter = TclWriter()

    def activate(self):
        if self.activated:
            return

        sys.stdout = self.tclWriter
        self.activated = 1

global rivetControl
rivetControl = RivetControl()

def rivet():
    rivetControl.activate()
