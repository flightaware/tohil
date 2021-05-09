

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

### trampoline stuff

import tohil
from tohil import procster
tohil.procster.package_require("Tclx")
defs = procster.procs.probe_procs()
exec(defs)

make the wrappers import into a namespace? 

make them define methods in a class?

done, that's exactly what we do now.

if tohil.call or tohil.eval or tohil.exec get a tcl error, instead of throwing RuntimeError have them throw a new TclError object that will include the errorCode, errorInfo, error frame, etc, from tcl.

Done, python TclError class, instantiated from C with a tclobj as the argument to the innit routine.

scrape the comment headers of the procs to create the docstrings for the tohil stuff


### tohil for people with big tcl code bases who want to use python and not rewrite everything


### stuff

look at https://github.com/python/cpython/blob/master/Modules/_tkinter.c_

---

a dict, a list, a canal, panama

right now a tclobj or tcldict returns data types it has been configured to return, or a default type of string.

if it's a nest of dicts and you're pulling a chunk of dict stuff out, you want a tcldict

but if it's something terminal in the tree, you don't want a dict, you want the value

if you try to get it as a dict it'll be an error

it's considered unsafe practice to examine tcl's objects to see what data type they are, and it
is unsafe because they can have the correct format that they will be the structure of a list or
dict or whatever, but they haven't been parsed into one so their type is string or whatever.

or it could be a list if looked at as a list but still also be a dict.  dicts will parse as lists.


---

apt install autoconf pre 2.71

apt install autoconf2.64
python3-ipython

---

crap, valtype::imei is both a namespace and a proc

---

need an equivalent to ShadowDicts for scalar variables in tcl

the problem is if it's a class then you don't have any keen assignment
syntax, unlike with shadow dicts; you're going to have to invoke a function
because if you say `foo = tohil.ShadowVar("foo")` that's good but you can't
say `foo = 'bar'` to assign to it because that'll create a new foo.

you'll have to do like foo.set('bar'), which is a bit annoying.

also the class can be callable so you could get it with foo()

and it could reconigze it was called with an argument and could set from that



analysis = tohil.ShadowVar("analysis")

analysis.set(analysis() + 'foo')

wish we could say analysis += message

that's not too hard actually

we could even make tclobj's += look at the argument and if it's a string use
concatenation, although we might ought to look at that string as a number,
we do now, and just stick with that, i don't know.

we've got these tclobjs and they're pretty handy already.  maybe the trick
is to provide a way to bind a tclobj to a tcl variable so that when read it
reattaches the tclobj to the var if it doesn't exactly point at it

and after stores a Tcl_ObjSetVar2, or whatever, to keep the var synced
with the object.

analysis = tohil.tclobj(var="analysis")

we already have t.setvar() and t.getvar() to manually sync




### custom building python on linux

apt install libbz2-dev libgdbm-dev liblzma-dev uuid-dev libffi-dev

sudo apt-get install build-essential libsqlite3-dev sqlite3 bzip2 libbz2-dev zlib1g-dev libssl-dev openssl libgdbm-dev libgdbm-compat-dev liblzma-dev libreadline-dev libncursesw5-dev libffi-dev uuid-dev

./configure --prefix=/opt --enable-ipv6 --with-pydebug --with-trace-refs --with-pymalloc

make
sudo make install


----

trying to build python debug in /opt and link to it from tohil

setuptools seems to be still referencing /usr

PATH=/opt/bin:$PATH
./configure --prefix=/opt --with-python-version=3.9d --enable-symbols


-----

building

for some reason python3 may not run from what which python3 says

make sure that you run python3.9 if you have it built python3.9d in /opt/local or whatever

./configure --prefix=/opt/local --enable-ipv6 --with-pydebug --with-trace-refs --with-pymalloc

configure tohil with like

./configure --with-python-version=3.9d --prefix=/opt/local

when you install, use

python3.9 setup.py install --prefix=/opt/local

-----

building docs

pip3 install asdl was needed for docs

pip install python_docs_theme

sphinx-build .  out

-----

say you are doing 'make html' repeatedly in the Doc dir and want to view,
you can start a dead simple webserver from the top level of your html dir
or whatever, using something like:

python3 -m http.server


