

.. _tohil-types:

**************
Tohil Types
**************

The following sections describe the types that importing tohil makes available
to the Python interpreter.

.. index:: pair: tohil; types

The principal tohil types are *tclobj* and *tcldict*.  There are a few additional
types for iterators and exceptions.

Tclobjs and tcldicts are mutable.  As with native Python types, methods that add,
subtract, or rearrange their members in place, and don't return a specific
item, returning ``None`` rather than the collection instance itself.

Some operations are supported by both object types; in particular,
they can be compared for equality, tested for truth
value, and converted to a string with the :func:`str` function, while
with the :func:`repr` produces a perhaps somewhat developer-friendly string
representation of the object.
Tclobjs can be used freely as float
or integer values in numeric calculations (when the contents of the tclobj are
numeric), including as a source or target of in-place arithmetic.

Tclobjs and tcldicts are very flexible in terms of what they can be
constructed from.  A tclobj can be created as an empty Tcl object, or
from a Python None object, a Python boolean, int, float, or string,
a Python list, tuple, set, dict, sequence
or map, and Unicode/UTF-8 translations should work fine.


.. _tohil-truth:

============================
Testing Tclobj Truth Values
============================

.. index::
   statement: if
   statement: while
   pair: truth; value
   pair: Boolean; operations
   single: false

Any tclobj can be tested for truth value, for use in an `if` or
`while` condition or as operand of a Boolean operations.

Interpretation of the boolean is according to Tcl rules.  These are very
close to Python rules, however.


    >>> import tohil
    >>> tohil.tclobj(True)
    <tohil.tclobj: '1'>
    >>> tohil.tclobj(False)
    <tohil.tclobj: '0'>

    >>> bool(tohil.tclobj(1))
    True
    >>> bool(tohil.tclobj(0))
    False

    >>> tohil.tclobj('y')
    <tohil.tclobj: 'y'>
    >>> bool(tohil.tclobj('y'))
    True
    >>> bool(tohil.tclobj('t'))
    True
    >>> bool(tohil.tclobj('f'))
    False
    >>> bool(tohil.tclobj('F'))
    False
    >>> bool(tohil.tclobj('not-a-boolean'))
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    TypeError: expected boolean value but got "not-a-boolean"




.. _tohil_comparisons:

===========
Comparisons
===========

Tclobjs and tcldicts can be compared.  When they are asked to be
compared, their string representations are compared.  If the Tcl
objects don't have strings available, they will be marshalled, and
this could be high overhead for large and/or very complicated structures.


.. _tohil_numeric:

====================================
Using Tohil tclobjs as Numeric Types
====================================

Tohil tclobjs can be freely used in Python code
where integers or floating
point numbers are needed.  The underlying Tcl object will be
requested using Tcl standard library routines, causing a fetch
of the cached representation if the cached representation is of
the correct type, or an attempt by the Tcl library to convert
the contents of the Tcl object to a python Boolean, integer or
float.

.. _tohil_bitstring-ops:

=================================
Bitwise Operations on Tohil Types
=================================

Tohil tclobj objects can be freely used as a source for boolean
operations and shift counts.  Bitwise and, or, exclusive or,
left and right shift, invert, and absolute value are supported.

Attempting bitwise operations on a tclobj that isn't or can't
be converted into an integer will fail with a TypeError exception raised.


.. _tohil_typesseq:

================
tclobjs as lists
================

Tclobjs whose internal contents are valid Tcl lists, can be largely
treated as Python lists.

Tclobjs-as-lists can be created from Python based on strings,
lists, tuples, sets, even dicts.  It's pretty cool.

The common sequence operations of ``in`` and ``not in`` work fine, while
the notation ``s[i]`` returns the *i*\ th item of tclobj *s*.

Slices are supported, for example ``s[i:j]`` returns a slice of *s*
from *i* to *j* while ``s[i:j:k]`` yields a slice of *s* from *i*
to *j* with step *k*.

``len(s)`` returns the length of *s*'s list, while ``min(s)`` returns
the smallest item and ``max(s)`` the largest.  Beware these'll be treated
like strings even if they're numbers.

