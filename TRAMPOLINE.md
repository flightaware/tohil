

Tohil 3 brings the integration of tcl functions into python to a new level of transparency, simplicity and versatility.  Most tcl procs now look and behave just like python functions in most cases.

Using Tcl's introspection capabilities, tohil traverses a hierarchy of tcl namespaces, identifying all the procs and C-commands in each one, and for the procs, sussing out their arguments and default values, and generating python wrapper functions for all the procs it finds.

While tohil can't determine arguments and defaults for tcl commands that are implemented by C, tohil still generates a wrapper for them, making them available from python.  Since many tcl commands and extensions are implemented in C and provide their functionality with a hybrid of tcl procs and C commands, wrapping the C functions can be important for providing a way for python to have access to everything such a package provides.

tohil.import_tcl() returns a TclNamespace object corresponding to whatever namespace you point it at ("::" is a good one), and all of the procs and commands found in that namespace are defined as methods of the TclNamespace object, and can be executed as such methods.  It's very natural and pythonic.

This means you can do stuff like:

import tohil
k = tohil.import_namespace("::")

And then invoke top level procs like k.intersect3().  And you can chain namespaces.

tohil.package_require("Tclx")
k = tohil.import_namespace("::")

in_a_but_not_b, in_both, in_b_but_not_a = k.intersect3(list1, list2, to=tuple)

And the subordinate namespaces are created in there too, and they're chainable too.




k.clock('format', k.clock("seconds", to=seconds), "-format", "%D %T", "-gmt", 1)

