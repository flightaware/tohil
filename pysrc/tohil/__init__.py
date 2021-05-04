""" tohil """

from collections.abc import MutableMapping
from io import StringIO
import keyword
import sys
import traceback
import types

# too few public methods.  come on, man.
# pylint: disable=R0903

### tcl support stuff - TclWriter class can plug python's stdout into Tcl's


class TclWriter:
    """TclWriter - class that can plug python's stdout into Tcl's"""

    def __init__(self):
        pass

    def write(self, string):
        """write a string, to Tcl"""
        call("puts", "-nonewline", string)
        # self.flush()

    def flush(self):
        """flush tcl's stdout"""
        call("flush", "stdout")


### trampoline support functions for tohil


# we call exec.  it's necessary.
# pylint: disable=W0122


def handle_exception(exception_type, val, traceback_object=None):
    """handle_exception - the tohil C code that handles uncaught
    python exceptions invokes this to turn an exception type, value
    and traceback object into a tcl error code and error info"""
    error_code = ["PYTHON", exception_type.__name__, val]

    if traceback_object is None:
        tb_list = list()
    else:
        tb_list = traceback.format_tb(traceback_object)
    error_info = "\nfrom python code executed by tohil" + " ".join(tb_list).rstrip()
    return error_code, error_info


def run(command):
    """run - perform exec but redirect stdout while
    python is running it into a string and return
    the string to run's caller after the exec has finished

    no need to worry about catching the exception; we'll
    let it go for tohil to take care of
    """
    try:
        their_stdout = sys.stdout
        my_stdout = StringIO()
        sys.stdout = my_stdout

        exec(command, globals(), locals())
    finally:
        sys.stdout = their_stdout

    return my_stdout.getvalue().rstrip()


def interact():
    """start interacting with the tcl interpreter"""
    eval("package require Tclx")
    call("commandloop", "-prompt1", 'return  " % "', "-prompt2", 'return "> "')


### shadow dictionaries


class ShadowDictIterator:
    """shadow dict iterator - one of these is returned by shadow dict
    iter function to iterate over a shadow dict"""

    def __init__(self, tcl_array):
        self.keys = call("array", "names", tcl_array, to=list)
        self.keys.sort()

    def __iter__(self):
        return self

    def __next__(self):
        if len(self.keys) == 0:
            raise StopIteration

        return self.keys.pop(0)


class ShadowDict(MutableMapping):
    """shadow dicts - python dict-like objects that shadow a tcl array"""

    def __init__(self, tcl_array, *, default=None, to=None):
        self.tcl_array = tcl_array
        if to is None:
            self.to_type = str
        else:
            self.to_type = to

        self.default = default

    def __getitem__(self, key):
        """access element of the shadow dict using the [] notation"""
        try:
            self.default
            return getvar(f"{self.tcl_array}({key})", to=self.to_type, default=self.default)
        except AttributeError:
            return getvar(f"{self.tcl_array}({key})", to=self.to_type)

    def __delitem__(self, key):
        """delete an item from the shadow dict"""
        unset(f"{self.tcl_array}({key})")
        return

    def __setitem__(self, key, value):
        """set an element of the shadow dict to the specified value"""
        setvar(f"{self.tcl_array}({key})", value)

    def __len__(self):
        """return the length of the shadow dict i.e. the size of the tcl array"""
        return call("array", "size", self.tcl_array, to=int)

    def __repr__(self):
        """return a representation of the shadow dict"""
        return str(call("array", "get", self.tcl_array, to=dict))

    def __iter__(self):
        """return a shadow dict iterator"""
        return ShadowDictIterator(self.tcl_array)

    def __contains__(self, key):
        """return true if the shadow dict has an element named key, else false"""
        return exists(f"{self.tcl_array}({key})")

    def clear(self):
        """remove all items from the shadow dictionary (unset the tcl array)"""
        call("array", "unset", self.tcl_array)

    def keys(self):
        """return a view of the ShadowDict's keys.  unlike dicts in later 3.x
        python, there come back in hash traversal i.e. basically random order"""
        return call("array", "names", self.tcl_array)

    def get(self, key, default=None, *, to=None):
        """return the value of an element of the shadow dict, with conversion
        to the specified type, if key is present in the tcl array.  if default
        is not given, it defaults to None, so this method never raises a KeyError."""

        if to is None:
            to = self.to_type
        if exists(f"{self.tcl_array}({key})"):
            return getvar(f"{self.tcl_array}({key})", to=to)
        return convert(default, to=to)

    def pop(self, key, *args, to=None):
        """if key is in the dictionary, remove it and return its value, else
        return default.  if default is not given and key is not in the dictionary,
        a KeyError is raised."""

        if len(args) > 1:
            raise TypeError(f"function takes one or two position arguments")
        if to is None:
            to = self.to_type

        var = f"{self.tcl_array}({key})"
        if exists(var):
            val = getvar(var, to=to)
            call("unset", var)
            return val
        if len(args) == 0:
            raise KeyError(key)
        if args[0] is None:
            return None
        return convert(args[0], to=to)

