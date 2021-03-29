""" tohil """

import sys

from tohil.trampoline import handle_exception, run

# too few public methods.  come on, man.
#pylint: disable=R0903

# handle_exception must be defined before importing from
# _tohil, which triggers loading of the C shared library,
# which looks for it upon load

from tohil._tohil import (
    call,
    eval,
    exists,
    expr,
    getvar,
    interp,
    plug,
    setvar,
    subst,
    unset,
)

from tohil.tcller import TclWriter


class RivetControl:
    """probably lame stuff to redirect python stdout to tcl,
    but only once"""

    def __init__(self):
        self.activated = False
        self.tcl_writer = TclWriter()

    def activate(self):
        """activate rivet control, but only do the work once"""
        if self.activated:
            return

        # plug the tcl writer into stdout
        sys.stdout = self.tcl_writer
        self.activated = True


#global rivet_control
rivet_control = RivetControl()


def rivet():
    """redirect python's stdout to write to tcl's stdout"""
    rivet_control.activate()
