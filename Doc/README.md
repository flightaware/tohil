
## Hello There

This is the Doc directory for tohil.

From here you can build the tohil HTML documentation.

This documentation is served by the Tohil maintainers
at https://flightaware.github.io/tohil-docs/

You would build this documentation if you are wanting
to help with it, are forking Tohil, or, I don't know,
whatever reason you might want it.

### Dependencies

You're going to need to install sphinx, probably
using something like `pip3 install sphinx`.

You might want to use a venv.

### Tohil Sphinx Theme

You'll need to install the _Tohil Docs Sphinx Theme_, from
https://github.com/flightaware/tohil-docs-theme

It provides a documentation structure
patterned after Python's own documentation,
using reStructturedText markup, and using Sphinx to process it.

### Weird Thing You Shouldn't Have To Do

You'll need to make a data dir under Doc and touch a file
in there called refcounts.dat.

### Build the HTML

After that, from the command line you should be able to do a

	make html

...to create an html document tree of the Tohil rst docs.

This builds the documentation in the `_build/html` subdirectory
of tohil/Docs (the directory where this READXME file resides).

### Serve the HTML Locally to View It

    make serve 

...will launch Python's simple webserver to serve the built docs.
This is for developers only; a real webserver would be required
if you wanted to serve this onto the Internet.

While running you should be able to access the files at
http://localhost:8000 or the IP address or hostname of the machine
where you're running `make serve` from.

### Or on the Mac Read the Docs Directly from Files Using Your Browser

	cd _build/html
	open index.html