#
# misc stuff and helpers
#


def source(file_name, encoding=""):
    """source in a tcl file with optional specification of
    the encoding of the data stored in the file.  if encoding
    is not specified, the system encoding is assumed."""
    if encoding == "":
        return _tohil.call("source", file_name)
    else:
        return _tohil.call("source", "-encoding", encoding, file_name)


def package_require(package, version=""):
    """try to load the specified package."""
    if version == "":
        return _tohil.call("package", "require", package)
    else:
        return _tohil.call("package", "require", package, version)


def use_vhost(vhost=""):
    if vhost == "":
        vhost = "production"
    return tohil.call("use_vhost", vhost)


#
# TclError exception class, for when tcl gets a tcl error that
#   wasn't caught, so we can send it to python
#
class TclError(Exception):
    """TclError class - instances of this class are returned to python for uncaught
    errors received from the tcl side.

    the tcl error object will be populated with attributes like result, errorcode, code,
    level, errorstack, errorinfo, and errorline."""

    def __init__(self, result, return_options):
        self.result = result
        for key in return_options:
            value = return_options[key]
            key = key[1:]
            # print(f"setting attribute '{key}' to '{value}'")
            if key in ("code", "errorline", "level"):
                value = int(value)
            elif key == "errorcode":
                value = convert(value, to=list)
            self.__setattr__(key, value)

    def __repr__(self):
        """repr function"""
        return f"<class TclError '{self.result}' {self.errorcode}'>"


### rivet stuff


class RivetControl:
    """probably lame stuff to redirect python stdout to tcl,
    but only once"""

    def __init__(self):
        self.activated = False
        self.tcl_writer = TclWriter()

    def activate(self):
        """activate rivet control, but only do the work once"""
        if self.activated:
            return

        # plug the tcl writer into stdout
        sys.stdout = self.tcl_writer
        self.activated = True


# global rivet_control
rivet_control = RivetControl()


def rivet():
    """redirect python's stdout to write to tcl's stdout"""
    rivet_control.activate()


###
### import our C language stuff
###

# handle_exception must be defined before importing from
# _tohil, which triggers loading of the C shared library,
# which looks for it upon load

from tohil._tohil import (
    call,
    eval,
    exists,
    expr,
    getvar,
    interp,
    setvar,
    subst,
    unset,
    tclobj,
    tcldict,
    convert,
    incr,
    __version__,
)

###
### tcl proc importer and trampoline
###

tcl_init = """
proc safe_info_default {proc arg} {
    if {[info default $proc $arg var] == 1} {
        return [list 1 $var]
    }
    return [list 0 ""]
}
"""

_tohil.eval(tcl_init)

def tclvar(tcl_var_name, **kwargs):
    return tclobj(var=tcl_var_name, **kwargs)


def info_args(proc):
    """wrapper for 'info args'"""
    return call("info", "args", proc, to=list)


def info_procs(pattern=None, what="procs"):
    """wrapper for 'info procs' or whatever"""
    if pattern is None:
        return sorted(call("info", what, to=list))
    else:
        return sorted(call("info", what, pattern, to=list))


def info_commands(pattern=None):
    """wrapper for 'info commands'"""
    return info_procs(pattern, what="commands")


def info_body(proc):
    """wrapper for 'info body'"""
    return call("info", "body", proc, to=str)


def namespace_children(namespace):
    """wrapper for 'namespace children'"""
    return sorted(call("namespace", "children", namespace, to=list))


def info_default(proc, var):
    """wrapper for 'info default'"""
    return call("safe_info_default", proc, var, to=tuple)


def doublecolon_tail(string):
    """return the last (rightmost) part of a namespace or namespace-qualified proc"""
    last_colons = string.rfind("::")
    if last_colons >= 0:
        return string[last_colons + 2 :]
    return string


