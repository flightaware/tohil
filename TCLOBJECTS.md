
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

obj.lappend_list() will append a tcl object comprising a list, or a python list, to a list, making it flat, i.e. each element of the list is appended to obj's list.

Thanks to tohil's increasingly thorough tclobj object implementation and python's excellent support for such things, you can use the indexing syntax to access and even change certain elements.

```
>>> x = tohil.eval("list 1 2 3 4 5 6", to=tohil.tclobj)
>>> x
<tohil.tclobj: '1 2 3 4 5 6'>
>>> x[2]
<tohil.tclobj: '3'>
>>> x[3:]
[<tohil.tclobj: '4'>, <tohil.tclobj: '5'>, <tohil.tclobj: '6'>]
>>> x[-2:]
[<tohil.tclobj: '5'>, <tohil.tclobj: '6'>]
>>> x[-2:-1]
[<tohil.tclobj: '5'>]
```


Tclobjs can be compared.  If equality check is requested, first their internal
tclobj pointers are compared for absolute equality.  Following that, and for all
other cases (<, <=, >, >=), their string representations are obtained and compared.
Not something you probably should rely on for complicated objects but should be
fine for simple ones.

Comparisons are really permissive, too, in what the tclobj implementation accepts from python.

It seems pretty good, but this is new stuff, so be careful and let us know how it's going.

### tcl dict access to tclobj objects

To avoid confusion with python dicts, we are calling tcl dicts td's.

The td_set method will do dict set on a tclobj.  It takes a key and a value.  The value can be a tclobj object in which case tohil will do the right thing and grab a reference to the object rather than copying it.  The key can be a list of keys, in which case instead of working with dict as a single-level dictionary, it will treat it as a nested tree of dictionaries, with inner dictionaries stored as values inside outer dictionaries.

```
x.td_set('a',1)
x.td_set(['a','b','c','d'],'bar')
```

The td_get method will do a dict get on a tclobj.  It returns the object in the style requested, str by default, but to= can be specified, as in:

```
>>> x = tohil.eval("list a 1 b 2 c 3", to=tohil.tclobj)
>>> x.td_get('a')
'1'
>>> x.td_get('a',to=int)
1
```

Likewise, td_get will accept a list of keys, treating the tcl object as a nested tree of dictionaries, with inner dictionaries stored as values inside outer dictionaries.  It is an error to try to get a key that doesn't exist.

td_exists can be used to see if a key exists, and also accepts a list of keys to access a hierarchy.

td_size() returns the size of the dict or throws an error if the contents of the object can't be treated as a tcl dict.

x.td_remove() removes an element from the dict.  It's not an error to remove something that doesn't exist.

td_remove can also accept a list of elements and in that case it will delete a hierarchy of subordinate namespaces.  In the list case, if more than one element is specified in the list, it is an error if any of the keys don't exist.

You can create new tclobjs as the contents of sub-parts of dictionaries and use them as dictionaries, or whatever.

Say you have a tclobj t containing a dictionary of elements, one of which, 'a', contains a dictionary of elements, one of which, 'c', contains a dictionary of elements, 'd'.

If you want a dictionary consisting of eveyrthing below c, you might do

```
x = t.td_get(['a','b','c'], to=tohil.tclobj)
```

Likewise you can compose a more complicated dictionaries by attaching a dictionary to a point within another dictionary, simply by doing a td_set with the value being a tclobj that itself contains a dictionary.

#### TD iterators

You can iterate over a TD with td_iter. For example, something like:

```
t = tohil.tclobj("a 1 b 2 c 3 d 4 e 5 f 6")
for i in t.td_iter():
    print(i)
```

If you pass a to= conversion to td_iter, the iterator returns tuples comprising the key and the value as well, with the value converted to the to= conversion.

```
for key, value in t.td_iter(to=int):
    print(f"key {key} value {value}")
```

### misc stuff

You can examine the tcl reference count.

```
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
```

You can create a tclobj from most python stuff.

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



