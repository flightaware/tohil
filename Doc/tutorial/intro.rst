.. _tutorial-intro:

######################
  The Tohil Tutorial
######################

Tohil is simultaneously a Python extension and a TCL extension that
makes it really seamless to move data around and invoke functions in
one from the other.

Tohil is open source software, available for free including for profit and/or for redistribution, under the permissive 3-clause BSD license (See :ref:`_copyright-and-license`).

It is written in C, Python and Tcl, and makes use of the Python and Tcl C APIs
to let Python call Tcl, Tcl call Python, give Python access to Tcl's
objects and give Tcl access to Python's objects.

Not just strings and ints and float, but lists, dicts, sets, tuples, and
more, flow freely, intuitively and largely unencumbered between the two
languages.

Tohil is efficient when moving data between the languages.
Integers and floats are copied natively; they do not suffer an
intermediate conversion through strings.  Likewise lists, tuples, dicts,
etc, are accessed through the C language mechanisms provided by the two
languages, ergo accessed and manipulated in the most native and efficient
ways.

Tohil is freely available
in source form from the Tohil github website,
https://github.com/flightaware/tohil, and may be freely distributed.

This tutorial is intended to provide an introduction to Tohil, and
typical ways of using it.

To follow along and start experimenting with Tohil you'll want to have  working Python
and Tcl interpreters, and have
Tohil built and installed such that `import tohil` works from
Python and `package require tohil` works from Tcl.


