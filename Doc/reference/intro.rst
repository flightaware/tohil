
.. _introduction:

************
Introduction
************

This reference describes Tohil, the Python module and Tcl package
that are two great tastes that taste great together.

Tohil provides ways to exchange data and execute code
between the Python and Tcl interpreters.

Python executes Tcl code using *tohil.eval* and *tohil.call*, and some
minor variants.

Tcl executes Python code using *tohil::eval*, *tohil::exec*, and some
variants.

Tohil's *tclobj* and *tcldict* Python type objects provide Python access
to Tcl's native objects.  Tclobjs and tcldicts behave in a very Python-like
way.  For instance, tclobjs can be used as numbers in calculations (if they
contain valid numbers), treated as lists and accessed using familiar Python
list syntax and methods, be automatically constructed by ingesting familiar
Python data types, including recursive ingestion of lists, dicts, tuples, sets,
etc.

Meanwhile, uncaught Tcl errors resulting from Tcl code invoked from Python
are propagated back through Python as TclError exceptions, while
uncaught exceptions raised from Python code invoked from Tcl are
propagated back through Tcl as a Tcl error, the traceback interspersing Tcl
and Python as the error/exception unwinds.

