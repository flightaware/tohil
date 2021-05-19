.. _tutorial-tcl-from-python:

======================
Using Tcl from Python
======================

Here we'll introduce using Tcl from Python.

Hopefully you've got Tcl and Python and Tohil installed and you can
follow along and try stuff out.

::

   >>> import tohil
   >>> tohil.eval('puts "Hello, world."')
   Hello, world.

Not bad.  You can actually do a lot with that.

Anything the Tcl code returns can be gotten by Python.

::

   >>>
   >>> t = tohil.eval('return "Hello, world."')
   >>> t
   <tohil.tclobj: 'Hello, world.'>
   >>> str(t)
   'Hello, world.'

Here we'll use Tcl's *clock format* function to format a
Unix epoch seconds-since-1970 clock into a Posix standard
time in the Spanish locale:

::

   >>> clock = 1616182348
   >>> tohil.eval(f"clock format {clock} -locale es -gmt 1")
   'vie mar 19 19:32:28 GMT 2021'

