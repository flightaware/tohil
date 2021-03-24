"""tcl support stuff - TclWriter class can plug python's stdout into Tcl's"""

import tohil


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
