.. _tutorial-tcl-from-python:

======================
Using Tcl from Python
======================

Here we'll introduce using Tcl from Python.

Hopefully you've got Tcl and Python and Tohil installed and you can
follow along and try stuff out.

**************
tohil.eval
**************

::

   >>> import tohil
   >>> tohil.eval('puts "Hello, world."')
   Hello, world.

Not bad.  You can actually do a lot with that.

Anything the Tcl code returns can be gotten by Python.

::

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

*****************
helper functions
*****************

We can load in Tcl packages by doing

::

   >>> tohil.eval('package require Tclx')

but we do this so often that tohil provides a shortcut:

::

   >>> tohil.package_require('Tclx')

You can specify the verison as an optional argument, either by
positional or named parameter.  The following two statements
are equivalent:

::

   >>> tohil.package_require('Tclx', '8.6')
   >>> tohil.package_require('Tclx',version='8.6')

Experienced Python developers without a lot of Tcl experience may be surprised
by Tcl's leniency when it comes to data types.

Here we request a Tcl package with the version number specified as
floating point.  It works fine.

::

   >>> tohil.package_require('Tclx', 8.6)

Another one you'd end up doing a lot is ``tohil.eval("source file.tcl")``.  For that
we provide the slightly less paper-cutty...

::

   >>> tohil.source("file.tcl")


**************
tohil.call
**************

You get fancy and start using f-strings to create Tcl commands with
arguments, maybe you're doing something like

::

   tohil.eval(f"register_user {user_id} {user_name} {user_fullname}")

If any of those variables being substituted contain dollar signs,
quotes, or square brackets, you're not going to have a good time,
because Tcl is going to try to interpret that stuff, and that could
lead to errors up to and including remote code execution.

Consequently, Tohil provides *tohil.call*, a function that takes an
arbitrary number of arguments and passes them one-for-one to the
corresponding Tcl function in a way that keeps Tcl from trying
to interpret any of the arguments.

::

   >>> import tohil
   >>> clock = 1616182348
   >>> tohil.call('clock', 'format', clock, '-locale', 'fr')
   'ven. mars 19 19:32:28 UTC 2021'

The key thing in the above is
``tohil.call('clock', 'format', clock, '-locale', 'fr')``, equivalent
to ``tohil.eval(f"clock format {clock} -locale fr")`` but without the
risk of inadvertent misinterpretation of arguments.


**************
tohil.expr
**************

You can also evaluate Tcl expressions from Python using *tohil.expr*.
As with many other tohil functions, to= can be used to request conversion to a
specific Python datatype.

::


   >>> tohil.expr('5+5')
   '10'
   >>> tohil.expr('5**5')
   '3125'
   >>> tohil.expr('1/3')
   '0'
   >>> tohil.expr('1/3.')
   '0.3333333333333333
   >>> tohil.expr('1/3.',to=float)
   0.3333333333333333
   >>> tohil.expr('[clock seconds] % 86400')
   '25571'
   >>> tohil.expr('[clock seconds] % 86400',to=int)
   25571

******************************
tohil.getvar and tohil.setvar
******************************

Python has direct access to Tcl variables and array elements
using *tohil.getvar*.  Likewise, *tohil.setvar* can set them.

::

   >>> import tohil
   >>> tohil.setvar("foo", "bar")
   >>> tohil.getvar("foo")
   'bar'
   >>> tohil.setvar(var="happy", value="lamp")
   >>> tohil.getvar("happy")
   'lamp'

   >>> tohil.eval("array set x [list a 1 b 2 c 3 d 4]")
   ''
   >>> tohil.getvar('x(a)')
   '1'
   >>> tohil.getvar('x(a)', to=int)
   1
   >>> tohil.getvar(var='x(b)', to=float)
   2.0
   >>> tohil.getvar("x(e)")
   Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
   NameError: can't read "x(e)": no such element in array

As you can see, it's an error to try to get a variable or array element
that doesn't exist.  You can use *tohil.exists* to see if the variable
exists, or trap the Python exception, or make use of *tohil.getvar*'s handy
*default* keyword-only argument:

::

   >>> tohil.getvar("x(e)", default="0")
   '0'
   >>> tohil.getvar("x(e)", default=0, to=int)
   0
   >>> tohil.getvar("x(d)", default=0, to=int)
   4

****************
tohil.exists
****************

You can use *tohil.exists* to see if a variable or array element exists:

::

   >>> tohil.eval("array set x [list a 1 b 2 c 3 d 4]")
   ''
   >>> tohil.exists("x(c)")
   True
   >>> tohil.exists("x(e)")
   False
   >>>
   >>> tohil.exists("banana")
   False


***************
tohil.incr
***************

*tohil.incr* takes a Tcl variable name or array element and attempts
to increment it.

If the contents of the variable preclude it being used as an int, a Python
TypeError exception is thrown.

An optional position argument specifies the amount to increment by.
The default increment is 1.
Negative increments are permitted.
The increment amount can also be specified as
a keyword argument, using "incr".

::

   tohil.incr('var')
   tohil.incr('var',2)
   tohil.incr('var',incr=-1)


**************
tohil.unset
**************

*tohil.unset* can be used to unset variables, array elements, and even entire
arrays in the Tcl interpreter.

::

   >>> tohil.setvar("x(e)", "5")
   >>> tohil.getvar("x(e)")
   '5'
   >>> tohil.unset("x(e)")
   >>> tohil.getvar("x(e)")
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   NameError: can't read "x(e)": no such element in array

* Unset takes an arbitrary number of arguments, including zero.
* Unsetting an array element uses Tcl subscript notation, for example
  ``tohil.unset('x(e)')``.
* Unsetting an array by name without a subscript will unset the entire array.
* It is not an error to attempt to unset a variable that doesn't exist.

******************
tohil.subst
******************

Tcl's *subst* command is pretty cool.  By default it performs Tcl backslash,
command and variable substitutions, but doesn't evaluate the final result,
like *eval* would.  So it's handy for generating some kind of string, but
with embedded $-substitution and square bracket evaluation.

::

   >>> import tohil
   >>> tohil.eval("set name karl")
   'karl'
   >>> tohil.subst("hello, $name")
   'hello, karl'

*******************
tohil.convert
*******************

*tohil.convert* will convert some Python thing passed to it, into a Tcl
object, and then back to some other Python type,
any type supported in accordance with the to= argument.


The "to=" way of requesting a type conversion is supported.  Although you might not care about converting to int or float or something, you might want a tohil.tclobj for your efforts, anirite?

*******************
tohil.interact
*******************

Run the Tcl interactive command loop on stdin, hopefully a terminal, until
you send an EOF, at which point you'll be returned to the Python command
line.  See also *tohil::interact*.

THis is handy if you're using Python interactively and you find yourself
making a lot of *tohil.eval* calls to manipulate the Tcl interpreter, you
can flip to the Tcl interpreter, interact with it directly, then flip
back by sending an end-of-file.



tcldict
tclobj
tclvar
