

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


