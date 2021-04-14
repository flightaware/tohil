

Tohil 3 Release Notes

Welcome to Tohil 3.  

Tohil 3 provides the means of accessing Tcl functions from python in such a way that they very much look and behave like native python functions.  Not only that but for tcl procs made available by tohil, every parameter can be specified by position or by name, something few native python functions or python C functions can do.

Any Tcl proc or C command can be defined as a python function simply by creating a TclProc object and then calling it.



Tohil 3 also adds a sweet TclError exception class, and any tcl errors that bubble back all the way to python without any tcl code having caught the error will be thrown in python as TclError exceptions.  The TclError object can be examined to find out all the stuff Tcl knows about the error... the result, the error code, code level, error stack, traceback, and error line.

New helpers Functions

* tohil.package_require(package_name, version=version)
* tohil.info_procs()
* tohil.info_commands()
* tohil.info_body()
* tohil.info_default()
* tohil.info_args()
* tohil.namespace_children()


Dozens of new tests.


