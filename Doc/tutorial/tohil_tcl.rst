.. _tutorial-python-from-tcl:

======================
Using Python From Tcl
======================

In this section we'll introduce using Python from Tcl.

Hopefully you've got Tcl and Python and Tohil installed and you can
follow along and try stuff out.

Let's fire up Tcl and mess around with Python::

   $ tclsh
   % package require tohil
   4.0.0

OK, good news, we've got a working Tcl and Tohil.

If the package-require failed then please visit
the installation instructions and get tohil built and installed
on your computer.

***************
tohil::eval
***************

::

   % tohil::eval "37 + 5"
   42

That may not look like much, but Tohil got Python to add 37 and 5 for us
and returned the result back to tcl.

::

  % set answer [tohil::eval "37 + 5"]
  42
  % puts $answer
  42

***************
tohil::exec
***************

In Python, *eval* evaluates a single expression and returns the
result, so even trying to eval something like ``answer = 42`` is
an error.  Python provides *exec*, which can evaluate an aribtrary
code block, and Tohil hews to that by providing *tohil::exec*.

::

   % tohil::exec "answer = 37 + 5"
   % tohil::exec "print(answer)"
   42

OK, that's pretty cool.  Tohil is getting Python to do stuff
for us from Tcl.  Yay.

(A quick note, *tohil::eval* and *tohil::exec* are named and work the
way Python's *eval* and *exec* work.  Tcl has its own *eval* for
evaluating Tcl stuff. It is for Tcl something closer to Python's *exec*,
except that Tcl's *eval* returns a result, while Tcl's *exec* runs programs
and returns their output, something much different.)

::

   % expr 5 / 4
   1
   % tohil::eval "5 / 4"
   1.25
   % tohil::eval "5 // 4"
   1
   % expr 5 // 4
   missing operand at _@_
   in expression "5 /_@_/ 4"
   while evaluating expr 5 // 4

Yep, it's Python we're talking to, all right.  See how Tcl division of
two integers yielded an integer result while Python, a float?  Then
we used Python's integer division *//* to get integer division, while
trying that with Tcl was an error because Tcl doesn't have that operator.

***************
tohil::import
***************

OK, we can start doing Python stuff from Tcl, like import a module.

::

   % tohil::exec "import numpy"

We do this often enough that Tohil provides a shortcut:

::

   % tohil::import numpy


****************
notes about exec
****************

One thing that can trip people up is it can be surprising that
tohil::exec never returns anything.

::

   % tohil::exec "answer = 42"
   % tohil::exec "answer"

The above returns without an error, but doesn't provide anything.

You instead need to use tohil::eval in this example.  You can call
functions using tohil::eval, by the way.

Though possibly a bit surprising, this behavior is consistent
with how exec works in Python. It probably sbhouldn't
be a surprise that Tohil is using Python's
eval and exec mechanisms at the C level to provide these capabilities
to Tcl.

***************
tohil::run
***************

tohil::run is a special version of tohil::exec that grabs anything
Python emits to stdout while the exec is running, and returns it
to the caller.

***************
tohil::call
***************

If you start creating from Tcl, Python to be executed with
eval and exec, you may notice there's a risk that if you use
substitute-in data, you know, such as names, addresses, cities
or whatever, that unless you are very careful, various characters
can cause your Python not to parse properly.  For example, a single
quote in a name, quotes in general, and other stuff.

Tohil provides *tohil::call* to make it possible to call a Python
function and make sure that the arguments you pass to the function
are not interpreted by Python along the way.


tohil::call provides a way to invoke one Python function, with zero or more
arguments, without having to pass it through Python's eval or exec and running
the risk that Python metacharacters appearing in the data will cause quoting
problems, accidental code execution, etc.

tohil::import provides a way to import Python modules, although it's not much
different from doing a tohil::exec "import module"


***************
tohil::interact
***************

Take tohil to eleven.  You're on ten here... all the way up... You're
on ten on your guitar... where can you go from there?  Where?  Nowhere.
Exactly.  What we do is if we need that extra... push over the cliff...
you know what we do?

We run *tohil::interact* from Tcl and enter the Python interactive loop.
When we're done, we send end of file (^D) to end the Python loop and
return to the Tcl one.

::

   % tohil::interact
   >>> def foo():
   ...   print("bar")
   ...
   >>> ^D
   % tohil::eval foo()
    bar


**********************
Using tohil from Rivet
**********************

Rivet is an Apache webserver module that provides among other things
a way for webpages to be made from HTML files with embedded Tcl code
that executes when the page is requested.

From a Rivet page, in some of your Tcl code, invoke `package require tohil`.

If you run tohil_rivet it will plug tohil's Python interpreter such that
everything Python writes to stdout using print, or whatever, will go through
Tcl's stdout and thereby into your Rivet page.

::

   <?

   package require tohil; tohil_rivet

   puts "calling out to Python to add 5 + 5: [::tohil::eval "5 + 5"]"

   tohil::exec {
       print('hello, world')
       print("<hr>")
   }

   ?>


