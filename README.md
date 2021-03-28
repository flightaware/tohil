# tohil

<img src="https://github.com/flightaware/tohil/blob/main/graphics/237px-Quetzalcoatl_feathered_serpent.png">

Tohil a feathered serpent, aims to provide a delightful integration between python, the serpent, and TCL, the feather.

Tohil is simultaneously a Python extension and a TCL extension that makes it possible to effortlessly call bidirectionally between Tcl and Python, targeting Tcl 8.6+ and Python 3.6+

Tohil is open source software, available for free including for profit and/or for redistribution, under the permissive 3-clause BSD license (see "LICENSE.txt").

tohil is based on, and is completely inspired by and exists because of, libtclpy, by Aidan Hobson Sayers available at https://github.com/aidanhs/libtclpy/.

## Usage

You can import tohil into either a Tcl or Python parent interpreter. Doing so will create and initialise an interpreter for the corresponding language and define tohil's functions in both.

Using tohil, Python code can call Tcl code at any time, and vice versa, and they can call "through" each other, i.e. Python can call Tcl code that calls Python code that calls Tcl code limited only by your machine's memory and your sanity (and the (settable) Python and Tcl recursion limits).

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

The types supported are str, int, bool, float, list, set, and dict.

```python
>>> tohil.eval('set a [list a 1 b 2 c 3]')
'a 1 b 2 c 3'
>>> tohil.eval('return $a', to=list)
['a', '1', 'b', '2', 'c', '3']
>>> tohil.eval('return $a',to=dict)
{'a': '1', 'b': '2', 'c': '3'}
```

Note that currently for list, set and dict, the values constructed therein will be strings.  We already have code that can recognize and convert a few types and could use that, or perhaps we will create a way to specify desired type conversions within compound types.

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


#### tohil.expr

You can also evaluate tcl expressions from python using tohil.expr:

```
>>> tohil.expr('5+5')
'10'
>>> tohil.expr('5**5')
'3125'
>>> tohil.expr('1/3')
'0'
>>> tohil.expr('1/3.')
1
>>> tohil.expr('[clock seconds] % 86400')
'25571'
>>> tohil.expr('[clock seconds] % 86400',to=int)
25571
```

#### tohil.subst

Tcl's *subst* command is pretty cool.  By default it performs Tcl backslash, command and variable substitutions, but doesn't evaluate the final result, like eval would.


```
>>> import tohil
>>> tohil.eval("set name karl")
'karl'
>>> tohil.subst("hello, $name")
'hello, karl'
```

Although we could easily make tohil.subst support the "to=" way of request a type conversion, is there any case where you wouldn't just expect it to return a string?

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

### todo

This is the old list.  SOme of this stuff has been done.  We probably don't have the same priorities.  Will update over time.

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

The single tohil shared library created by building this software is loaded both by Python and Tcl, which is pretty cool and important to how it works.

### image attribution

Do you like the tohil logo?  It's from a creative commons-licensed image of the Mayan deity Quetzalcoatl (also known in some cultures as Tohil), from the Codex Telleriano-Remensis, from the 16th century.

A scan of the image can be found here https://commons.wikimedia.org/wiki/File:Quetzalcoatl_telleriano.jpg.  A wikimedia user, https://commons.wikimedia.org/wiki/User:Di_(they-them), made an SVG file of it, available here https://commons.wikimedia.org/wiki/File:Quetzalcoatl_feathered_serpent.svg


