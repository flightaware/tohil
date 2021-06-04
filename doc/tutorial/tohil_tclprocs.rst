
*************
TclProcs
*************

TclProcs brings the integration of Tcl functions into Python to a new
level of transparency, simplicity and versatility.  Using TclProcs,
Most Tcl procs
now look and behave just like Python functions in most cases.

Using Tcl's introspection capabilities, tohil traverses a hierarchy
of Tcl namespaces, identifying all the procs and C-commands in each one,
and for the procs, sussing out their arguments and default values,
and stashing it so that we can create entrypoints for all of the
Tcl procs in Python to invoke Tohil's trampoline function to call
the Tcl proc and return the result.

It's awesome!

While tohil can't determine arguments and defaults for Tcl commands
that are implemented in C, Tohil still makes entrypoints for them,
making them available from Python.  Since many Tcl commands and
extensions are implemented in C and provide their functionality
with a hybrid of Tcl procs and C commands, wrapping the C functions
can be important for providing a way for Python to have access to
everything such a package provides.

``tohil.import_tcl()`` returns a TclNamespace object corresponding to whatever
namespace you point it at ("::" is a good one), and all of the procs and
commands found in that namespace are defined as methods of the TclNamespace
object, and can be executed as such methods.  It's very natural and pythonic.

This means you can do stuff like:

::

    import tohil
    k = tohil.import_namespace("::")

...and then invoke top level procs like k.intersect3().
And you can chain namespaces.

::

    >>> tohil.package_require("Tclx")
    >>> k = tohil.import_tcl()
    list1 = ["a", "b", "c", "d", "e", "f"]
    list1 = ["d", "e", "f", "g", "h", "i"]
    a_only, in_both, b_only = k.intersect3(list1, list2, to=tuple)

And the subordinate namespaces are created in there too, and they're chainable too.

::

    k.clock('format', k.clock("seconds", to=seconds), "-format", "%D %T", "-gmt", 1)


