

you have to build without stubs to create a .so/.dylib that python likes

turn off stubs in configure.in

change -ltclstub86 to -ltcl86 in Makefile -- this needs to be done properly

"python3 setup.tcl build" builds a shared library but it's no good

"sudo python3 setup.tcl install" installs it and the tooling for "import tclpy" to work.

but if you copy the shared library that make makes into the place that the setup.py installer thing uses, import tclpy works from python3

something like (on the mac, using pyenv)

sudo cp libtclpy0.5.0.dylib ~/.pyenv/versions/3.8.2/lib/python3.8/site-packages/tclpy.cpython-38-darwin.so

cp libtclpy0.5.0.dylib build/lib.macosx-10.6-x86_64-3.8/tclpy.cpython-38-darwin.so



from python:


>>> import tclpy
>>> tclpy.eval('set a(99) goof')
'goof'
>>> tclpy.eval('set a(5) foo')
'foo'
>>> tclpy.getvar('a','99')
'goof'
>>> tclpy.getvar(array='a',var='5')
'foo'
>>> tclpy.getvar(array='a',var='16')
>>>


new subst method

>>> import tclpy
>>> tclpy.eval("set name karl")
'karl'
>>> tclpy.subst("hello, $name")
'hello, karl'



>>> tclpy.expr('5+5')
'10'
>>> tclpy.expr('5**5')
'3125'
>>> tclpy.expr('1/3')
'0'
>>> tclpy.expr('1/3.')
1
>>> tclpy.expr('[clock seconds] % 86400')
'25571'


>>> tclpy.eval('set a "a 1 b 2 c 3"')
'a 1 b 2 c 3'
>>> tclpy.subst("$a")
'a 1 b 2 c 3'
>>> tclpy.eval('return $a')
'a 1 b 2 c 3'
>>> tclpy.megaval('return $a','list')
['a', '1', 'b', '2', 'c', '3']
>>> tclpy.megaval('return $a','dict')
{'a': '1', 'b': '2', 'c': '3'}


>>> tclpy.megaval(to="list",tcl_code="return [list 1 2 3 4]")
['1', '2', '3', '4']

check this out, converting expected results to python datatypes:

>>> import tclpy
>>> tclpy.megaval("clock seconds")
'1616053828'
>>> tclpy.megaval("clock seconds",to="int")
1616053834
>>> tclpy.megaval("clock seconds",to="float")
1616053838.0
>>> tclpy.megaval("clock seconds",to="bool")
True
>>> tclpy.megaval("clock seconds",to="list")
['1616053849']

don't forget you're doing

    python3 setup.py build
    sudo python3 setup.py install

when building and installing the python module

now megaval with to='set' option to return a set from a list

>>> tclpy.megaval('return [list 1 2 3 4 4 3]',to='set')
{'3', '4', '2', '1'}

TODO

make tclpy.expr able to do the to= stuff that tclpy.megaval can do.

get rid of tclpy.eval and rename tclpy.megaval to tclpy.eval

intercept stdout when exec'ing python in rivet and pump it to rivet





### what wizardry is this

how about a python object that wraps a tcl object and can do things with it

it would need to be implemented in C

it would be able to get the object as a string, an int, a float, a list, etc, if it can be represented that way.

for example if you try to do list stuff with it it'll do a Tcl_GetListFromObj and if it returns an error it'll throw an exception.



### tcl dict walk

tcl dict walks are done with Tcl_DictObjFirst, Tcl_DictObjNext and Tcl_DictObjDone.

when we implement iterators on dicts, we'll use those in the iterator function that we return for the iterator.

<<<<<<< Updated upstream
### ideas

look at tcl errorCode in tohil_python_return to set the python error code

hmm pass the tcl error code as part of an exception object, as a python list

maybe make a specific tcldict object so it can have all the tricked out iter semantics and stuff.  can it inherit from the tclobj object?  i bet it can

make a .td_lappend and .td_incr that don't suck.

make a .incr

maybe have a shadow variable as well as shadow arrays, a way for tclobj to shadow a variable or an array element, or maybe a subclass, the tclobj is fetched from the variable or element at the start of the operation and stored at the end

this solves the sort of surprising behavior of tclobjs that when you get from a var and change it, it doesn't change the var

### templates?

possibly inspect tcl procs using "info args" and "info default" to figure out what arguments they expect and generate a python trampoline that lets you invoke with python style named arguments, etc, 


### tohil for people with big tcl code bases who want to use python and not rewrite everything


### stuff

look at https://github.com/python/cpython/blob/master/Modules/_tkinter.c



