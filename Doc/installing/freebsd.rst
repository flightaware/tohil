

.. _tohil-installing-freebsd:

**********************************
Building and Installing on FreeBSD
**********************************

=====================
Install Prerequisites
=====================

*You should backup your machine regularly, and confirm
you have made a full backup before proceeding.*

These instructions assume you are building FreeBSD
ports from source.  You can also install ports without
building from source by using the pkg package manager.
We're only covering doing it using ports at this time.

First you need to install Python and
the Python *pip* installer:

::

    cd /usr/ports/lang/python39
    sudo make install

    cd /usr/ports/devel/py-pip
    sudo make install

Next install Tcl if you haven't already:

::

    cd /usr/ports/lang/tcl86
    sudo make install

    cd /usr/ports/lang/tclX
    sudo make install

    cd /usr/ports/devel/tcllib
    sudo make install

    cd /usr/ports/devel/tcllibc
    sudo make install

There are a few addition things that are probably nice to have
such as ports *devel/tclreadline*, *databases/tcl-sqlite3*,
*devel/tclbsd*, *devel/tcllauncher*, and *devel/tcltls*.

To run the test suite, you'll need Python's *hypothesis*
module:

::

    sudo pip3 install hypothesis

...and if you plan to build documentation, sphinx:

::

    sudo pip3 install sphinx

==============================
Build the Configure Script
==============================

Next you build the configure script:

::

    autoreconf

You might need to install *devel/autoconf*.

========================
Run the Configure Script
========================

Run the configure script.  The Python version must be specified.

This specification is a little trickier than usual because the
approach the FreeBSD developers have taken toward packaging
is a little more particular about where stuff is supposed to go.

This has advantages, though.  For instance you can have multiple
versions of Tcl installed and multiple versions of Python 3
installed at the same time.

::

    ./configure --with-tcl=/usr/local/lib/tcl8.6 --mandir=/usr/local/man --with-python-version=3.7m

In the above, we tell configure where to find the Tcl library because
it's in a slightly nonstandard place.  We tell it the Python version;
Tohil's configure script will use python3.7m-config or whatever
to find the Python library and includes.


Don't forget the "m" in the version name if your stuff has that.


====
Make
====

::

    make
    sudo make install
    make test

There's a README.FreeBSD file in the top-level tohil directory
that might have some useful info in it.
