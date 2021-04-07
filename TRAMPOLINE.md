

Tohil 3 brings the integration of tcl functions into python to a new level of transparency, simplicity and versatility.

Using Tcl's powerful introspection capabilities, tohil traverses a hierarchy of namespaces, finds all the procs and C-commands therein, and generates python wrappers for all the ones it finds.

tohil.import_tcl() returns a TclNamespace object, and all of the procs and commands in that namespace are defined as methods of the TclNamespace object, and can be executed as such methods.  It's very natural and very pythonic.

This means you can do stuff like:

import tohil
k = tohil.import_namespace("::")

And then invoke top level procs like k.intersect3().  And you can chain namespaces.

tohil.package_require("yajltcl")
k = tohil.import_namespace("::")

And the subordinate namespaces are created in there too, and they're chainable too.




k.clock('format', k.clock("seconds", to=seconds), "-format", "%D %T", "-gmt", 1)

