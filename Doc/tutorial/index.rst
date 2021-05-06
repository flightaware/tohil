.. _tutorial-index:

######################
  The Tohil Tutorial
######################

Tohil is an easy to understand, powerful piece of software that is
simultaneously a Python module and a Tcl package.

It is written in C, Python and Tcl, and makes use of the Python and Tcl C APIs
to let Python call Tcl, Tcl call Python, give Python access to Tcl's
objects and give Tcl access to Python's objects.

Not just strings and ints and float, but lists, dicts, sets, tuples, and
more, flow freely, intuitively and largely unencumbered between Python and
Tcl.

Tohil is as efficient as can be when moving data between the languages.
Integers and floats are copied natively, that is they do not suffer an
intermediate conversion through strings.  Likewise lists, tuples, dicts,
etc, are accessed through the C language mechanisms provided by the two
languages, ergo accessed in native and efficient ways.

Tohil is freely available
in source form from the Tohil github website,
https://github.com/flightaware/tohil, and may be freely distributed.

This tutorial introduces the reader informally to the basic concepts and
features of the Tohil. It helps to have Python and Tcl
interpreters handy for hands-on experience, but all examples are self-contained,
so the tutorial can be read off-line as well.

