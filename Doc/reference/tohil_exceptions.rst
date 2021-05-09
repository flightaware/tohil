.. _tohil-exceptions:

Tohil Exceptions
================

If Tcl code invoked from Python using Tohil gets a Tcl traceback, and
no Tcl code traps the error, Tohil will receive the error and turn around
and throw a TclError exception.

TclError is an exception class that is, as Python requires, a class
derived from the :class:`BaseException` class.

In a `try` statement with an `except`
clause that mentions TclError, that clause will handle the Tcl exception.

What's nice about the TclError class is that it is populated by Tohil
with useful information that Tohil gleaned from the Tcl interpreter,
such as the interpreter result, traceback, Tcl error code, code level,
and in some cases the file and line number.

Likewise, uncaught exceptions in the Python interpreter resulting from
code invoked from Tcl using Tohil will propagate a TCL error including
a stack trace of the Python code that was executing. As the exception
continues up the stack, the Tcl stack trace will be appended to it.

The Tcl error code is set to a Tcl list comprising "PYTHON", the class
name of the exception, and the base error message.  This is experimental
but likely to continue.  We would like to add the class arguments, though.

Such Python errors may be caught (as per Tcl stack traces) with
Tcl's ``catch`` or ``try``, the same as any other TCL error.

    >>> try:
    ...     tohil.eval("no")
    ... except tohil.TclError as err:
    ...     mine = err
    ... 
    >>> mine
    <class TclError 'invalid command name "no"' ['TCL', 'LOOKUP', 'COMMAND', 'no']'>
    >>> mine.code
    1
    >>> mine.errorcode
    ['TCL', 'LOOKUP', 'COMMAND', 'no']
    >>> mine.errorline
    1
    >>> mine.errorinfo
    'invalid command name "no"\n    while executing\n"no"'
    >>> mine.errorstack
    'INNER no'
    >>> mine.level
    0
    >>> mine.result
    'invalid command name "no"'

Here is a sample Tcl session catching an uncaught Python exception as a
Tcl error:

   >>> tohil.interact()
   $ tclsh
   % package require tohil
   3.2.0
   % catch {tohil::eval "no"} catchResult catchDict
   1
   % puts $catchResult
   name 'no' is not defined
   % puts $catchDict
   -code 1 -level 0 -errorstack {INNER {invokeStk1 tohil::eval no} UP 1} -errorcode {PYTHON NameError {name 'no' is not defined}} -errorinfo {name 'no' is not defined
   from python code executed by tohil  File "tohil", line 1, in <module>
       invoked from within
   "tohil::eval "no""} -errorline 1


