

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
ports from source.  You can also install without
building using the pkg package manager.

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

    sudo pip3 install hypothesis

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

This specification is a little trickier than usual because FreeBSD
is a little more rigorous about where stuff is supposed to go:

::

    ./configure --with-tcl=/usr/local/lib/tcl8.6 --mandir=/usr/local/man --with-python-include=/usr/local/include/python3.7m --with-python-lib=/usr/local/lib --with-python-version=3.7m

# don't forget the "m" if your stuff has that.


====
Make
====

::

    make
    sudo make install
    make test

There's a README.FreeBSD file in the top-level tohil directory
that might have some useful info in it.
