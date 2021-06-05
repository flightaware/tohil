
.. tohil-built-ins:


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

.. function:: tohil.package_require()

.. function:: tohil.result()

.. function:: tohil.run()

.. function:: tohil.setvar()

.. function:: tohil.source()

.. function:: tohil.subst()

.. function:: tohil.tcldict()

.. function:: tohil.TclNamespace()

.. function:: tohil.tclobj()

.. function:: tohil.TclProc()

.. function:: tohil.tclvar()

.. function:: tohil.TclWriter()

.. function:: tohil.traceback()

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


