


## I want to get up and running as quickly as possible, accessing my tcl stuff from python.

Great!

Tohil gives you a few options for how to run tcl code and access tcl data from python.

You can pick and choose what you need, and how far you want to go.

After building and installing tohil, from python you should be able
to `import tohil` from the python command line.

### tohil.eval

The simplest way to do something with tcl from python, is to
use `tohil.eval`.

```
import tohil
tohil.eval("package require Tclx")
```

You can call tohil.eval as much as you want, and feed it any tcl
code, and tcl will evaluate it, and return the result.
If a tcl error occurs and it isn't caught by "try" or "catch",
tohil will raise a python TclError exception that contains all
the stuff tcl knows about the error.

Pretty cool.  Very handy.  You'll probably use it some, and for some
people it may be all that they need.

But if we get to the point where we're trying to embed data into the
stuff we're passing to "eval", like say with f-strings or something...

```
tohil.eval(f"""set users({user}) [list name "{name}" address "{address}" phone "{phone}"]""")
```

^ don't do this!

...we are asking for trouble.  Dollar signs, double quotes, curly brackets,
square brackets in the data, the tcl interpreter will try to interpret it.

You can do some heavy lifting to try to make sure that the data if properly
quoted, but that's pretty hard and kind of error prone and if you miss
running your data through your conditioner anywhere, you've got the risk
of the problem again.

### tohil.call

tohil.call is a way to call tcl with each argument specified explicitly.

This way, even if the arguments contain metacharacters we can ensure they
are not evaluated by tcl because when invoked this way, tcl will not
evaluate tohil.call's arguments.

There are tcl-side equivalents to these things for calling python from
tcl, by the way.

And we provide that...  And it's really handy.  But since you have to
explicitly invoke this "call" function every time you want to call out to
tcl from python and to python from tcl, we wouldn't call that fluent.

"Fluent" would be where the tcl commands look and act like python commands
to a very high degree.

"Fluent" also would mean that when exceptions and errors are handled
gracefully across the boundary between the two languages.  That is,
for example, if a python exception is thrown and no python code in the
call stack traps it, ergo it makes its way all the way back to tcl,
that it is translated into a tcl error that represents the python error
in a way that is thorough and accurate, such that tcl code can figure
out the error and see the important error information such as the
traceback, error object type, etc, using tcl's facilities for doing that.
Tohil supports that.

Likewise from the python side, an uncaught tcl error from tcl code
invoked from python should throw a python exception by creating an
exception object that robustly contains the tcl error information,
and tohil does that with its TclError class, which can be poked and
prodded to find out the tcl result, error code, traceback, etc.

Recognize that it's going to need to go both ways.  Once you begin to create
your libraries of python code, it'll be inevitable that you'll need to
call python from tcl as well.  Currently from tcl you need to use
tohil::eval, tohil::exec, and tohil::call to call python, although
Gerald Lester's pyman extension points to a way to introspect python to
generate tcl-side code structures to make python functions and classes
look to tcl like tcl ones.  We are likely to add such support in a
future version of tohil.



WHEN we import namespaces should we also import variables and arrays!?!?

could it see the non-array namespace variables as a dict of variables associated with the namespace