Tclobjs are mutable; you can assign an element with ``s[i] = x``, append
an element with ``s.append(x)``, extend *s* with the contents of a Python
list, set, tuple, int, float, etc, or another tclobj, with
``s.extend(x)``.

(Because tclobjs are mutable, they cannot be directly used as a key
in a dictionary or a value in a set.  If you need to use one as a key,
wrap it with *str()* or something.)

You can clear a tclobj or tcldict using ``s.clear()``, and pop items
from the list using ``s.pop([i])``.

.. method:: list.append(x)
   :noindex:

   Add an item to the end of the list.  Equivalent to ``a[len(a):] = [x]``.


.. method:: list.extend(iterable)
   :noindex:

   Extend the list by appending all the items from the iterable.  Equivalent to
   ``a[len(a):] = iterable``.


.. method:: list.insert(i, x)
   :noindex:

   Insert an item at a given position.  The first argument is the index of the
   element before which to insert, so ``a.insert(0, x)`` inserts at the front of
   the list, and ``a.insert(len(a), x)`` is equivalent to ``a.append(x)``.


.. method:: list.remove(x)
   :noindex:

   Remove the first item from the list whose value is equal to *x*.  It raises a
   :exc:`ValueError` if there is no such item.


.. method:: list.pop([i])
   :noindex:

   Remove the item at the given position in the list, and return it.  If no index
   is specified, ``a.pop()`` removes and returns the last item in the list.  (The
   square brackets around the *i* in the method signature denote that the parameter
   is optional, not that you should type square brackets at that position.  You
   will see this notation frequently in the Python Library Reference.)


.. method:: list.clear()
   :noindex:

   Remove all items from the list.  Equivalent to ``del a[:]``.


.. method:: list.index(x[, start[, end]])
   :noindex:

   Return zero-based index in the list of the first item whose value is equal to *x*.
   Raises a :exc:`ValueError` if there is no such item.

   The optional arguments *start* and *end* are interpreted as in the slice
   notation and are used to limit the search to a particular subsequence of
   the list.  The returned index is computed relative to the beginning of the full
   sequence rather than the *start* argument.


Some standard Python list methods are not implemented, such as
``count``, ``reverse``, ``sort``, and ``copy``.


An example that uses most of the list methods::

    >>> fruits = tohil.tclobj(['orange', 'apple', 'pear', 'banana', 'kiwi', 'apple', 'banana'])
    >>> fruits
    <tohil.tclobj: 'orange apple pear banana kiwi apple banana'>
    >>> len(fruits)
    7
    >>> fruits.append('watermelon')
    >>> fruits
    <tohil.tclobj: 'orange apple pear banana kiwi apple banana watermelon'>
    >>> fruits.insert(1, 'cantaloupe')
    >>> fruits
    <tohil.tclobj: 'orange cantaloupe apple pear banana kiwi apple banana watermelon'>
    >>> fruits.pop()
    'watermelon'
    >>> fruits.pop(5)
    'kiwi'


.. _typesmapping:

==================================
Mapping Types --- :class:`tcldict`
==================================

Tcldicts are a Python type that manages a Tcl object of a dictionary structure.
Most things you can do with a Python dicts you can do with a tcldict.

However, unlike dicts, tcldicts are recursive.  From Python, if a key is
specified as a Python list, the Tcl dictionary is managed as a hierarchy
of dictionaries.

Tcldicts can be created by the :class:`tcldict` constructor.

