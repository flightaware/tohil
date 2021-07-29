
.. tohil-python-functions:


Tohil Python Functions
======================

Tohil has a number of functions and data types that it provides
when the tohil package has been imported.

.. function:: tohil.call(* args[, to=type])

   Invoke a Tcl command while specifying each argument explicitly,
   and returns the result.

   This way, even if some arguments contain Tcl metacharacters such
   as dollar sign, backslash, and square brackets, Tcl will not
   evaluate them.

   Zero or more arguments can be specified.  If one or more arguments
   are specified, the first argument is the command name (could be a proc
   or a C function or whatever), and whatever additional positional
   arguments are passed as arguments to the command.

   If no arguments are specified, that's legal for Tcl.  The Tcl
   interpreter will evaluate an empty string, and an empty result.

   The optional *to=* named parameter can specify a Python data type
   to return, such as *str*, *int*, *float*, *bool*, *list*, *set*,
   *dict*, *tuple*, *tohil.tclobj*, *tohil.tcldict*, or a function that takes
   one argument and returns a result.

   If the evaluation results in a Tcl error and the error is not caught
   by inline Tcl code using *try* or *catch*, that is to say if an
   uncaught Tcl error is received by Tohil from the attempt, Tohil
   uses information about the Tcl error to raise a TclError exception
   to Python.

.. function:: tohil.convert(python_object)

    Convert some Python object into a Tcl object and then convert
    back to a Python object, a tohil.tclobj by default, but it can
    be converted to any optional *to=* destination type or be passed through
    a *to=* function.

.. function:: tohil.eval([tcl_code=]code[, to=type])

   Given a string of valid Tcl code, including at the caller's discretion
   multiple statements separated by semicolons, or multiline blocks,
   evaluate *tcl_code* using the Tcl interpreter, and return its result,
   by default as a *tohil.tclobj* data type.

   As with *tohil.call*, above, if the evaluation results in an uncaught
   Tcl error, Tohil will construct and raise a TclError exception to
   Python.

.. function:: tohil.exists([var=]varString)

   Returns True if the variable named by varString exists, or False if it
   doesn't.

   *varString* can be an element of a Tcl array by using Tcl array notation,
   for example 'airports(KHOU)', and tohil.exists will return
   based on the existence of the specified element in the specified array.

.. function:: tohil.expr([expression=]exprString[, to=type])

   Evaluate *exprString* as a Tcl expression, and returns the result.

   The optional *to=* named parameter can be supplied to specify one of
   the supported Python data types or functions.

.. function:: tohil.getvar([var=]varString, to=tohil.tclobj[, default=defVal])

   Get a Tcl variable or array element and return it to the caller.

   The variable is accessed from the current Tcl context, which may
   be global.

   The name of the variable or array element is in *varString*.

   *varString* can include namespace qualifiers to ensure a reference
   is global or to explicitly access a variable within a specific namespace.

   The optional *to=* named parameter can be supplied to specify one of
   the supported Python data types or functions.

   An optional default value can be specified using the *default=*
   named parameter.  If a default value is specified and the
   specified variable or array element doesn't exist in the
   Tcl interpreter, the default value will be returned instead.
   *default=None* is a valid default value and is distinct from
   not providing a default value.

   Note that default values are coerced to the *to=* data type,
   a tohil.tclobj by default.

.. function:: tcl = tohil.import_tcl()

   Using Tcl's introspection capabilities, traverse all Tcl
   namespaces, identify all procs and C commands in each one.

   Create a hierarchy of TclNamespace objects returning the
   top-level namespace object.

   For the procs, suss out their arguments andf default values,
   and attach to each namespace entrypoints for each proc and
   C command so that calling the Tcl procs looks very much
   like calling any Python function.

.. function:: tohil.incr([var=]varName[, [incr=]increment])

   Take a Tcl variable name or array element as specified
   by the *varName* string, and attempt to increment it.

   The optional increment amount can be specified positionally
   or using the *incr=* keyword.  Its value is **1** by default.
   The increment amount can be negative.

   If the variable doesn't exist, it is created and set
   to the increment amount.

   If the contents of the variable preclude it being used as
   an integer, a Python TypeError exception will be thrown.

