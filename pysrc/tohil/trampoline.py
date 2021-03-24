"""trampoline

support functions for tohil
"""

from io import StringIO
import sys
import traceback

# we call exec.  it's necessary.
#pylint: disable=W0122

def handle_exception(exception_type, val, traceback_object):
    """handle_exception - the tohil C code that handles uncaught
    python exceptions invokes this to turn an exception type, value
    and traceback object into a tcl error code and error info"""
    error_code = ["PYTHON", exception_type.__name__, val]
    tb_list = traceback.format_tb(traceback_object)
    error_info = "\nfrom python code executed by tohil\n" + " ".join(tb_list).rstrip()
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
