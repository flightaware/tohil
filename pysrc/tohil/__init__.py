""" tohil """

from collections.abc import MutableMapping
from io import StringIO
import sys
import traceback


# too few public methods.  come on, man.
#pylint: disable=R0903

### tcl support stuff - TclWriter class can plug python's stdout into Tcl's

class TclWriter:
    """TclWriter - class that can plug python's stdout into Tcl's"""
    def __init__(self):
        pass

    def write(self, string):
        """write a string, to Tcl"""
        tohil.call("puts", "-nonewline", string)
        # self.flush()

    def flush(self):
        """flush tcl's stdout"""
        tohil.call("flush", "stdout")


### trampoline support functions for tohil


# we call exec.  it's necessary.
#pylint: disable=W0122

def handle_exception(exception_type, val, traceback_object = None):
    """handle_exception - the tohil C code that handles uncaught
    python exceptions invokes this to turn an exception type, value
    and traceback object into a tcl error code and error info"""
    error_code = ["PYTHON", exception_type.__name__, val]

    if traceback_object is None:
        tb_list = list()
    else:
        tb_list = traceback.format_tb(traceback_object)
    error_info = "\nfrom python code executed by tohil" + " ".join(tb_list).rstrip()
    return error_code, error_info


def run(command):
    """run - perform exec but redirect stdout while
    python is running it into a string and return
    the string to run's caller after the exec has finished

    no need to worry about catching the exception; we'll
    let it go for tohil to take care of
    """
    try:
        their_stdout = sys.stdout
        my_stdout = StringIO()
        sys.stdout = my_stdout

        exec(command, globals(), locals())
    finally:
        sys.stdout = their_stdout

    return my_stdout.getvalue()

### shadow dictionaries

class ShadowDictIterator():
    def __init__(self, tcl_array):
        self.keys = tohil.call("array", "names", tcl_array, to=list)
        self.keys.sort()

    def __iter__(self):
        return self

    def __next__(self):
        if len(self.keys) == 0:
            raise StopIteration

        return self.keys.pop(0)

class ShadowDict(MutableMapping):
    def __init__(self, tcl_array):
        self.tcl_array = tcl_array

    def __getitem__(self, key):
        return tohil.getvar(f"{self.tcl_array}({key})")

    def __delitem__(self, key):
        tohil.unset(f"{self.tcl_array}({key})")
        return

    def __setitem__(self, key, value):
        tohil.setvar(f"{self.tcl_array}({key})", value)

    def __len__(self):
        return tohil.call("array", "size", self.tcl_array, to=int)

    def __repr__(self):
        return str(tohil.call("array", "get", self.tcl_array, to=dict))

    def __iter__(self):
        return ShadowDictIterator(self.tcl_array)

    def __contains__(self, key):
        return tohil.exists(f"{self.tcl_array}({key})")


### rivet stuff

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

### import our C language stuff

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