.. class:: tcldict(val, [kwargs])

   Return a new tcldict initialized from an optional positional argument
   and a possibly empty set of keyword arguments.

   Tcldicts can be created by passing a Python ``list``, ``dict``,
   ``tuple``, or ``set``, a Tcl list, a tclobj or tcldict object,
   or create one aliased to a variable in the Tcl interpreter using
   ``tohil.tcldictvar``.

   If no positional argument is given, an empty tcldict is created.
   If a positional argument is given and it is a mapping object, a dictionary
   is created with the same key-value pairs as the mapping object.  Otherwise,
   the positional argument must be an `iterable` object.  Each item in
   the iterable must itself be an iterable with exactly two objects.  The
   first object of each item becomes a key in the new dictionary, and the
   second object the corresponding value.  If a key occurs more than once, the
   last value for that key becomes the corresponding value in the new
   dictionary.

   Keywords can be ``default``, ``to``, and/or ``var``.  Specifying
   a default using the keyword is the same as doing it using a positional
   parameter.

   The ``to`` keyword specifies a default type conversion to be applied
   when retrieving an item from the dict.  To-types can be str, bool,
   int, float, list, set, dict, tuple, tohil.tclobj or tohil.tcldict.

   These are the operations that dictionaries support (and therefore, custom
   mapping types should support too):

   .. describe:: list(d)

      Return a list of all the keys used in the tcldict *d*.

   .. describe:: len(d)

      Return the number of items in the tcldict *d*.

   .. describe:: d[key]

      Return the item of *d* with key *key*.  Raises a :exc:`KeyError` if *key* is
      not in the map.

      The :meth:`__missing__` method supported by native Python dicts is
      not support by tohil tcldicts.

   .. describe:: d[key] = value

      Set ``d[key]`` to *value*.

   .. describe:: del d[key]

      Remove ``d[key]`` from *d*.  Note that while native Python
      dicts raise a :exc:`KeyError` if *key* is not in the map,
      it is not an error to attempt to delete a key from a tohil
      dict.

   .. describe:: key in d

      Return ``True`` if *d* has a key *key*, else ``False``.

   .. describe:: key not in d

      Equivalent to ``not key in d``.

   .. describe:: iter(d)

      Return an iterator over the keys of the dictionary.  This is a shortcut
      for ``iter(d.keys())``.

   .. method:: clear()

      Remove all items from the dictionary.

   .. method:: get(key[, default])

      Return the value for *key* if *key* is in the dictionary, else *default*.
      If *default* is not given, it defaults to ``None``, so that this method
      never raises a :exc:`KeyError`.

   .. method:: items()

      Return a new view of the tcldict's items (``(key, value)`` pairs).
      Note that unlike native Python dict items, tcldict items are not
      mutable.  You probably didn't even know that dict items are mutable.
      See the :ref:`documentation of view objects <dict-views>`.

   .. method:: keys()

      Return a new view of the tcldict's keys.  As with items above, if
      you keep a reference to keys the keys doesn't change if the tcldict
      does.  For more on keys in general, see the 
      :ref:`documentation of view objects <dict-views>`.

   .. method:: pop(key[, default])

      If *key* is in the tcldict, remove it and return its value, else return
      *default*.  If *default* is not given and *key* is not in the dictionary,
      a :exc:`KeyError` is raised.

   .. method:: update([other])

      Update the dictionary with the key/value pairs from *other*, overwriting
      existing keys.  Return ``None``.

      :meth:`update` accepts either another dictionary object or an iterable of
      key/value pairs (as tuples or other iterables of length two).  If keyword
      arguments are specified, the dictionary is then updated with those
      key/value pairs: ``d.update(red=1, blue=2)``.

      Note: Not implemented yet unless it has been and someone didn't
      update the docs.

   .. method:: values()

      Return a new view of the tcldicts's values.  Same notes apply.  See the
      :ref:`documentation of view objects <dict-views>`.

   Dictionaries compare equal if and only if they are the exact same
   Tcl object or their Tcl string representations are identical.

   Order comparisons ('<', '<=', '>=', '>') can be performed.

   Please note that unlike modern Python dicts, Tcldicts do **not** preserve
   insertion order.  Tcldicts are traversed in hash order, which you can
   consider to effectively be random.  Sorry not sorry, not my fault.

.. _dict-views:

=======================
Dictionary view objects
=======================

The objects returned by :meth:`tcldict.keys`, :meth:`tcldict.values` and
:meth:`tcldict.items` are fake *view objects*.  Unlike native Python dicts,
they do not provide a dynamic view on the tcldict's entries, which means
that when the tcldict changes, the view does **not** reflect these changes.

Dictionary views can be iterated over to yield their respective data, and
support membership tests.

