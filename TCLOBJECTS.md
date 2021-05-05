
## tohil's tclobj type

tohil 2 introduces a new python data type called tclobj, aka tohil.tclobj.

It's a python-wrapped Tcl object.

It's pretty insanely powerful.

Each tclobj maintains a pointer to a Tcl object and can do things to and with that tcl object.

### creating tclobj objects

You can create an empty tclobj just like creating any other object from a class in python:

t = tclobj()

What's kind of cool is you can pass a lot of different stuff to tclobj() and it will pretty much "do the right thing."

For example you can pass tclobj() None, bools, numbers, bytes, unicode, sequences, maps, and even other tclobjs.

### .getvar() and .setvar() methods

You can also attach a tclobj to a Tcl variable or array element, or set a variable or array element from the contents of the tclobj using its getvar() and setvar() methods.

### get stuff from tclobjs as other python pbjects

Tclobjs have methods to convert the tclobj to python strings, ints, floats, bools, lists, sets, tuples, dicts, byte arrays, and, again, tclobjs.

t = tohil.tclobj()

* bool(t) - return the contents of the tclobj object as a python bool
* t.as_byte_array() - a python byte array
* t.as_dict() - a python dict
* float(t), int(t) - as a python float and int (long), respectively.
* list(t) - as a python list
* str(t) - as a python str
* tuple(t) - as a python tuple object
* tohil.tclobj(t) - as a new python tclobj object
* tohil.tcldict(t) - as a new python tcldict object

### set() and reset()

t.set() tries to covert whatever python object is passed to it and store it in the tclobj as a tcl object, while t.reset() resets the tclobj object to have an empty tcl object.

### incr()

t.incr() tries to increment the tclobj object.  If the contents of the object preclude it from being used as an integer, a TypeError exception is throwing.

t.incr takes an optional positional argument, which is the increment amount.  It can also be specified using the "incr" named argument

```
t = tohil.tclobj(0)
t.incr()
t.incr(1)
t.incr(incr=-1)
```

### tclobjs containing tcl lists

When a tclobj contains tcl lists, cool stuff comes into play.

You can get the length of the tclobj list with len(obj), while obj.lindex(i) will return the i'th element.

You can also just use `l[i]` to get the i'th element of l, although lindex supports the
to=type conversion as well.

obj.append() will append python stuff to the list stored in the tclobj.

obj.extend() will append a tcl object comprising a list, or a python list, to a list, making it flat, i.e. each element of the list is appended to obj's list.

obj.pop(x) will pop the xth element from a tcl obj comprising a list.  If no index is specified, obj.pop() removes and returns the liast item in the list.

obj.insert(i, x) will insert item x at position i.  So obj.insert(0, x) inserts at the front of the list, and a.insert(len(a), x) is equivalent to obj.append(x)

obj.clear() removes all items from the list.

Thanks to tohil's increasingly thorough tclobj object implementation and python's excellent support for such things, you can use the indexing syntax to access and even change certain elements.

```
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
```

### comparing tclobjs to each other

Tclobjs can be compared.  If equality check is requested, first their internal
tclobj pointers are compared for absolute equality.  Following that, and for all
other cases (<, <=, >, >=), their string representations are obtained and compared.
Not something you probably should rely on for complicated objects but should be
fine for simple ones.

Comparisons are really permissive, too, in what the tclobj implementation accepts from python.

It seems pretty good, but this is new stuff, so be careful and let us know how it's going.

### find out the tclobj tcl object's type and reference count

t._tcltype will tell you the tcl object type of the tcl object stored within
the tclobj.  Note that you may get nothing back even though there is some
valid thing there, say for instance a dict, but you haven't accessed it as
a dict, so it's just a string or list or some other data type until you do.

t._refcount will tell you the reference count of the tclobj's tcl object.
This isn't probably useful for production code but it is kind of cool for poking
around and trying to understand what objects are shared and how and when and stuff.

t._pyrefcount likewise will return the python reference count of the tclobj.

Note if you're poking around that sometimes you might think the reference
count is one higher than it should be, but frequently the object you just
set the value of also happens to be the tcl interpreter result (you used
the interpreter to make it).  Once the interpreter does something else and
produces a new result, your object's reference count will go down by one.

## tcldict objects

tcldicts have the same internal structure as tclobj... a python object pointing to a tcl
object.  But it is a distinct data type because it has different implementations of
sequences and maps and whatnot, to provide pythonic feel to tcl dictionaries, which can
also be hierarchies of key-value data.

You can create a tcldict object similarly to creating a tclobj.

>>> d = tohil.tcldict("a 1 b 2 c 3 d 4 e 5")

