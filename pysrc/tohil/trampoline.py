

from io import StringIO
import re
import sys
import traceback

def handle_exception(type, val, traceback_object):
    print("handle exception invoked, type:")
    print(type)
    print("val:")
    print(val)
    print("format_tb:")
    print(traceback.format_tb(traceback_object))
    print("---")
    errorCode = ["PYTHON", type.__name__, val]
    errorInfo = "stub error info"
    return errorCode, errorInfo

class Trampoline:
    """exec something and return whatever it emitted to stdout

    crack exceptions and prepare to send them to tcl
    """

    def __init__(self, passed_globals, passed_locals):
        self.globals = passed_globals
        self.locals = passed_locals
        self.carat_pattern = re.compile('^ *\^')


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
            exc_list = self.get_exception()
            return 'exception', exc_list
        finally:
            pass
        return 'success'

    def get_exception(self):
        exc_type, exc_value, exc_tb = sys.exc_info()
        if exc_type is None:
            raise RuntimeError('get_exception called with no extant exception')
        exc_list = traceback.format_exception(exc_type, exc_value, exc_tb)
        err_msg = exc_list.pop().rstrip()
        exc_list.reverse()
        exc_list.pop() # get rid of header line
        print(f'exc_type {exc_type}, exc_value {exc_value}, exc_tb {exc_tb}')
        return exc_list

    def playpen(self, command):
        try:
            exec(command, self.globals, self.locals)
        except Exception as err:
            exc_type, exc_value, exc_tb = sys.exc_info()
            exc_list = traceback.format_exception(exc_type, exc_value, exc_tb)
            print("---")
            print(exc_list)
            print("---")
            if 'Traceback' in exc_list[0]:
                exc_list.pop(0)
                new_list = list()
            for string in exc_list:
                if self.carat_pattern.search(string) is not None:
                    new_list[0], string = string, new_list[0]
                new_list.insert(0, string)
            return new_list, exc_type, exc_value, exc_tb
        finally:
            pass
        return

