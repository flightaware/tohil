

.. _tohil-installing-macos:

**********************************
Building and Installing on macOS
**********************************

=====================
Install Prerequisites
=====================

*You should backup your machine regularly, and confirm
you have made a full backup before proceeding.*

These instructions assume you are using MacPorts,
an open source community initiative to provide
a way to build and install open source software
on a Mac, and handle their dependencies.

Please follow instructions on installing MacPorts
at https://www.macports.org.

*Caveat emptor:  Remember in our License we are not
responsible if this stuff screws up your computer
or fails to work in the expected way for whatever reason.*

Make sure you've got Xcode installed (Apple's developer
tools; they're free. You can install it from Apple's App Store.)

Then install MacPorts for the version of macOS that you're
using/building for.


===========================
Bring MacPorts Up to Date
===========================

You'll want to update MacPorts config to the latest, using
its *selfupdate* feature, and upgrade any installed ports.

::

  sudo port selfupdate

This next part feels a wee bit dangerous in that it
might break some of your ports, but it's cool.  It'll
update all your installed ports, and their dependencies,
to the latest version it can:

::

  sudo port upgrade outdated

You can use ``port outdated`` to see what ports need updating.

===============
Install Python
===============

Install python and select it.  If you expect ``python``
to start Python 2 instead of Python 3, don't execute
the line below that sets *python* to point to
*python39*.

::

  sudo port install python39
  sudo port select --set python python39
  sudo port select --set python3 python39
  sudo port install py39-setuptools

===============
Install Tcl
===============

The commands below will install Tcl and a number of widely
used packages.  You can, of course, install all the Tcl
stuff you want.

::

    sudo port install tcl tcl-tls tcllib
    sudo port install tclreadline tclx
    sudo port install sqlite3-tcl

================
Install Autoconf
================

Install autoconf, a little bit older version than the absolute
latest, because autoconf 2.71 changed stuff quite a bit and
the Tcl Extension Architecture (TEA) autoconf stuff that
TOhil uses hasn't
yet been updated to work with it, so if you try to use 2.71
to create the *configure* script, it'll fail with a whole bunch
of errors.

::

    sudo port install autoconf264

================
Run Autoconf
================

From the tohil top-level directory, run ``autoconf264``.
Like many Unix command line tools, it will produce no output
if everything worked.

========================
Run the Configure Script
========================

It shouldn't be necessary to specify *--exec-prefix*, but it seems
to be.  We could use some help on this.

::

    ./configure --prefix=/opt/local --exec-prefix=/opt/local --with-python-version=3.9 --with-tcl=/opt/local/lib

...then *make* and *make install*:


====
Make
====

::

    make
    sudo make install
    make test

There's *README.macOS* and *README.MacPorts* files in the top-level
tohil directory that might have some useful info in them.

