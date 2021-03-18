# tohil

tohil is based on libtclpy, by Aidan Hobson Sayers.

This is tohil, a dual-purpose Python extension AND TCL extension that makes it possible to effortlessly call bidirectionally between Tcl and Python, targeting Tcl >= 8.6 and Python 3.6+

The extension is available under the 3-clause BSD license (see "LICENSE").

Tcl users may also want to consider using
[pyman](http://chiselapp.com/user/gwlester/repository/pyman/home), a Tcl package
that provides a higher level of abstraction on top of tclpy.

## Usage

You can import tohil into either a Tcl or Python parent interpreter. Doing
so will initialise an interpreter for the other language and insert all
libtclpy methods. This means you can call backwards and forwards between
interpreters.

### From TCL

General notes:
 - Unless otherwise noted, 'interpreter' refers to the python interpreter.
 - All commands are run in the context of a single interpreter session. Imports,
   function definitions and variables persist.
 - Exceptions in the python interpreter will return a stack trace of the python
   code that was executing. If the exception continues up the stack, the tcl
   stack trace will be appended to it.
   They may be masked (as per tcl stack traces) with catch.

```tcl
package require tohil
```

Reference:
 - `tohil::call ?obj.?func ?arg ...?`
   - `takes: name of a python function`
   - `returns: return value of function with the first appropriate conversion
      applied from the list below:`
     - `None is converted to an empty string`
     - `True is converted to 1`
     - `False is converted to 0`
     - `Python 'str' objects are considered to be byte arrays`
     - `Python 'unicode' objects are considered to be unicode strings`
     - `Python 'number' objects are converted to base 10 if necessary`
     - `Python mapping objects (supporting key-val mapping, e.g. python dicts)
        are converted to tcl dicts`
     - `Python sequence objects (supporting indexing, e.g. python lists) are
        converted to tcl lists`
     - `Otherwise, the str function is applied to the python object`
   - `side effects: executes function`
   - `func` may be a dot qualified name (i.e. object or module method)
 - `tohil::eval evalString`
   - `takes: string of valid python code`
   - `returns: the result of the eval`
   - `side effects: executes code in the python interpreter`
   - **Do not use with substituted input**
   - `evalString` may be any valid python expression
 - `tohil::exec execString`
   - `takes: string of valid python code`
   - `returns: the result of the python exec`
   - `side effects: executes code in the python interpreter`
   - **Do not use with substituted input**
   - `execString` may be any valid python expression code, including semicolons for single line statements or (non-indented) multiline blocks
   - errors reaching the python interpreter top level are printed to stderr
 - `tohil::import module`
   - `takes: name of a python module`
   - `returns: nothing`
   - `side effects: imports named module into globals of the python interpreter`
   - the name of the module may be of the form module.submodule

example tclsh session:

```
% package require tohil
%
% tohil::exec {def mk(dir): os.mkdir(dir)}
% tohil::exec {def rm(dir): os.rmdir(dir); return 15}
% tohil::import os
% set a [tohil::eval {print "creating 'testdir'"; mk('testdir')}]
creating 'testdir'
% set b [py call rm testdir]
15
%
% tohil::import StringIO
% tohil::eval {sio = StringIO.StringIO()}
% tohil::call sio.write someinput
% set c [py call sio.getvalue]
someinput
%
% tohil::eval {divide = lambda x: 1.0/int(x)}
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

### From Python

Reference:
 - `tohil.eval(evalstring)`
   - `takes: string of valid Tcl code`
   - `returns: the final return value`
   - `side effects: executes code in the Tcl interpreter`
   - **Do not use with substituted input**
   - `evalString` may be any valid Tcl code, including semicolons for single
     line statements or multiline blocks
   - errors reaching the Tcl interpreter top level are raised as an exception

You can access variables in TCL from python using the getvar method:

tohil.getvar(var)
tohil.getvar(array, var)
tohil.getvar(array='a', var='5')

example python session:

```
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


>>> tclpy.eval(to="list",tcl_code="return [list 1 2 3 4]")
['1', '2', '3', '4']

```
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


now megaval with to='set' option to return a set from a list

>>> tclpy.megaval('return [list 1 2 3 4 4 3]',to='set')
{'3', '4', '2', '1'}

TODO

make tclpy.expr able to do the to= stuff that tclpy.megaval can do.

get rid of tclpy.eval and rename tclpy.megaval to tclpy.eval

intercept stdout when exec'ing python in rivet and pump it to rivet


### Unix Build

tohil builds with the familiar GNU autoconf build system.  "autoreconf" will produce a configure script based on the configure.in.  The tooling used is the standard Tcl Extension Architecture (TEA) approach, which is pretty evolved and fairly clean considering it's auitoconf.

It is assumed that you
 - have got the repo (either by `git clone` or a tar.gz from the releases page).

The build process fairly simple:
 - run the configure script
 - make
 - sudo make install

We're using distutils to build the python module, so the Makefile.in/Makefile is basically doing

```
python3 setup.py build
python3 setup.py install
```

to build and installing the python module

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

1. Be very careful when putting unicode characters into a inside a `py eval`
call - they are decoded by the tcl parser and passed as literal bytes
to the python interpreter. So if we directly have the character "ಠ", it is
decoded to a utf-8 byte sequence and becomes u"\xe0\xb2\xa0" (where the \xXY are
literal bytes) as seen by the Python interpreter.
2. Escape sequences (e.g. `\x00`) inside py eval may be interpreted by tcl - use
{} quoting to avoid this.

you need to build the library without stubs for python to be able to use it.

on freebsd at least i have to change -ltclstub86 to -ltcl86 in Makefile after
it is created -- this needs to be done properly

on the mac the python3 setup.tcl thing builds a shared library but it doesn't properly link it to the tcl library so you get a runtime error when you try to import tohil.

copy the .dylib file that make built to the .so file where make install sent the python module and it will work.

something like (on the mac, using pyenv)

sudo cp tohil1.0.0.dylib ~/.pyenv/versions/3.8.2/lib/python3.8/site-packages/tclpy.cpython-38-darwin.so

or

cp tohil1.0.0.dylib build/lib.macosx-10.6-x86_64-3.8/tclpy.cpython-38-darwin.so

### todo

This is the old list.  SOme of this stuff has been done.  We probably don't have the same priorities.  Will update over tinme.

In order of priority:

 - allow python to call back into tcl
 - allow compiling on Windows
 - `py call -types [list t1 ...] func ?arg ...? : ?t1 ...? -> multi`
   (polymorphic args, polymorphic return)
 - unicode handling (exception messages, fn param, returns from calls...AS\_STRING is bad)
 - allow statically compiling python into tclpy
   - http://pkaudio.blogspot.co.uk/2008/11/notes-for-embedding-python-in-your-cc.html
   - https://github.com/albertz/python-embedded
   - https://github.com/zeha/python-superstatic
   - http://www.velocityreviews.com/forums/t741756-embedded-python-static-modules.html
   - http://christian.hofstaedtler.name/blog/2013/01/embedding-python-on-win32.html
   - http://stackoverflow.com/questions/1150373/compile-the-python-interpreter-statically
 - allow statically compiling
 - check threading compatibility
 - let `py eval` work with indented multiline blocks
 - `py import ?-from module? module : -> nil`
 - return the short error line in the catch err variable and put the full stack
   trace in errorInfo
 - py call of non-existing function says raises attribute err, should be a
   NameError
 - make `py call` look in the builtins module - http://stackoverflow.com/a/11181607
 - all TODOs


