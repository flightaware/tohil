# tohil

<img src="https://github.com/flightaware/tohil/blob/main/graphics/237px-Quetzalcoatl_feathered_serpent.png">

Tohil a feathered serpent, aims to provide a delightful integration between python, the serpent, and TCL, the feather.

Tohil is simultaneously a Python extension and a TCL extension that makes it possible to effortlessly call bidirectionally between Tcl and Python, targeting Tcl 8.6+ and Python 3.6+

Tohil is open source software, available for free including for profit and/or for redistribution, under the permissive 3-clause BSD license (see "LICENSE.txt").

tohil is based on, and is completely inspired by and exists because of, libtclpy, by Aidan Hobson Sayers available at https://github.com/aidanhs/libtclpy/.

Tohil is pronounced as, your choice, toe-heel, or toe-hill.

## Usage

You can import tohil into either a Tcl or Python parent interpreter. Doing so will create and initialise an interpreter for the corresponding language and define tohil's functions in both.

Using tohil, Python code can call Tcl code at any time, and vice versa, and they can call "through" each other, i.e. Python can call Tcl code that calls Python code that calls Tcl code, limited only by your machine's memory and your sanity (and the (settable) Python and Tcl recursion limits).

### Accessing TCL From Python

To use Python to do things in Tcl, you invoke functions defined by the tohil module that gets created when you import tohil into your Python interpreter.

```python
import tohil
```

#### tohil.eval

 - `tohil.eval(evalstring, to=type)`
   - takes: string of valid Tcl code
   - returns: the final return value
   - side effects: executes code in the Tcl interpreter
   - *Do not use with untrusted substituted input*
   - `evalString` may be any valid Tcl code, including semicolons for single line statements or multiline blocks
   - uncaught tcl errors tracing back all the way to the the tohil membrane are raised as a python exception

