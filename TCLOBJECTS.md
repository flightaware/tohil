
## tohil's tclobj type

tohil 2 introduces a new python type called tclobj

It's a python-wrapped Tcl object.

It's pretty insanely powerful.


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


