
.. _tutorial-tclobjs:

********************************
Tohil's Tclobj Python Data Type
********************************

To provide powerful and facile interactions between Python and Tcl,
Tohil provides a new Python data type, the Tcl object, or *tclobj*, aka
*tohil.tclobj*.

It's a Python-wrapped Tcl object.

It's pretty insanely powerful.

See, Tcl has these objects, fairly similar to Python at the C level, that
it uses throughout.

They can be a string or an int or a float or a list or a nested dictionary
or a bunch of other things.

Each tclobj maintains a pointer to a Tcl object and can do things to and
with that Tcl object.

####################################
Creating Tclobj Objects From Python
####################################

You can create an empty tclobj just like creating any other object from
a class in Python:

::

   t = tclobj()

Something pretty cool is you can pass many different Python data types,
including lists and dicts, to tohil.tclobj, and it will
and it will pretty much "do the right thing," i.e. it will produce
something expected and straightforward that Tcl can make sense of.

For example you can pass tclobj() None, bools, numbers, bytes, unicode,
sequences, maps, and even other tclobjs.

########################################
.getvar() and .setvar() methods
########################################

You can also attach a tclobj to a Tcl variable or array element, or set
a variable or array element from the contents of the tclobj using its
*getvar()* and *setvar()* methods.

##################################################
Get stuff from tclobjs as Other Python Objects
##################################################

Tclobjs have methods to convert the tclobj to Python strings, ints, floats,
bools, lists, sets, tuples, dicts, byte arrays, and, again, tclobjs.

* *bool(t)* - Return the contents of the tclobj object as a Python bool
* *float(t)*, *int(t)* - as a python float and int (long), respectively.
* *list(t)* - as a python list
* *str(t)* - as a python str
* *tuple(t)* - as a python tuple object
* *tohil.tclobj(t)* - as a new python tclobj object
* *tohil.tcldict(t)* - as a new python tcldict object
* *t.as_byte_array()* - a Python byte array
* *t.as_dict()* - a Python dict

#########################
set() and reset()
#########################

*t.set()* tries to covert whatever Python object is passed to it and store
it in the tclobj as a Tcl object, while *t.reset()* resets the tclobj object
to contain an empty Tcl object.

###############
incr()
###############

*t.incr()* tries to increment the tclobj object.  If the contents of the object
preclude it from being used as an integer, a TypeError exception is thrown.

t.incr takes an optional positional argument, which is the increment amount.
It can also be specified using the "incr" named argument.

::

    t = tohil.tclobj(0)
    t.incr()
    t.incr(1)
    t.incr(incr=-1)


#############################
Tclobjs Containing Tcl lists
#############################

When a tclobj contains Tcl lists, cool stuff comes into play.

Accessing a tclobj containing a list from Python is nearly identical
to accessing a standard Python list.  You can access and change elements,
use slice notation, etc, and most standard Python list methods are provided
as well.

You can get the length of the tclobj list with *len(obj)*, while
*obj.lindex(i)* will return the *i*'th element.

You can also just use ``l[i]`` to get the i'th element of *l*, although
the *lindex* supports Tohil's *to=type* conversion as well.

*obj.append()* will append to the list stored in the tclobj.

*obj.extend()* will append a Tcl object comprising a list, or a Python list,
to a list, making it flat, i.e. each element of the list is appended to obj's
list.

*obj.pop(x)* will pop the *x*'th element from a tcl obj comprising a list.
If no index is specified, obj.pop() removes and returns the last item in
the list.

*obj.insert(i, x)* will insert item *x* at position *i*.
So as with Python lists, ``obj.insert(0, x)`` inserts at the front of the
list, and ``a.insert(len(a), x)`` is equivalent to ``obj.append(x)``

*obj.clear()* removes all items from the list.

You can use Python's indexing syntax to access and replace list elements.

::

    >>> x = tohil.eval("list 1 2 3 4 5 6", to=tohil.tclobj)
    >>> x
    <tohil.tclobj: '1 2 3 4 5 6'>
    >>> x[2]
    '3'
    >>> x[3:]
    ['4', '5', '6']
    >>> x[-2:]
    ['5', '6']
    >>> x[-2:-1]
    ['5']

####################################
Comparing Tclobjs wo Each Other
####################################

Tclobjs can be compared.  If equality check is requested, first their
internal tclobj pointers are compared for absolute equality.  Following that,
and for all other cases (<, <=, >, >=), their string representations are
obtained and compared.

Not something you probably should rely on for complicated objects but should
be fine for simple ones.

Comparisons are really permissive, too, in what the tclobj implementation
accepts from Python.

It seems pretty good, but this is new stuff, so be careful and let us know
how it's going.

#############################################
Get the tclobj's Tcl Type and Reference Count
#############################################

*t._tcltype* will tell you the tcl object type of the tcl object stored
within the tclobj.  Note that you may get nothing back even though there
is some valid thing there, say for instance a dict, but you haven't
accessed it as a dict, so it's just a string or list or some other data
type until you do.

*t._refcount* will tell you the reference count of the tclobj's tcl object.
This isn't probably useful for production code but it is kind of cool for poking
around and trying to understand what objects are shared and how and when
and stuff.

*t._pyrefcount* likewise will return the python reference count of the tclobj.

Note that if you're poking around, that sometimes you might think the reference
count is one higher than it should be, but frequently the object you just
set the value of also happens to be the Tcl interpreter result (i.e. you used
the interpreter to make it).  Once the interpreter does something else and
produces a new result, your object's reference count will go down by one.

If this doesn't make sense, don't worry about it.  You probably don't need
it and don't care anyway.

You can create a tclobj from most Python stuff.

...a list:

::


    >>> l = [1, 2, 3, 4, 5]
    >>> type(l)
    <class 'list'>
    >>> kl = tohil.tclobj(l)
    >>> str(kl)
    '1 2 3 4 5'
    >>> kl.llength()
    5

...a tuple:

::


    >>> z = tohil.tclobj((1, 2, 3))
    >>> str(z)
    '1 2 3'

...a dict:

::

    >>> d = {'a': 0, 'b': 1, 'c': 2, 'd': 3}
    >>> z = tohil.tclobj(d)
    >>> str(z)
    'a 0 b 1 c 2 d 3'