By default the results of the Tcl code evaluated (if there wasn't an exception) is returned to the caller, as a string.

The optional "to" named parameter allows you to specify one of a number of data types that will cause tohil to convert the return into a native Python data type.

The types supported are str, int, bool, float, list, set, dict, tuple, and tohil.tclobj.

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

Note that currently for list, set, dict, and tuple, the values constructed therein will be strings.  We already have code that can recognize and convert a few types and could use that, or perhaps we will create a way to specify desired type conversions within compound types.

#### tohil.call

 - `tohil.call(command, arg1 arg2, arg3, to=type)`
   - takes: single Tcl command name plus zero or more arguments, and an optional data type to convert the return to
   - returns: whatever tcl returned
   - side effects: executes code in the Tcl interpreter
   - uncaught tcl errors tracing back all the way to the the tohil interface are raised as a python exception

As it can be tricky to invoke Tcl using eval and not getting possibly unwanted side effects if arguments (such as data!) contain tcl metadata such as square brackets and dollar signs, a direct argument-for-argument *tohil.call* is provided where tcl will not do variable and command substitution on its arguments and thus help keep funny business to a minimum.

```python
>>> import tohil
>>> clock = 1616182348
>>> tohil.call('clock', 'format', clock, '-locale', 'fr')
'ven. mars 19 19:32:28 UTC 2021'
```

The above example is trivial and not really an example of something that might be unsafe to use eval for.  But imagine if you were submitting arbitrary data as arguments to Tcl commands.  It would be difficult to examine it in python to be sure tcl will execute it as you intended.

#### tohil.getvar and tohil.setvar

Python has direct access TCL variables and arrays using tohil.getvar.  Likewise, tohil.setvar can set them.

```
>>> import tohil
>>> tohil.setvar("foo", "bar")
>>> tohil.getvar("foo")
'bar'
>>> tohil.setvar(var="happy", value="lamp")
>>> tohil.getvar("happy")
'lamp'

>>> tohil.eval("array set x [list a 1 b 2 c 3 d 4]")
''
>>> tohil.getvar('x(a)')
'1'
>>> tohil.getvar('x(a)', to=int)
1
>>> tohil.getvar(var='x(b)', to=float)
2.0
>>> tohil.getvar("x(e)")
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
RuntimeError: can't read "x(e)": no such element in array
```

As you can see, it's an error to try to get a variable or array element that isn't there.  You can use tohil.exists to see if the variable is there, or trap the python exception, or possibly make use of tohil.getvar's handy "default" keyword-only argument.

```
>>> tohil.getvar("x(e)", default="0")
'0'
>>> tohil.getvar("x(e)", default=0, to=int)
0
>>> tohil.getvar("x(d)", default=0, to=int)
4
```

#### tohil.incr

tohil.incr takes a tcl variable name or array element and attempts to increment it.

If the contents of the variable preclude it being used as an int, a python
TypeError exception is thrown.

An optional position argument specifies an increment amount.  The default increment is 1.
Negative increments are permitted.  The increment amount can also be specified as
a keyword argument, using "incr".

```
tohil.incr('var')
tohil.incr('var',2)
tohil.incr('var',incr=-1)
```


#### tohil.exists

Since it is an error to try to getvar a variable that doesn't exist, you can trap the request from python and handle the exception, or use tohil.exists to see if the var or array element exist.

```
>>> tohil.eval("array set x [list a 1 b 2 c 3 d 4]")
''
>>> tohil.exists("x(c)")
True
>>> tohil.exists("x(e)")
False
>>>
>>> tohil.exists("banana")
False
```

#### tohil.unset

Tohil can be used to unset variables, array elements, and even entire arrays in the Tcl interpreter.

Unsetting an array element uses the subscrit notation, for example `x(e)`.

Unsetting an array by name with a subscript will unset the entire array.

It is not an error to attempt to unset a variable that doesn't exist.

```
>>> tohil.setvar("x(e)", "5")
>>> tohil.getvar("x(e)")
'5'
>>> tohil.unset("x(e)")
>>> tohil.getvar("x(e)")
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
RuntimeError: can't read "x(e)": no such element in array
```


#### tohil.expr

You can also evaluate tcl expressions from python using tohil.expr.  As with many other tohil functions, to= can be used to request conversion to a specific python datatype.

```
>>> tohil.expr('5+5')
'10'
>>> tohil.expr('5**5')
'3125'
>>> tohil.expr('1/3')
'0'
>>> tohil.expr('1/3.')
'0.3333333333333333
>>> tohil.expr('1/3.',to=float)
0.3333333333333333
>>> tohil.expr('[clock seconds] % 86400')
'25571'
>>> tohil.expr('[clock seconds] % 86400',to=int)
25571
```

Remember that, like eval, tohil.expr evaluates its arguments and will do $-substition and execute square-bracketed code embedded in the passed expression.

#### tohil.convert

Tohil.convert will convert some python thing passed to it, into a tcl object, and then back to some other python type, a string by default, but any type supported in accordance with the to= argument.

It's an easy way to get a tclobj object `(t = tohil.convert(5, to=tohil.tclobj)`, but in that case it's easier to do `t = tohil.tclobj(5)`.

Pass a python object to tohil.convert and get back a string by default, or use the same to=

#### tohil.subst

Tcl's *subst* command is pretty cool.  By default it performs Tcl backslash, command and variable substitutions, but doesn't evaluate the final result, like eval would.  So it's nice to generate some kind of string but with embedded $-substitution and square bracket evaluation.


```
>>> import tohil
>>> tohil.eval("set name karl")
'karl'
>>> tohil.subst("hello, $name")
'hello, karl'
```

The "to=" way of requesting a type conversion is supported.  Although you might not care about converting to int or float or something, you might want a tohil.tclobj for your efforts, anirite?

#### tohil.interact

Run the Tcl interactive command loop on stdin, hopefully a terminal, until you send an EOF, at which point you'll be returned to the python command line.  See also tohil::interact.

#### Shadow Dictionaries

Shadow Dictionaries, aka ShadowDicts, create a python dict-like object that shadows a Tcl array.

Tcl arrays are kind of the Tcl equivalent of Python's dicts, by the way.

Anyway, let's assume we have an array "x" in Tcl that we want to shadow as a dictionary "x" in Python, we would write `x = tohil.ShadowDict("x")`

Once created, the shadowdict can be gotten as a string using str() or print(), etc.

Elements can be read form the python side using dictionary notation, for example `x['d']`, set in a standard way (`x['e'] = '5'`), and deleted using del (`del x['e']`).  Also you can iterate on the keys as with dicts.

Changes made from the Python side occur on the Tcl side, and all accesses, traversals, etc, are made using the Tcl array.  In other words, ShadowDicts never cache values from the Tcl array on the python side.

In the example below we set up a Tcl array, create a ShadowDict of it in python, get a string representation of the dict, read from the dict, insert into it, delete from it, and demonstrate that the changes we made are present on the Tcl side.  Finally, it iterates over the shadow dict, showing the same keys from python that tcl was shown to have.

```
>>> tohil.eval("array set x [list a 1 b 2 c 3 d 4]")
''
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
''
>>> for i in x:
...     print(i)
...
a
b
c
e
```

#### Examples using tohil from Python

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
>>> tohil.eval('return $a' to=list)
['a', '1', 'b', '2', 'c', '3']
>>> tohil.eval('return $a',to=dict)
{'a': '1', 'b': '2', 'c': '3'}

>>> tohil.eval(to=list,tcl_code="return [list 1 2 3 4]")
['1', '2', '3', '4']

```

Check this out, converting expected results to python datatypes:

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

### Accessing Python From TCL

From Tcl, tohil provides access to Python through several commands and some procs.

Probably the most important commands are `tohil::eval`, `tohil::exec` and `tohil::call`.  The first two commands correspond closely to python's `eval` and `exec`.

General notes:
 - All commands are run in the context of a single interpreter session. Imports, function definitions and variables persist.
 - Uncaught exceptions in the python interpreter resulting from code invoked from Tcl using tohil will propagate a TCL error including a stack trace of the python code that was executing. As the exception continues up the stack, the tcl stack trace will be appended to it.
 - The Tcl error code is set to a list comprising "PYTHON", the class name of the exception, and the base error message.  This is experimental but likely to continue.  I would like to add the class arguments, though.
 - Such Python errors may be caught (as per tcl stack traces) with Tcl's catch or try, the same as any other TCL error.

```tcl
package require tohil
```

Tohil provides new commands for interacting with the python interpreter, via the ::tohil namespace.

#### tohil::eval

tohil::eval evaluates the code passed to it as if with native python's eval.  So the argument has to be an expression, some kind of simple call, etc, i.e. it is an error if you try to define a function with it, or even set the value of a variable.

Anything returned by python from the eval is returned to to the caller of tohil::eval.

#### tohil::exec

tohil::exec evaluates the code passed to it as if with python's exec.  Nothing is returned.  If the python code prints anything, it goes to stdout using python's I/O subsystem.  However you can easily redirect python's output to go to a string, or whatever, in the normal python manner.  tohil::run, in fact, provides a way to do this.

#### tohil::run

tohil::run evaluates the code passed to it as if with python's exec, but unlike tohil::exec, anything emitted by the python code to python's stdout (print, etc) is captured by tohil::run and returned to the caller.

#### tohil::call

tohil::call provides a way to invoke one python function, with zero or more arguments, without having to pass it through Python's eval or exec and running the risk that python metacharacters appearing in the data will cause quoting problems, accidental code execution, etc.

#### tohil::import

tohil::import provides a way to import python modules, although I'm not sure that it's much different from doing a tohil::exec "import module"

#### tohil::interact

Take tohil to eleven.  You're on ten here... all the way up... You're on ten on your guitar... where can you go from there?  Where?  Nowhere.  Exactly.  What we do is if we need that extra... push over the cliff... you know what we do?

We run tohil::interact from tcl and enter the python interactive loop.  When we're done, we send end of file (^D) to end the python loop and return to the tcl one.

```
tcl % tohil::interact
>>> def foo():
...   print("bar")
...
>>> ^D
tcl % tohil::eval foo()
bar
```

#### Reference

 - `tohil::eval evalString`
   - takes: string of valid python code
   - returns: the result of the eval
   - side effects: executes code in the python interpreter
   - `evalString` may be any valid python expression
 - `tohil::call ?obj.?func ?arg ...?`
   - takes: name of a python function
   - returns: return value of function with the first appropriate conversion applied from the list below:
     - `None` is converted to an empty string
     - `True` and `False` are converted to a Tcl boolean object with a corresponding value
     - Python *str objects* are converted to Tcl byte arrays
     - Python *unicode objects* are converted to Tcl unicode strings
     - Python *number objects* are converted I think to text numbers NB this is probably improvable
     - Python *mapping objects* (supporting key-val mapping, e.g. python dicts) are converted to tcl dicts
     - Python *sequence objects* (supporting indexing, e.g. python lists) are converted to tcl lists
     - Otherwise, the str function is applied to the python object and that's set into the corresponding Tcl object
   - side effects: executes function
   - `func` may be a dot qualified name (i.e. object or module method)
 - `tohil::exec execString`
   - `takes: string of valid python code`
   - `returns: the result of the python exec`
   - `side effects: executes code in the python interpreter`
   - **Do not use with substituted input**
   - `execString` may be any valid python code, including semicolons for single line statements or (non-indented) multiline blocks with indentions, etc.
   - errors reaching the python interpreter top level (i.e. not caught tcl-side or python-side by application code) are printed to stderr
 - `tohil::import module`
   - takes: name of a python module
   - returns: nothing
   - side effects: imports named module into globals of the python interpreter
   - the name of the module may be of the form module.submodule
   - You can do the same thing using exec and, currently, exercise more control.  For example `tohil::exec "from io import StringIO"`


#### Examples using tohil from Tcl

```
% package require tohil
% tohil::exec {divide = lambda x: 1.0/int(x)}
% set d [tohil::call divide 16]
0.0625
% list [catch {tohil::call divide 0} err] $err
1 {ZeroDivisionError: float division by zero
  File "<string>", line 1, in <lambda>
----- tcl -> python interface -----}
%
% tohil::import json
% tohil::exec {
def jobj(*args):
    d = {}
    for i in range(len(args)/2):
        d[args[2*i]] = args[2*i+1]
    return json.dumps(d)
}
% set e [dict create]
% dict set e {t"est} "11{24"
t\"est 11\{24
% dict set e 6 5
t\"est 11\{24 6 5
% set e [py call jobj {*}$e]
{"t\"est": "11{24", "6": "5"}
%
% tohil::import sqlite3
% tohil::eval {b = sqlite3.connect(":memory:").cursor()}
% tohil::eval {def exe(sql, *args): b.execute(sql, args)}
% tohil::call exe "create table x(y integer, z integer)"
% tohil::call exe "insert into x values (?,?)" 1 5
% tohil::call exe "insert into x values (?,?)" 7 9
% tohil::call exe "select avg(y), min(z) from x"
% tohil::call b.fetchone
4.0 5
% tohil::call exe "select * from x"
% set f [tohil::call b.fetchall]
{1 5} {7 9}
%
% puts "a: $a, b: $b, c: $c, d: $d, e: $e, f: $f"
a: , b: 15, c: someinput, d: 0.0625, e: {"t\"est": "11{24", "6": "5"}, f: {1 5} {7 9}
```

This might bake your noodle...

```
>>> tohil.eval('tohil::eval "2 ** 32 - 1"')
'4294967295'
```

### Using tohil from Rivet

From a Rivet page, in some of your Tcl code, invoke `package require tohil`.

If you run tohil_rivet it will plug tohil's python interpreter such that everything it writes to stdout using print, or whatever, will go through Tcl's stdout and thereby into your Rivet page.

```
<?

package require tohil; tohil_rivet

puts "calling out to python to add 5 + 5: [::tohil::eval "5 + 5"]"

tohil::exec {
print('hello, world')
print("<hr>")
}

?>
```

###  Building tohil on Unix, Linux, FreeBSD and the Mac

tohil builds with the familiar GNU autoconf build system.  "autoreconf" will produce a configure script based on the configure.in.  The tooling used is the standard Tcl Extension Architecture (TEA) approach, which is pretty evolved and fairly clean considering it's autoconf.

It is assumed that you
 - have got the repo (either by `git clone` or a tar.gz from the releases page).

The build process fairly simple:
 - run the configure script
 - make
 - sudo make install

We're using setuptools to build the python module, so the Makefile.in/Makefile is basically doing

```
python3 setup.py build
python3 setup.py install
```

...to build and install the python module.

Now try it out:

	$ TCLLIBPATH=. tclsh
	% package require tohil
	1.0.0
	% tohil::import random
	% tohil::call random.random
	0.507094977417

### tests

Run the tests with

	$ make test

### gotchas

1. Be very careful when putting unicode characters into a inside a `tohil.eval`
or `tohil.exec` call - they are decoded by the tcl parser and passed as literal bytes
to the python interpreter. So if we directly have the character "ಠ", it is
decoded to a utf-8 byte sequence and becomes u"\xe0\xb2\xa0" (where the \xXY are
literal bytes) as seen by the Python interpreter.
2. Escape sequences (e.g. `\x00`) inside py eval may be interpreted by tcl - use
{} quoting to avoid this.

You need to build the library without stubs for python to be able to use it.

On FreeBSD at least i have to change -ltclstub86 to -ltcl86 in Makefile after
it is created -- this needs to be done properly

On the Mac the python3 setup.tcl thing builds a shared library but it doesn't properly link it to the tcl library so you get a runtime error when you try to import tohil.

copy the .dylib file that make built to the .so file where make install sent the python module and it will work.

something like (on the mac, using pyenv)

sudo cp tohil1.0.0.dylib ~/.pyenv/versions/3.8.2/lib/python3.8/site-packages/tohil.cpython-38-darwin.so

or

cp tohil1.0.0.dylib build/lib.macosx-10.6-x86_64-3.8/tohil.cpython-38-darwin.so

### To Do

* a way to pass kwargs thought tohil::eval - done
* if python is the parent, register a tcl panic handler and invoke Py_FatalError if tcl panics.
* the reverse of the above if tcl is the parent if python has a panic-type function with a registerable callback

Below is the old list.  Some of this stuff has been done.  We probably don't have the same priorities.  Will update over time.

In order of priority:

 - allow compiling on Windows
 - `py call -types [list t1 ...] func ?arg ...? : ?t1 ...? -> multi`
   (polymorphic args, polymorphic return)
 - unicode handling (exception messages, fn param, returns from calls...AS\_STRING is bad)
 - allow statically compiling python into tohil
   - http://pkaudio.blogspot.co.uk/2008/11/notes-for-embedding-python-in-your-cc.html
   - https://github.com/albertz/python-embedded
   - https://github.com/zeha/python-superstatic
   - http://www.velocityreviews.com/forums/t741756-embedded-python-static-modules.html
   - http://christian.hofstaedtler.name/blog/2013/01/embedding-python-on-win32.html
   - http://stackoverflow.com/questions/1150373/compile-the-python-interpreter-statically
 - allow statically compiling
 - check threading compatibility
 - `py import ?-from module? module : -> nil`
 - return the short error line in the catch err variable and put the full stack trace in errorInfo
 - py call of non-existing function says raises attribute err, should be a NameError
 - make `py call` look in the builtins module - http://stackoverflow.com/a/11181607
 - all TODOs

### notes

[pyman](http://chiselapp.com/user/gwlester/repository/pyman/home) is a Tcl package that provides a higher level of abstraction on top of tclpy.  It will need to be updated for tohil but bears examination and hopefully the participation of its author, Gerald Lester.

### geek notes

The same tohil shared library created by building this software can be
loaded both by Python and Tcl, which is pretty cool.  It used to be
necessary so that tohil could work at all.

However since there are different
build pipelines for tcl extensions (based on autoconf via the tcl extension
architecture) and python (based on python setuptools), we
changed tohil's implementation to be able to work ok even with two different
shared libraries by moving the critical piece of shared data, the tcl
interpreter pointer, formerly held statically by the shared library itself,
into the python interpreter via python's capsule stuff, allowing both shared
libraries to be able to find the interpreter.

### what magic is this

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


### formatting

this needs to be built into the makefile or something

clang-format -style=file -i generic/tohil.c

### debugging tohil internals

https://pythonextensionpatterns.readthedocs.io/en/latest/debugging/debug_python.html#debug-version-of-python-memory-alloc-label


build and install python with something like

mkdir linux
cd linux
../configure --with-pydebug --without-pymalloc --with-valgrind --enable-shared

not sure about the enable shared

build tohil

./configure --prefix=/usr/local --exec-prefix=/usr/local --with-python-version=3.9d

note 3.9d instead of just 3.9

### image attribution

Do you like the tohil logo?  It's from a creative commons-licensed image of the Mayan deity Quetzalcoatl (also known in some cultures as Tohil), from the Codex Telleriano-Remensis, from the 16th century.

A scan of the image can be found here https://commons.wikimedia.org/wiki/File:Quetzalcoatl_telleriano.jpg.  A wikimedia user, https://commons.wikimedia.org/wiki/User:Di_(they-them), made an SVG file of it, available here https://commons.wikimedia.org/wiki/File:Quetzalcoatl_feathered_serpent.svg

