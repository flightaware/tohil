.. _tutorial-shadow-dictionaries:


*******************
Shadow Dictionaries
*******************

Shadow Dictionaries, aka ShadowDicts, create a Python dict-like object that
shadows a Tcl array.

Tcl arrays are basically the Tcl equivalent of Python's dicts, by the way.

Let's assume we have an array *x* in Tcl that we want to shadow as
a dictionary *x* in Python, we would write ``x = tohil.ShadowDict("x")``.

If you just specify a variable name without any namespace qualifiers, the
array references the current Tcl execution frame, like if a Tcl proc had
called Python and in our Python we did the x equals thing for a shadow dict
then the x array would exist in the proc's frame.  In other words, the
array is local to the caller on the Tcl side.

If we're invoking it not from Tcl code called from Python, just from Python
or the top level of Python or whatever, then x is in the global ("::")
namespace.  You can always provide namespace qualifiers to identify the
global or some subordinate namespace, like "::cryptolib::x"

Once created, shadowdict elements can be gotten as a string using *str()*
or *print()*, etc.

Elements can be read form the Python side using dictionary notation,
for example `x['d']`, set in a standard way (`x['e'] = '5'`), and
deleted in a standard way using *del* (`del x['e']`).  Also you can
iterate on the keys as with dicts.

Changes made from the Python side occur on the Tcl side, and all accesses,
traversals, etc, are made using the actual Tcl array.  In other words,
ShadowDicts never cache values from the Tcl array on the Python side.

In the example below we set up a Tcl array, create a ShadowDict of it
in Python, get a string representation of the dict, read from the dict,
insert into it, delete from it, and demonstrate that the changes we made
are present on the Tcl side.  Finally, it iterates over the shadow dict,
showing the same keys from Python that Tcl was shown to have.

::

   >>> tohil.eval("array set x [list a 1 b 2 c 3 d 4]")
   <tohil.tclobj: ''>
   >>> x = tohil.ShadowDict("x", to=int)
   >>> x
   {'d': '4', 'e': '5', 'a': '1', 'b': '2', 'c': '3'}
   >>> x['d']
   4
   >>> x['e'] = '5'
   >>> x['e']
   5
   >>> del x['d']
   >>> tohil.eval("parray x")
   x(a) = 1
   x(b) = 2
   x(c) = 3
   x(e) = 5
   <tohil.tclobj: ''>
   >>> for i in x:
   ...     print(i)
   ...
   a
   b
   c
   e

ShadowDict support many of the capabilties of regular python dicts.
For example,
*len(x)* will return the length of the shadow dict i.e. the size of the
shadowed Tcl array.

*x.keys()* return the keys, *x.values()* returns the values,
and *x.items()* returns the
keys and items as a list of two-element tuples.  However, unlike
regular Python dicts, they are not mutable, i.e. if you have captured
a reference to x.keys() the contents of x.keys() does not change when
the corresponding dict is changed.

*x.get(key)* will return the element of the array indexed by key if it
exists, else it will raise a *KeyError* exception.  However if a named
parameter, *default*, is specified with a value, in the event key is
not found in x, the default value will be returned instead.

Finally the *to=* named parameter can be used to specify a Python return
type such as *list*, *set*, *dict*, *int*, *float*, *str*,
*tohil.tclobj*, *tohil.tcldict*, etc.

*x.pop(key)*, if *key* is in the shadow dictionary, removes it and returns
it.  A default value can be specified as an optional second argument.
If a default is not specified and the key is not in the dictionary,
a *KeyError* exception is raised.  As with so many other functions, the
to= named parameter can be specified to state what data type you
want the data returned to Python as.


