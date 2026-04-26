# Tohil

[![Linux CI](https://github.com/flightaware/tohil/actions/workflows/linux-ci.yml/badge.svg)](https://github.com/flightaware/tohil/actions/workflows/linux-ci.yml)

<img src="https://github.com/flightaware/tohil/blob/main/graphics/237px-Quetzalcoatl_feathered_serpent.png">

Tohil a feathered serpent, aims to provide a delightful integration between Python, the serpent, and TCL, the feather.

Tohil is simultaneously a Python extension and a TCL extension that makes it possible to effortlessly call bidirectionally between Tcl and Python, targeting Tcl 8.6+ and Python 3.6+

Tohil is open source software, available for free including for profit and/or for redistribution, under the permissive 3-clause BSD license (see "LICENSE.txt").

Tohil is pronounced as, your choice, toe-heel, or toe-hill.

Tohil has a growing body of documentation, including a tutorial
and reference, available at https://flightaware.github.io/tohil-docs/.

## Usage

You can import Tohil into either a Tcl or Python parent interpreter. Doing so will create and initialize an interpreter for the corresponding language and define Tohil's functions in both.

Using Tohil, Python code can call Tcl code at any time, and vice versa, and they can call "through" each other, i.e. Python can call Tcl code that calls Python code that calls Tcl code, and so on.

### Accessing Tcl From Python

To use Python to do things in Tcl, you invoke functions defined by the Tohil module that gets created when you import Tohil into your Python interpreter.

Tohil:

* ...provides several routines to evaluate Tcl code, passing it data using common and familiar Python objects such as strs, bools, ints, floats, lists, dicts, tuples, etc, and producing those types from Tcl results as well.
* ...defines a new Python data type, [tohil.tclobj](https://flightaware.github.io/tohil-docs/tutorial/tohil_tclobjs.html), that provides direct and efficient manipulation of Tcl, well, strings, of course, but strings containing ints, floats, lists, dicts, etc, passing them around, using them as arguments in calls to Tcl functions, and receiving them from function results as well.
* ...creates shadow dictionaries, a Python dictionary-type object that accesses and manipulates Tcl arrays as Python dictionaries.
* ...provides a [TclProc class](https://flightaware.github.io/tohil-docs/tutorial/tohil_tclprocs.html) that creates callable Python object-functions that will call their corresponding Tcl procs and C commands and return the results to Python, optionally with a specified Python type that the returned data should be converted to.
* ...provides a TclNamespace class that has the ability to import all the Tcl procs and C commands found there as methods of the namespace class, and recursively descend child namespaces, creating new TclNamespaces objects, binding them to their parent objects, and importing all the procs found within them as well.  See also the Tohil 3 [release notes](https://flightaware.github.io/tohil-docs/whatsnew/3.0.html).

Here's a simple example of using Tohil to get Tcl to format a Unix "epoch" clock
into a standard Posix time and date string, in the French locale:

```python
>>> import tohil
>>> clock = 1616182348
>>> tohil.eval(f"clock format {clock} -locale es -gmt 1", to=str)
'vie mar 19 19:32:28 GMT 2021'
```

The optional _to_ named parameter allows you to specify one of a number of data types, or a processing function, that will cause Tohil to convert the return into a native Python data type.

The types supported are str, int, bool, float, list, set, dict, tuple, tohil.tclobj, and tohil.tcldict.

By default the results of the Tcl code evaluated (if there wasn't an exception) is returned to the caller, as a tclobj.

Uncaught Tcl errors tracing back all the way to the the Tohil membrane are raised as a Python exception.

```python
>>> tohil.eval('set a [list a 1 b 2 c 3]')
'a 1 b 2 c 3'
>>> tohil.eval('return $a', to=list)
['a', '1', 'b', '2', 'c', '3']
>>> tohil.eval('return $a',to=dict)
{'a': '1', 'b': '2', 'c': '3'}

>>> a, b, c = tohil.eval("list 1 2 3", to=tuple)
>>> c
'3'
```

Besides tohil.eval, Tohil provides Python functions to call Tcl functions without argument evaluation (_tohil.call_), to get and set and manipulate Tcl variables and array
elements (_tohil.getvar_, _tohil.setvar_, _tohil.exists_, _tohil.unset_), and a few others.

For a complete tutorial on using Tcl from Python, please visit [The Tohil Tutorial](https://flightaware.github.io/tohil-docs/tutorial/tohil_python.html).

#### Tcl objects

Tohil 2 introduced a new Python data type called tclobj, aka tohil.tclobj.

It's a Python-wrapped Tcl object and it's very useful for generating and manipulating, passing to and receiving from, Tcl routines, Tcl lists, .  See [TCLOBJECTS.md](https://flightaware.github.io/tohil-docs/tutorial/tohil_tclobjs.html) for more.

#### Shadow Dictionaries

Shadow Dictionaries, aka ShadowDicts, create a Python dict-like object that shadows a Tcl array, meaning that any changes to the dictionary from the Python side are immediately visible to Tcl and vice versa.

For more info please visit https://flightaware.github.io/tohil-docs/tutorial/shadow_dicts.html

#### Examples using Tohil from Python

```python
>>> import tohil
>>> a = tohil.eval('list 1 [list 2 4 5] 3')
>>> print(a)
1 {2 4 5} 3

>>> import tohil
>>> tohil.eval('set a(99) goof')
'goof'
>>> tohil.eval('set a(5) foo')
'foo'
>>> tohil.getvar('a','99')
'goof'
>>> tohil.getvar(array='a',var='5')
'foo'
>>> tohil.getvar(array='a',var='16')


>>> tohil.eval('set a [list a 1 b 2 c 3]')
'a 1 b 2 c 3'
>>> tohil.subst("$a")
'a 1 b 2 c 3'
>>> tohil.eval('return $a')
'a 1 b 2 c 3'
>>> tohil.eval('return $a',to=list)
['a', '1', 'b', '2', 'c', '3']
>>> tohil.eval('return $a',to=dict)
{'a': '1', 'b': '2', 'c': '3'}

>>> tohil.eval(to=list,tcl_code="return [list 1 2 3 4]")
['1', '2', '3', '4']

```

Check this out, converting expected results to Python datatypes:

```python
>>> import tohil
>>> tohil.eval("clock seconds")
'1616053828'
>>> tohil.eval("clock seconds",to=int)
1616053834
>>> tohil.eval("clock seconds",to=float)
1616053838.0
>>> tohil.eval("clock seconds",to=bool)
True
>>> tohil.eval("clock seconds",to=list)
['1616053849']
```

Now eval with to=set option to return a set from a list...

```python
>>> tohil.eval('return [list 1 2 3 4 4 3]',to=set)
{'3', '4', '2', '1'}
```

### Accessing Python From Tcl

From Tcl, Tohil provides access to Python through several commands and procs.

Probably the most important commands are `tohil::eval`, `tohil::exec` and `tohil::call`.  The first two commands correspond closely to Python's `eval` and `exec`.

```tcl
package require tohil
```

Tohil provides commands for interacting with the Python interpreter, via the ::tohil namespace.

Check out the part of the tutorial about [Using Python From Tcl](https://flightaware.github.io/tohil-docs/tutorial/tohil_tcl.html), and the Tohil reference on [Tohil Tcl Functions](https://flightaware.github.io/tohil-docs/reference/tohil_tcl_functions.html)

### Using Tohil from Rivet

From a Rivet page, in some Tcl code body, invoke `package require tohil`.

If you run _tohil_rivet_ it will plug Tohil's Python interpreter such that everything Python writes to stdout using print, or whatever, will go through Tcl's stdout and thereby into your Rivet page.

```
<?

package require tohil; tohil_rivet

puts "calling out to Python to add 5 + 5: [::tohil::eval "5 + 5"]"

tohil::exec {
print('hello, world')
print("<hr>")
}

?>
```

###  Building Tohil on Unix, Linux, FreeBSD and the Mac

Tohil builds with the familiar GNU autoconf build system.  You will need to use the older version, autoconf 2.69.  "autoreconf" will produce a configure script based on the configure.in.  The tooling used is the standard Tcl Extension Architecture (TEA) approach, which is pretty evolved and fairly clean considering it's autoconf.

It is assumed that you
 - have got the Tohil repo (either by `git clone` or a tar.gz from the releases page).

The build process is fairly simple:
 - run the configure script
 - make
 - sudo make install

We're using setuptools to build the Python module, so the Makefile.in/Makefile is basically doing

```
Python3 setup.py build
Python3 setup.py install
```

...to build and install the Python module.

Chceck out the docs on [installing Tohil](https://flightaware.github.io/tohil-docs/installing/index.html).

Also there are README files for Linux, FreeBSD and macOS.

### Tests

Run the tests with

	$ make test

There are currently about 165 tests, but most tests perform many tests, so several hundred.  The Python _hypothesis_ testing framework is used in many cases, and is highly recommended and it helped us identify several tohil bugs that the more typical naive tests would not have found.

### Gotchas

1. Be very careful when putting unicode characters into a inside a `tohil.eval`
or `tohil.exec` call - they are decoded by the Tcl parser and passed as literal bytes
to the Python interpreter. So if we directly have the character "ಠ", it is
decoded to a utf-8 byte sequence and becomes u"\xe0\xb2\xa0" (where the \xXY are
literal bytes) as seen by the Python interpreter.
2. Escape sequences (e.g. `\x00`) inside py eval may be interpreted by Tcl - use
{} quoting to avoid this.

You need to build the library without stubs for Python to be able to use it.

### To Do

* if Python is the parent, register a Tcl panic handler and invoke Py_FatalError if Tcl panics.
* the reverse of the above if Tcl is the parent if Python has a panic-type function with a registerable callback

We'd like to have Tohil work on Windows if someone would be willing to take it on.

### Notes

Gerald Lester's [pyman](http://chiselapp.com/user/gwlester/repository/pyman/home) is a Tcl package that provides a higher level of abstraction on top of tclpy.  It will need to be updated for Tohil but bears examination.

### Geek Notes

The same Tohil shared library created by building this software originally
could be loaded both by Python and Tcl, which was pretty cool.  Due to
global data kept in the library It used to be necessary so that Tohil could
work at all.

However since there are different
build pipelines for Tcl extensions (based on autoconf via the Tcl extension
architecture) and Python (based on Python setuptools), we
changed Tohil's implementation to be able to work ok even with two different
shared libraries by moving the critical piece of shared data, the Tcl
interpreter pointer, formerly held statically by the shared library itself,
into the Python interpreter via Python's capsule stuff and the Tcl interpreter
via Tcl's associated data stuff, allowing both shared
libraries to be able to find what they need without resorting to global data
at all.

This was also necessary to support Python subinterpreters.

### What Magic Is This

```
tohil.call("set", "mydict", tohil.call("dict", "create", *itertools.chain(*d.items())))
```

Aww that's old stuff, with TclProcs we can do

```
l = {'a': 1, 'b': 2, 'c': 3, 'd': 4}
t = tohil.import_tcl()
t.set("mydict", t.dict("create", *itertools.chain(*l.items())))
t.dict("get", t.set("mydict"), "c", to=int)
```

that's a little gross, still.

With tclobjs we can do

```
o = tohil.tclobj({'a': 1, 'b': 2, 'c': 3, 'd': 4})
t.set("mydict", o)
o.td_get('c', to=int)
```


### Formatting

this needs to be built into the makefile or something

clang-format -style=file -i generic/tohil.c

### Debugging Tohil Internals

https://Pythonextensionpatterns.readthedocs.io/en/latest/debugging/debug_Python.html#debug-version-of-Python-memory-alloc-label

Build and install Python with something like

mkdir linux
cd linux
../configure --with-pydebug --without-pymalloc --with-valgrind --enable-shared

not sure about the enable shared

build tohil

./configure --prefix=/usr/local --exec-prefix=/usr/local --with-Python-version=3.9d

note 3.9d instead of just 3.9

### Acknowledgements

Tohil is based on, and is completely inspired by and exists because of, libtclpy, by Aidan Hobson Sayers available at https://github.com/aidanhs/libtclpy/.

### Image Attribution

Do you like the Tohil logo?  It's from a creative commons-licensed image of the Mayan deity Quetzalcoatl (also known in some cultures as Tohil), from the Codex Telleriano-Remensis, from the 16th century.

A scan of the image can be found here https://commons.wikimedia.org/wiki/File:Quetzalcoatl_telleriano.jpg.  A wikimedia user, https://commons.wikimedia.org/wiki/User:Di_(they-them), made an SVG file of it, available here https://commons.wikimedia.org/wiki/File:Quetzalcoatl_feathered_serpent.svg

