

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

    sudo pip3 install sphinx

==============================
Build the Configure Script
==============================

Next you build the configure script:

::

    autoreconf

Then run the configure script.  The Python version must
be specified.

If you get a ton of errors from autoreconf then you probably
need to run autoreconf 2.69.  Autoreconf 2.71 changed things
quite a bit and tohil's build hasn't been brought forward yet.

This isn't an uncommon problem and most Linux distros et al make
2.69 explicitly available for this reason.


========================
Run the Configure Script
========================

Run the configure script.  The Python version must be specified.

::

    ./configure --with-python-version=3.7

# don't forget the "m" if your stuff has that, which Debian tends to.


====
Make
====

::

    make
    sudo make install
    make test

There's a README.Linux file in the tohil repo's os_readmes directory
that might have some useful info in it.