### accessing elements in a tcldict

You can access elements using normal python dict access techniques.

For instance, `d["a"]` returns 1, `d.get('a')` does the same.  With the "get" approach
you can specify a to=type to control what python type is returned.  Also you
can set the python type returned by doing `d.to = type`.

The tcldict can represent a hierarchy of dicts, so you can say stuff like

### setting values into a tcldict

Setting values will do "dict set" on a tcldict.  It takes a key and a value.
The value can be one among a number of different python objects.
Two choices are a tclobj or tcldict object, in which case tohil will do the right thing
and grab a reference to the object rather than copying it.

The key can be a
list of keys, in which case instead of working with dict as a single-level
dictionary, it will treat it as a nested tree of dictionaries, with inner
dictionaries stored as values inside outer dictionaries.

```
d[['airport', 'KHOU', 'name']] = 'Houston Hobby'
d[['airport', 'KHOU', 'lat']] = 29.6459
d[['airport', 'KHOU', 'lon']] = -95.2769
```

The td_get method will do a "dict get" on a tclobj.  It returns the object in the style requested, str by default, but to= can be specified, as in:

```
>>> x = tohil.eval("list a 1 b 2 c 3", to=tohil.tcldict)
>>> x.get('a')
'1'
>>> x.get('a',to=int)
1
```


### get()

Likewise, `get` will accept a list of keys, treating the tcl object as
a nested tree of dictionaries, with inner dictionaries stored as values
inside outer dictionaries.  It is an error to try to get a key that
doesn't exist.

t.get() supports our to=datatype technique to get the contents of the
tcldict as one of numbers different datatypes (the same ones supported
for tohil.getvar, etc.)

One kind of annoyance about tcl dicts is having to use dict exist to
traverse the hierarchy of dicts to see if something exists before
traversing it a second time to actually get it, or to try the get
and catch the error.

get() offers a second approach, where you invoke get() and specify
the optional default= argument, where you specify a value that get
will return if the requested key doesn't exist.

If a to=datatype is specified, the default value is coerced to that
datatype if possible, or an exception is raised if not.

### checking for existence

You can do the usual python `'a' in mydict` check for existence.

The thing being checked for can be a list, in order to navigate
a hierarchy of tcl dictionaries.  For example, `["airport", "KHOU"] in mydict`

### len()

len(t) returns the size of the tcl dict, or throws an error if the contents of the object can't be treated as a tcl dict.

### removing

Standard `del t[key]` python stuff.

td_remove can also accept a list of elements and in that case it will delete a hierarchy of subordinate tcl dictionaries.  In the list case, if more than one element is specified in the list, it is an error if any of the keys don't exist.

You can create new tclobjs or tcldicts as the contents of sub-parts of dictionaries and use them as dictionaries in their own right, or whatever.

Say you have a tcldict t containing a dictionary of elements, one of which, 'a', contains a dictionary of elements, one of which, 'c', contains a dictionary of elements, 'd'.

If you want a dictionary consisting of eveyrthing below c, you might do

```
x = t[['a', 'b', 'c']]
```

...or...

```
x = t.get(['a','b','c'], to=tohil.tclobj)
```

Likewise you can compose a more complicated dictionaries by attaching a dictionary to a point within another dictionary, simply by assigning a tclobj or tcldict that itself contains a dictionary.

#### iterators

You can iterate over a tcldict with normal python semantics.
For example, something like:

```
t = tohil.tcldict("a 1 b 2 c 3 d 4 e 5 f 6")
for i in t:
    print(i)
```

If you pass a to= conversion to iter, the iterator returns tuples comprising the key and the value as well, with the value converted to the to= conversion.

```
for key, value in t.iter(to=int):
    print(f"key {key} value {value}")
```

### misc stuff

You can examine the tcl reference count.

```
x = tohil.eval("list 1 2 3 4 5", to=tohil.tclobj)

x.lindex(0)

x.refcount

x.setvar("foo")

x.refcount

x.unset("foo")

x.refcount

str(x)

y = x

y.refcount
```

You can create a tclobj or tcldict from most python stuff.

a list:

```
>>> l = [1, 2, 3, 4, 5]
>>> type(l)
<class 'list'>
>>> kl = tohil.tclobj(l)
>>> str(kl)
'1 2 3 4 5'
>>> kl.llength()
5
```

a tuple:

```
>>> z = tohil.tclobj((1, 2, 3))
>>> str(z)
'1 2 3'
```

a dict:

```
>>> d = {'a': 0, 'b': 1, 'c': 2, 'd': 3}
>>> z = tohil.tclobj(d)
>>> str(z)
'a 0 b 1 c 2 d 3'
```



