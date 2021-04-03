
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

The td_get method will do a dict get on a tclobj.  It returns the object in the style requested, str by default, but to= can be specified, as in:

```
>>> x = tohil.eval("list a 1 b 2 c 3", to=tohil.tclobj)
>>> x.td_get('a')
'1'
>>> x.td_get('a',to=int)
1
```


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