.. function:: tohil.interact()

   Run the Tcl interactive command loop on stdin, which hopefully
   is a terminal, until the user sends EOF, at which point they'll
   be returned to the Python command line, or whatever the Python
   code that called *tohil.interact()* does next.

.. function:: tohil.package_require(packageName[, [version=]versionID])

   Load the specified package.  A specific package version
   can be specified, either positionally or by name using
   the *version=* parameter.

   This is a shortcut for
   ``tohil.eval(f"package require {packageName} {versionID}")``.

.. function:: tohil.register_callback(name, callback)

   Create a Tcl command with the given name linked to the given Python
   callable. When the command is invoked, it will directly invoke the callback,
   passing along any arguments. This is useful in cases where the Tcl event
   loop is utilized to execute code asynchronous.

.. function:: tohil.result([to=type])

   Return the Tcl interpreter result object.

   The Tcl interpreter has a "result object."  It contains
   the result of the last thing the interpreter did.

   It's not something you would likely normally need to access, because
   you would have gotten the result by doing something like
   ``set myResult [myFunction myArg1 myArg2]``.

   Nonetheless we make it available because it's been useful for
   the Tohil devs to be able to see what's in there.

.. function:: tohil.run()

   Perform tohil.exec, but redirect stdout emitted while
   python is running it into a string and return
   the string to run's caller after the exec has finished.

   Python users are often surprised that exec doesn't return
   anything.

.. function:: tohil.setvar([var=]varName[, [value=]value)

   Set a variable or array element referenced by *varName*
   to the value specified by *value*.

   A few errors are possible, such as trying to set an array
   element of a scalar variable or set a scalar variable
   that is actually an array.

.. function:: tohil.source(fileName)

   Take the contents of the file specified by *fileName* and
   evaluate it using the Tcl interpreter.  The return value
   is the value of the last command executed in the script.

   This is the equivalent of ``tohil.call("source", fileName)``.

.. function:: tohil.subst(substString)

   Perform Tcl backslash, command and variable substiutions,
   and return the result of doing that without evaluating it.

   This is handy for generating some kind of string while
   substituting parts of it with embedded $-substitutions of
   Tcl variables and evaluation of Tcl code enclosed in square
   brackets.

.. function:: tohil.tclvar()

   Create a tclobj object that shadows a Tcl variable or
   array element.

   Any accesses of the resulting tclobj from Python will
   always begin with a (noncopying) access of the Tcl
   variable or array element's contents, and any writing
   of the variable from Python (by doing things with the tclobj
   such as invoking methods on them, using Python list notation
   to update tclobj list elements, etc.

.. function:: tohil.unset(* args)

   *tohil.unset* is used to unset variables, array elements,
   and even entire arrays in the Tcl interpreter.

   Zero or more arguments specify names to unset.

   Unsetting an array element uses subscript notation, for
   example *x(e)*.

   Unsetting an array by name without a subscript will unset
   the entire array.

   It is not an error to attempt to unset variables, arrays and
   array elements that don't exist.


.. function:: tohil.tcl_stdout_to_python()

   Tcl normally uses its own I/O system to read and write data.

   As *tohil.rivet()* can be used from Python to redirect Python's
   writing to standard output to go through Tcl's I/O subsystem
   (and, hence, to Rivet),
   *tohil.tcl_stdout_to_python* does the opposite, configuring the
   Tcl interpreter to redirect its
   standard output, *stdout*, away from Tcl's I/O subsystem and
   instead send whatever is written through Python's.

   If using `Jupyter Notebook <https://https://jupyter.org>`_, invoking
   tohil.tcl_stdout_to_python() will cause
   any Tcl output written to standard output to appear in the notebook rather
   than in the log file or stdout of the jupyter command line that's causing
   the Jupyter notebook webserver to exist.



