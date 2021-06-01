

.. _tohil-installing-linux:

*********************************
Building and Installing on Linux
*********************************

=====================
Install Prerequisites
=====================

*You should backup your machine regularly, and confirm
you have made a full backup before proceeding.*

First you need to install Python and
the Python *pip* installer:

::

   sudo apt install python3-dev python3-pip tcl8.6-dev

There are a few addition things that are probably nice to have:

::

    sudo apt install tcl-doc tcl-tclreadline tclx8.4-dev tclx8.4-doc
    sudo apt install tcllib tcllib-critcl

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

Then run the configure script.  The Python version must
be specified.

========================
Run the Configure Script
========================

Run the configure script.  The Python version must be specified.

::

    ./configure --with-python-version=3.7m

# don't forget the "m" if your stuff has that, which Debian tends to.


====
Make
====

::

    make
    sudo make install
    make test

There's a README.Linux file in the top-level tohil directory
that might have some useful info in it.
