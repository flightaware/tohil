

from io import StringIO
import re
import sys
import traceback

def handle_exception(type, val, traceback_object):
    """handle_exception - the tohil C code that handles uncaught
    python exceptions invokes this to turn an exception type, value
    and traceback object into a tcl error code and error info"""
    errorCode = ["PYTHON", type.__name__, val]
    tb_list = traceback.format_tb(traceback_object)
    errorInfo = "\nfrom python code executed by tohil\n" + " ".join(tb_list).rstrip()
    return errorCode, errorInfo

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

