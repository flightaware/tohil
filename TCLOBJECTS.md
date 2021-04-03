
## tohil's tclobj type

tohil 2 introduces a new python data type called tclobj, aka tohil.tclobj.

It's a python-wrapped Tcl object.

It's pretty insanely powerful.

Each tclobj maintains a pointer to a Tcl object and can do things to and with that tcl object.

You can create an empty tclobj just like creating any other object from a class in python:

t = tclobj()

What's kind of cool is you can pass a lot of different stuff to tclobj() and it will pretty much "do the right thing."

For example you can pass tclobj() None, bools, numbers, bytes, unicode, sequences, maps, and even other tclobjs.

You can also attach a tclobj to a Tcl variable or array element, or set a variable or array element from the contents of the tclobj using its getvar() and setvar() methods.

Tclobjs have methods to convert the tclobj to python strings, ints, floats, bools, lists, sets, tuples, dicts, byte arrays, and, again, tclobjs.

When a tclobj contains tcl lists, cool stuff comes into play.

You can get the length of the list with obj.llength(), while obj.lindex(i) will return the i'th element.

obj.lappend() will append python stuff to the list stored in the tclobj.

Thanks to tohil's increasingly thorough tclobj object implementation and python's excellent support for such things, you can use the indexing syntax to access and even change certain elements.







x = tohil.eval("list 1 2 3 4 5", to=tohil.tclobj)

x.lindex(0)

x.refcount()

x.setvar("foo")

x.refcount()

x.unset("foo")

x.refcount()

str(x)

y = x

y.refcount()

You can create a tclobj from most python stuff.

a list:

>>> l = [1, 2, 3, 4, 5]
>>> type(l)
<class 'list'>
>>> kl = tohil.tclobj(l)
>>> str(kl)
'1 2 3 4 5'
>>> kl.llength()
5

a tuple:

>>> z = tohil.tclobj((1, 2, 3))
>>> str(z)
'1 2 3'

a dict:

>>> d = {'a': 0, 'b': 1, 'c': 2, 'd': 3}
>>> z = tohil.tclobj(d)
>>> str(z)
'a 0 b 1 c 2 d 3'


