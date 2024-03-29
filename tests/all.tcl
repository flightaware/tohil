# all.tcl --
#
# This file contains a top-level script to run all of the Tcl
# tests.  Execute it by invoking "source all.test" when running tcltest
# in this directory.
#
# Copyright (c) 1998-1999 by Scriptics Corporation.
# Copyright (c) 2000 by Ajuba Solutions
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

set tcltestVersion [package require tcltest]
namespace import -force tcltest::*

# tcltest::configure -verbose {body pass skip error}

tcltest::testsDirectory [file dir [info script]]
exit [tcltest::runAllTests]
