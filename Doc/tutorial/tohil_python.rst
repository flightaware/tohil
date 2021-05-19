.. _tutorial-tcl-from-python:

======================
Using Tcl from Python
======================

::

   >>> import tohil
   >>> clock = 1616182348
   >>> tohil.eval(f"clock format {clock} -locale es -gmt 1")
   'vie mar 19 19:32:28 GMT 2021'
