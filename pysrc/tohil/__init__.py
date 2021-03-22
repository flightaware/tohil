""" tohil """

import sys

from tohil.trampoline import handle_exception, run

# handle_exception must be defined before importing from
# _tohil, which triggers loading of the C shared library,
# which looks for it upon load

from tohil._tohil import (
    eval, expr, getvar, interp, setvar, subst, call, plug,
)

from tohil.tcller import TclWriter

class RivetControl:
    """probably lame stuff to redirect python stdout to tcl,
    but only once"""
    def __init__(self):
        self.activated = False
        self.tclWriter = TclWriter()

    def activate(self):
        if self.activated:
            return

        sys.stdout = self.tclWriter
        self.activated = True

global rivetControl
rivetControl = RivetControl()

def rivet():
    """redirect python's stdout to write to tcl's stdout"""
    rivetControl.activate()