class TclProc:
    """instantiate with a tcl proc name as the argument.  the proc can be
    in any namespace but the name should be fully qualified.  although
    maybe not, not sure.

    the init routine determines if the argument is a proc or a tcl
    function written in C.  if it's a proc, it uses tcl's introspection
    to obtain the proc's arguments and default parameters, if any.

    we define the __call__ method to check whether the tcl target is a proc
    or C function and then call either our trampoline function or
    passthrough_trampoline function, below.
    """

    def __init__(self, proc, to=str):
        self.proc = proc
        self.function_name = self._proc_to_function(proc)

        try:
            self.proc_args = info_args(proc)
            self.is_proc = True
        except TclError:
            # print(f"info args failed for proc '{proc}'")
            self.is_proc = False

        self.to_type = to

        if self.is_proc:
            # self.body = info_body(proc)
            self.defaults = dict()

            for arg in self.proc_args:
                has_default, default_value = info_default(proc, arg)
                if int(has_default):
                    self.defaults[arg] = default_value

    def _proc_to_function(self, proc):
        """conver ta tcl proc name to a python function name"""
        function = doublecolon_tail(proc)
        # python doesn't like dashes or colons in function names, so we map to underscores.
        # "::" will map to "__" -- i think that's reasonable, at least for now.
        # some other characters also appear in some tcl proc names out there, so we map
        # them to other stuff.  tcl is too permissive, i feel like.
        function = (
            function.replace("-", "_")
            .replace(":", "_")
            .replace("?", "_question_mark")
            .replace("+", "_plus_sign")
            .replace("<", "_less_than")
            .replace("@", "_at_sign")
            .replace(">", "_greater_than")
        )
        return function

    def __repr__(self):
        """repr function"""
        return f"<class 'TclProc' '{self.proc}', args '{repr(self.proc_args)}>"

    def set_to(to):
        self.to_type = to

    def __call__(self, *args, **kwargs):
        """this gets invoked when they call the object as if it is
        a function.  we then return the results of our trampoline
        or passthrough trampoline method, depending on if this instance
        ingested a proc or C function"""
        if self.is_proc:
            return self.trampoline(args, kwargs)
        else:
            return self.passthrough_trampoline(args, kwargs)

    def passthrough_trampoline(self, args, kwargs):
        """passthrough trampoline function is for calling C functions on the tcl
        side where we don't know anything about what arguments it takes so we
        treat everything as positional and pass through exactly what we get

        but we still support the to= conversion... :-)"""
        if "to" in kwargs:
            to_type = kwargs["to"]
            del kwargs["to"]
        else:
            to_type = self.to_type

        if len(kwargs) > 0:
            raise TypeError(
                f"can't specify named parameters to a tcl function that isn't a proc: '{self.proc}'"
            )
        return call(self.proc, *args, to=to_type)

    def trampoline(self, args, kwargs):
        """trampoline function takes our proc probe data, positional parameters
        and named parameters, figures out if everything's there that the proc
        needs and calls the proc, or generates an exception for missing parameters,
        too many parameters, unrecognized parameters, etc

        if performance becomes an issue, this should be able to be converted to
        C without great difficulty.  it's good in python while we figure it all out.
        """
        final = dict()

        # print(f"trampoline invoked, self '{self}', args '{args}', kwargs '{kwargs}', defaults '{self.defaults}'")

        if "to" in kwargs:
            to_type = kwargs["to"]
            del kwargs["to"]
            # print(f"set aside to  of {to_type}")
        else:
            to_type = self.to_type

        nargs = len(args)
        if nargs + len(kwargs) > len(self.proc_args) and self.proc_args[-1] != "args":
            raise TypeError(
                f"too many arguments specified to be passed to tcl proc '{self.proc}'"
            )

        # pump any named parameters into the "final" dict
        # print("checking named parameters")
        for arg_name, arg in kwargs.items():
            if arg_name in final:
                raise TypeError(
                    f"parameter '{arg_name}' specified multiple times -- can only specify it once"
                )
            if arg_name not in self.proc_args:
                raise TypeError(
                    f"named parameter '{arg_name}' is not a valid arument for proc '{self.proc}'"
                )
            # print(f"trampoline filling in named parameter {arg_name}, '{repr(arg)}'")
            final[arg_name] = arg

        # pump the positional arguments into the "final" dict
        # these args are positional from the python side; python already split out
        # named parameters into kwargs before calling us.
        #
        # advance, matching the tcl arg names to the args tuple, but if an
        # arg name is already in the final array due to having come from
        # a named parameter, advance to the next argument, without advancing
        # the args list.
        # print("checking positional parameters")
        pos = 0
        # for arg_name, arg in zip(self.proc_args, args):
        for arg_name in self.proc_args:
            if pos >= nargs:
                break
            if arg_name != "args":
                if arg_name not in final:
                    # a position parameter has been fulfilled
                    final[arg_name] = args[pos]
                    # print(f"trampoline filling in position arg {arg_name}, '{repr(args[pos])}'")
                else:
                    # already have this from a named parameter, skip but
                    # keep the current positional parameter for the next arg
                    continue
            else:
                # this argument is the tcl-special "args",
                # grab the remainder of the arg list into args and stop iterating.
                # we'll make use of this to handle args correctly in our call to tcl.
                final[arg_name] = args[pos:]
                break
            pos += 1

        # pump any default values, if needed, by checking for the existence
        # of the default values' var names in the final array.  if it isn't
        # there, put it there.  if it is there, the default isn't needed.
        # print("checking defaults")
        for arg_name, def_value in self.defaults.items():
            if arg_name not in final:
                # print(f"trampoline filling in default value {arg_name}, '{def_value}'")
                final[arg_name] = def_value

        # make sure we've got something for each of the proc's argument - if anything are
        # missing, it's an error.
        for arg_name in self.proc_args:
            # it's ok if args is missing
            if not arg_name in final and arg_name != "args":
                raise TypeError(f"required arg '{arg_name}' missing")

        # assemble the final argument list in the correct order for the proc
        final_arg_list = list()
        for arg_name in self.proc_args:
            if arg_name != "args":
                final_arg_list.append(final[arg_name])
            else:
                # it's "args". if we have args (because we may not, if there
                # weren't enough arguments specified to have there be one), extend
                # the final_arg_list with them, i.e. add them flat instead of nested,
                # the way tcl will expect the args args
                if "args" in final:
                    final_arg_list.extend(final[arg_name])

        # ...and invoke the proc
        # print(f"trampoline calling {self.proc} with final of '{repr(final_arg_list)}'")
        return call(self.proc, *final_arg_list, to=to_type)


