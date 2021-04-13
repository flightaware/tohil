

The use case is you have a lot of valuable Tcl code that works that you're not crazy enough to try to get rid of, but you want to be able to use python for web stuff and/or backend stuff, get access to python and all the libraries available for it, the benefits of the much larger user community, greater acceptance among the developers (possibly your employees) and the larger talent pool, etc.

It's not going to work to rewrite.  You can't.  THere are too many customers that are already using what you've got, and there are bug fixes and improvements to that.  You're going to retask some considerably proportion of your staff to rewrite years to bazillions of years of work, almost for sure you're going to go out of business.  You're going to go out of business bdcause your rewrite is going to fail.  Your rewrite will regress, recreate tons of bugs that were already fixed in the old stuff, cause trouble tickets, maintenance, tl;dr you're not going to like it.

So I have a ton of Tcl code and I want to get to python.  What's it going to take to do that?

We're gonna need to make Tcl and python fluent with each other.

What does fluent mean?  From the MacOS dictionary, whatever that is from, fluent - adjective - ...able to express oneself easily and articulately... able to speak or write a particular foreign language easily and accurately... spoken accurately and with facility... smoothly graceful and easy... abole to flow freely, fluid.

Wow, yeah, that's what you want, even the bit about a being able to speak or write a particular foreign language easily and accurately.

How do we interface between Tcl and python in a way that is fluent?

Probably the first answer, to people familiar with the C interfaces of python or tcl or people who think about in particular interpreted languages, is to provide a way to invoke "eval", some function that takes whatever it is given and evaluates it, in this case, evaluate tcl code from python.

And indeed that is useful, and helpful, perhaps essential.  And we provide that... but it doesn't get you there.  The big problem is how data will be expressed as part of the evaluated code, and it is difficult to ensure that the data is fully properly quoted, that some data containing interpreter metacharacters will inadvertently trigger a malfunction in the code or mae it vulnerable to an injection attack.

We don't want that.

OK, we can take it further, upon examination, by providing a way to call one language from another with a more explicit set of arguents, where even if the arguments contain metacharacters we can ensure they are not evaluated.

And we provide that.  And it's really handy.  But we wouldn't call that fluent.

Fluent is where the tcl commands look and act like python commands as much as possible.

WHEN we import namespaces should we also import variables and arrays!?!?

could it see the non-array namespace variables as a dict of variables associated with the namespace






