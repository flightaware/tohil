

import tohil
from io import StringIO
import sys

class TclWriter:
    def __init__(self):
        pass

    def write(self, string):
        tohil.call('puts', '-nonewline', string)
        self.flush()

    def flush(self):
        tohil.call('flush','stdout')

class Tcller:
    """exec something and return whatever it emitted to tcl's stdout"""

    def __init__(self, passed_globals, passed_locals):
        self.globals = passed_globals
        self.locals = passed_locals

    def run(self, command):
        try:
            their_stdout = sys.stdout
            my_stdout = StringIO()
            sys.stdout = my_stdout

            exec(command, self.globals, self.locals)
        finally:
            sys.stdout = their_stdout

        return my_stdout.getvalue()
