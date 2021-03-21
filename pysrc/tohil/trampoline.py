

from io import StringIO
import sys
import traceback

class Trampoline:
    """exec something and return whatever it emitted to stdout

    crack exceptions and prepare to send them to tcl
    """

    def __init__(self, passed_globals, passed_locals):
        self.globals = passed_globals
        self.locals = passed_locals

    def run(self, command):
        try:
            their_stdout = sys.stdout
            my_stdout = StringIO()
            sys.stdout = my_stdout

            exec(command, self.globals, self.locals)
        except Exception as err:
            print(f"exception: {err}")
            print(f"exception type: {type(err).__name__}")
            print(f"exception args: {err.args}")
            print('format_exception():')
            exc_type, exc_value, exc_tb = sys.exc_info()
            print(traceback.format_exception(exc_type, exc_value, exc_tb))
        finally:
            sys.stdout = their_stdout

        return my_stdout.getvalue()

    def goof(self, command):
        try:
            exec(command, self.globals, self.locals)
        except Exception as err:
            print(f"exception: {err}")
            print(f"exception type: {type(err).__name__}")
            print(f"exception args: {err.args}")
            print('format_exception():')
            exc_type, exc_value, exc_tb = sys.exc_info()
            ret = traceback.format_exception(exc_type, exc_value, exc_tb)
        finally:
            pass
        return ret