class TclNamespace:
    """tcl namespace class -- one instance corresponds to a tcl namespace

    it has the facility to import all the procs and C commands as TclProc
    objects and keep track of them from the namespace



    """

    proc_excluder = keyword.kwlist

    def __init__(self, namespace):
        # print(f"{self} importing namespace '{namespace}'")
        # be able to find TclProcs by proc name and function name, for convenience,
        # not actually used for anything yet
        self.__tohil_procs__ = dict()
        self.__tohil_functions__ = dict()

        # keep track of subordinate namespaces
        self.__tohil_namespaces__ = dict()
        self.__tohil_import_namespace__(namespace)
        # print(f"{self} done importing namespace '{namespace}'")

    def __tohil_import_proc__(self, proc):
        """create a callable TclProc object corresponding to "proc",
        which can be a tcl proc or a C command"""
        tclproc = TclProc(proc)
        # print(f"setting name '{tclproc.function_name}'")
        self.__tohil_procs__[proc] = tclproc
        self.__tohil_functions__[tclproc.function_name] = tclproc

        # create a new method in this tcl namespace comprising the
        # tclproc function name and tclproc object we just created.
        #
        # when this namespace object.function_name() is invoked, it'll
        # be called as a method of tclproc, i.e. self will be the first
        # argument and will be the tclproc object ergo we can get to the
        # trampoline and other stuff about the proc like its arguments,
        # defaults, etc, because self is us.
        self.__setattr__(
            tclproc.function_name,
            tclproc
            # tclproc.function_name, types.MethodType(tclproc, tclproc)
        )

    def __tohil_import_procs__(self, pattern=None):
        """import all the procs commands in one namespace"""
        # print(f"    importing procs pattern '{pattern}', '{info_commands(pattern)}'")
        for proc in info_commands(pattern):
            # NB this excluder stuff is a little clumsy, but if it was
            # in the TclProc init routine then wouldn't that routine
            # have to raise an exception if it didn't want the thing created?
            if proc.startswith("::tcl::mathop::"):
                continue
            if doublecolon_tail(proc) in TclNamespace.proc_excluder:
                continue
            try:
                self.__tohil_import_proc__(proc)
            except Exception as exception:
                # except NameError:
                # DES::Pad has a null byte for a default argument
                if proc != "::DES::Pad":
                    print(
                        f"failed to import proc '{proc}', exception '{exception}', continuing...",
                        file=sys.stderr,
                    )
        # print(f"    done importing procs pattern '{pattern}'")

    def __tohil_import_namespace__(self, namespace="::"):
        """import from a tcl namespace.  create a TclNamespace object
        and import all the procs into it and return it

        and import all the subordinate namespaces into them as well"""

        self.__tohil_import_procs__(namespace + "::*")

        for child in namespace_children(namespace):
            # print(f"  importing child namespace {child}")
            new_namespace = TclNamespace(child)

            child_short = doublecolon_tail(child)
            # print(f"  {self} storing child {child_short} namespace {new_namespace}")
            self.__setattr__(child_short, new_namespace)


def import_tcl():
    return TclNamespace("")


### end of trampoline stuff
