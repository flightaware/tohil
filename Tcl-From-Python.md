

The use case is you have a lot of valuable Tcl code that works that you're
not crazy enough to try to get rid of, but you want to be able to use python
for web stuff and/or backend stuff, get access to python and all the libraries
available for it, the benefits of the much larger user community, greater 
acceptance among the developers (possibly your employees) and the larger
talent pool, etc.

It's not going to work to rewrite.  You can't.  THere are too many
customers that are already using what you've got, and there are bug
fixes and improvements that they're waiting on.  You're going to retask some
considerably proportion of your staff to rewrite years to bazillions
of years of work, and almost for sure you're going to go out of business.
You're going to go out of business because your rewrite is going to fail.
Your rewrite will regress, recreate tons of bugs that were already fixed
in the old stuff, cause trouble tickets, maintenance, tl;dr you're not
going to like it.

So you have a ton of Tcl code and you want to get to python.  What's it
going to take to do that?

You're gonna need Python and Tcl to be fluent with each other.

What does "fluent" mean?  From my dictionary... fluent - adjective - ...able
to express oneself easily and articulately... able to speak or write a
particular foreign language easily and accurately... spoken accurately
and with facility... smoothly graceful and easy... abole to flow freely, fluid.

Wow, yeah, that's what you want, even the bit about a being able to
speak or write a particular foreign language easily and accurately.

How do we interface between Tcl and python in a way that is fluent?

Probably the first answer, to people familiar with the C interfaces
of python or tcl or people who think in particular about interpreted
languages, is to provide a way to invoke "eval", some function that
takes whatever it is given and evaluates it, in this case, evaluate
tcl code from python.

And indeed that is useful, and helpful, perhaps essential.  And we
provide that... but it doesn't get you there.  The big problem is how
data will be expressed as part of the evaluated code, and it is difficult
to ensure that the data is fully properly quoted, that some data containing
interpreter metacharacters will inadvertently trigger a malfunction in the
code or make it vulnerable to an injection attack.

So "eval" is very useful, but it's not enough.

OK, we can take it further, upon examination, by providing a way to call
one language from another, somehow specifying each argument explicitly,
where even if the arguments contain metacharacters we can ensure they
are not evaluated by the other interpreter because when invoked with explicit
arguments, the arguments are not evaluated.  This can be done both for
calling tcl from python and for calling python from tcl, as it would need
to be.

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






