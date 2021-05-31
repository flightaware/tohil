
This is the Doc directory for tohil.

You're going to need to install sphinx.

You'll need to install the Tohil Docs Sphinx Theme, from
https://github.com/flightaware/tohil-docs-theme

It contains documentation patterned after Python's own documentation,
using reStructturedText markup, and using Sphinx to process it.

You'll need to make a data dir under Doc and touch a file
in there called refcounts.dat.

After that, from the command line you should be able to do a

	make html

...to create an html document tree of the Tohil rst docs.

    make serve 

...will launch Python's simple webserver to serve the built docs.
This is for developers.


