
.. _tutorial-tcldicts:

********************************
Tohil's Tcldict Python Data Type
********************************

*Tcldicts* are Python-wrapped Tcl dictionaries.

While they have the same internal structure as tclobjs (a Python object
pointing to a Tcl object), tcldicts are a distinct data type in Python
because tcldicts have different implementations of
sequences and maps and whatnot, to provide a Pythonic feel to Tcl
dictionaries, which can also be hierarchies of key-value data.

You can create a tcldict object similarly to creating a tclobj:

::

    >>> d = tohil.tcldict("a 1 b 2 c 3 d 4 e 5")

####################################
Accessing Elements in a Tcldict
####################################

You can access elements using normal Python dict access techniques.

For instance, *d["a"]* returns 1, *d.get('a')* does the same.
With the "get" approach you can specify a *to=type* to control what Python
type is returned.  Also you can set the Python type returned by
doing ``d.to = type``.

##############################
Setting Values into a Tcldict
##############################

Setting values will do a Tcl *dict set* on a tcldict.
It takes a key and a value.
The value can be one among a number of different Python objects.
Two options are to pass a tclobj or tcldict object, in which case tohil
will do the right thing and grab a reference to the object rather than
copying it.

The key can be a
list of keys, in which case instead of working with dict as a single-level
dictionary, it will treat it as a nested tree of dictionaries, with inner
dictionaries stored as values inside outer dictionaries.

```
d[['airport', 'KHOU', 'name']] = 'Houston Hobby'
d[['airport', 'KHOU', 'lat']] = 29.6459
d[['airport', 'KHOU', 'lon']] = -95.2769
```

Standard Python dicts can't do this.

The *td_get* method will also do a *dict get* on a tclobj.
It returns the object in the style requested, tclobj by default, but *to=*
can be specified, as in:

::

    >>> x = tohil.eval("list a 1 b 2 c 3", to=tohil.tcldict)
    >>> x.get('a')
    <tohil.tclobj: '1'>
    >>> x.get('a',to=int)
    1


###################
get()
###################

Likewise, *get* will accept a list of keys, treating the Tcl object as
a nested tree of dictionaries, with inner dictionaries stored as values
inside outer dictionaries.  It is an error to try to get a key that
doesn't exist.

*t.get()* supports our *to=datatype* technique to get the contents of the
tcldict as one of a number of different datatypes (the same ones supported
for tohil.getvar, etc.)

One kind of annoyance about Tcl dicts is having to use dict exist to
traverse the hierarchy of dicts to see if something exists before
traversing it a second time to actually get it, or to try the get
and catch the error.

*get()* offers a nice alternative approach, where you invoke it
and specify
the optional *default=* argument, where you can specify a value that *get*
will return if the requested key doesn't exist.

If a *to=datatype* is specified and the default value is used, Tohil
will coerve the default value to the *to*-specified datatype, if
possible, or an exception is raised if not.

############################
Checking for Existence
############################

You can do the usual Python ``'a' in mydict`` check for existence.

The thing being checked for can be a list, in order to navigate
a hierarchy of Tcl dictionaries.
For example, ``["airport", "KHOU"] in mydict``

##########
len()
##########

*len(t)* returns the size of the Tcl dict, or throws an error if the contents
of the object can't be treated as a Tcl dict.

##################
Removing Elements
##################

Standard ``del t[key]`` Python stuff.

*td_remove* can also accept a list of elements and in that case it will
delete a hierarchy of subordinate Tcl dictionaries.  In the list case,
if more than one element is specified in the list, it is an error if
any of the keys don't exist.

#############################################
Assembling Tcldicts from Tcldicts and Tclobjs
#############################################

You can create new tclobjs or tcldicts as the contents of sub-parts of
dictionaries and use them as dictionaries in their own right, or whatever.

Say you have a tcldict *t* containing a dictionary of elements, one of which,
*a*, contains a dictionary of elements, one of which, *c*, contains a
dictionary of elements, *d*.

If you want a dictionary consisting of eveyrthing below *c*, you might do

::

    x = t[['a', 'b', 'c']]

...or...

::


    x = t.get(['a','b','c'], to=tohil.tclobj)

Likewise you can compose more complicated dictionaries by attaching a
dictionary to a point within another dictionary, simply by assigning
a tclobj or tcldict that itself contains a dictionary.

###############
iterators
###############

You can iterate over a tcldict with normal Python semantics.

For example, something like:

::

    >>> t = tohil.tcldict("a 1 b 2 c 3 d 4 e 5 f 6")
    >>> for i in t:
    ...    print(i)

If you pass a *to=* conversion to iter, the iterator returns tuples comprising
the key and the value as well, with the value converted to the *to=*
conversion.

::

    for key, value in t.iter(to=int):
        print(f"key {key} value {value}")

