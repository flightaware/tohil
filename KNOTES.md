

have to build without stubs to create a dylib that python likes

turn off stubs in configure.in

change -ltclstub86 to -ltcl86 in Makefile -- this needs to be done properly

python setup.tcl build builds a shared library but it's no good, at least not so far

but if you copy the one that make makes into the place that the setup.py installer thing uses, import tclpy works from python3


