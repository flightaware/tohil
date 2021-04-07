""" tohil """

from collections.abc import MutableMapping
from io import StringIO
import sys
import traceback
import types

# too few public methods.  come on, man.
#pylint: disable=R0903

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
#pylint: disable=W0122

def handle_exception(exception_type, val, traceback_object = None):
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

### tclobj iterator

class TclObjIterator():
    """tclobj iterator - one of these is returned by tclobj
    iter function to iterate over a tclobj"""
    def __init__(self, tclobj):
        self.tclobj = tclobj
        self.index = -1

    def __iter__(self):
        self.index = -1
        return self

    def __next__(self):
        self.index += 1
        if self.index >= self.tclobj.llength():
            raise StopIteration

        return self.tclobj.lindex(self.index)

### shadow dictionaries

class ShadowDictIterator():
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
    def __init__(self, tcl_array, to=str):
        self.tcl_array = tcl_array
        self.to_type = to

    def __getitem__(self, key):
        return getvar(f"{self.tcl_array}({key})", to=self.to_type)

    def __delitem__(self, key):
        unset(f"{self.tcl_array}({key})")
        return

    def __setitem__(self, key, value):
        setvar(f"{self.tcl_array}({key})", value)

    def __len__(self):
        return call("array", "size", self.tcl_array, to=int)

    def __repr__(self):
        return str(call("array", "get", self.tcl_array, to=dict))

    def __iter__(self):
        return ShadowDictIterator(self.tcl_array)

    def __contains__(self, key):
        return exists(f"{self.tcl_array}({key})")

#
# misc stuff and helpers
#

def package_require(package, version=''):
    return _tohil.eval(f"package require {package} {version}")

def use_vhost(vhost=''):
    if vhost == '':
        vhost = 'production'
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

    def __init__(self, result, td_obj):
        #print(f"TclError executing with self '{self}', result '{result}', td_obj '{td_obj}'")
        self.result = result
        err_pairs = td_obj.as_list()
        for key, value in zip(err_pairs[::2], err_pairs[1::2]):
            key = key[1:]
            #print(f"setting attribute '{key}' to '{value}'")
            if key in ("code", "errorline", "level"):
                value = int(value)
            elif key == "errorcode":
                value = convert(value, to=list)
            self.__setattr__(key, value)


    def __repr__(self):
        """repr function"""
        return(f"<class TclError '{self.result}' {self.errorcode}'>")

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


#global rivet_control
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
    convert,
    incr,
)

###
### tcl proc importer and trampoline
###

tcl_init = '''
proc safe_info_default {proc arg} {
    if {[info default $proc $arg var] == 1} {
        return [list 1 $var]
    }
    return [list 0 ""]
}
'''

_tohil.eval(tcl_init)

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
    return info_procs(pattern, what="commands")

def info_body(proc):
    return call("info", "body", proc, to=str)

def namespace_children(namespace):
    return sorted(call("namespace", "children", namespace, to=list))

def info_default(proc, var):
    """wrapper for 'info default'"""
    return call("safe_info_default", proc, var, to=tuple)


class TclProc:
    """instantiate with a tcl proc name as the argument.  the proc can be
    in any namespace but the name should be fully qualified.  although
    maybe not, not sure.

    the init routine uses tcl introspection to get the proc's arguments
    and default values

    one it's done this, it generates the function using its gen_proc_function()
    method and execs it into python.  this function when called from
    python invokes the trampoline function below to give python really
    nice handling of tcl proc arguments, better than most python functions
    implement.

    typically this class will be instantiated by running the probe_proc method,
    or the methods that sit above it
    """
    def __init__(self, proc, to_type=str):
        self.proc = proc
        self.function_name = self._proc_to_function(proc)

        try:
            self.proc_args = info_args(proc)
            is_proc = True
        except RuntimeError:
            print(f"info args failed for proc '{proc}'")
            is_proc = False

        self.to_type = to_type

        if not is_proc:
            self.wrapper_source = self.gen_passthrough_function()
        else:
            #self.body = info_body(proc)
            self.defaults = dict()

            for arg in self.proc_args:
                has_default, default_value = info_default(proc,arg)
                if int(has_default):
                    self.defaults[arg] = default_value

            self.wrapper_source = self.gen_proc_function()

        # compile the function and grab a reference to it
        locs = dict()
        #print(self.wrapper_source)
        exec(self.wrapper_source, globals(), locs)
        self.function = locs[self.function_name]

    def _proc_to_function(self, proc):
        """conver ta tcl proc name to a python function name"""
        last_colons = proc.rfind("::")
        if last_colons >= 0:
            function = proc[last_colons + 2:]
        else:
            function = proc
        # python doesn't like dashes or colons in function names, so we map to underscores.
        # "::" will map to "__" -- i think that's reasonable, at least for now.
        # some other characters also appear in some tcl proc names out there, so we map
        # them to other stuff.  tcl is too permissive, i feel like.
        function = function.replace("-", "_").replace(":", "_").replace("?", "_question_mark").replace("+", "_plus_sign").replace("<", "_less_than").replace("@", "_at_sign").replace(">", "_greater_than")
        return function

    def __repr__(self):
        """repr function"""
        return(f"<class 'TclProc' '{self.proc}', args '{repr(self.proc_args)}>")

    def set_to(to):
        self.to = to

    def gen_passthrough_function(self):
        """generate a python function for a non-proc that calls our trampoline"""
        # if function is defined outside the tohil namespace, return tohil.procs... not procs...
        string = f"def {self.function_name}(self, *args, **kwargs):\n"
        string += f"    return self.passthrough_trampoline(args, kwargs)\n\n"
        return string

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
            raise TypeError(f"can't specify named parameters to a tcl function that isn't a proc: '{self.proc}'")
        return call(self.proc, *args, to=to_type)

    def gen_proc_function(self):
        """generate a python function for the proc that will call our trampoline and execute tcl"""
        # if function is defined outside the tohil namespace, return tohil.procs... not procs...
        string = f"def {self.function_name}(self, *args, **kwargs):\n"
        string += f"""    print(f"wrapper called for {self} {self.function_name}""" """(args='{args}', kwargs='{kwargs}')")\n"""
        string += f"    return self.trampoline(args, kwargs)\n\n"
        return string


    def trampoline(self, args, kwargs):
        """trampoline function takes our proc probe data, positional parameters
        and named parameters, figures out if everything's there that the proc
        needs and calls the proc, or generates an exception for missing parameters,
        too many parameters, unrecognized parameters, etc

        if performance becomes an issue, this should be able to be converted to
        C without great difficulty.  it's good in python while we figure it all out.
        """
        final = dict()

        nargs = len(args)
        if nargs + len(kwargs) > len(self.proc_args) and self.proc_args[-1] != "args":
            raise TypeError(f"too many arguments specified to be passed to tcl proc '{self.proc}'")

        if "to" in kwargs:
            to_type = kwargs["to"]
            del kwargs["to"]
        else:
            to_type = self.to_type

        # pump any named parameters into the "final" dict
        for arg_name, arg in kwargs.items():
            if arg_name in final:
                raise TypeError(f"parameter '{arg_name}' specified multiple times -- can only specify it once")
            if arg_name not in self.proc_args:
                raise TypeError(f"named parameter '{arg_name}' is not a valid arument for proc '{self.proc}'")
            #print(f"trampoline filling in named parameter {arg_name}, '{repr(arg)}'")
            final[arg_name] = arg

        # pump the positional arguments into the "final" dict
        # these args are positional from the python side; python already split out
        # named parameters into kwargs before calling us.
        #
        # advance matching the tcl arg names to the args tuple, but if an
        # arg name is already in the final array due to having come from
        # a named parameter, advance to the next argument, without advancing
        # the args list
        pos = 0
        #for arg_name, arg in zip(self.proc_args, args):
        for arg_name in self.proc_args:
            if pos >= nargs:
                if arg_name == "args":
                    break
                raise TypeError(f"not enough parameters to '{self.proc}' ({self.proc_args}), you provided {nargs + len(kwargs)}")
            #print(f"trampoline filling in position arg {arg_name}, '{repr(arg)}'")
            if arg_name != "args":
                if arg_name not in final:
                    # a position parameter has been fulfilled
                    final[arg_name] = args[pos]
                else:
                    # already have this from a named parameter, skip but
                    # keep the positional parameter for the next arg
                    continue
            else:
                # this argument is the tcl-special "args",
                # grab the remainder of the arg list into args and stop iterating.
                # we'll make use of this to handle args correctly in our call to tcl.
                final[arg_name] = args[pos:]
                break
            pos += 1

        # pump any default values if needed
        for arg_name, def_value in self.defaults.items():
            if arg_name not in final:
                #print(f"trampoline filling in default value {arg_name}, '{def_value}'")
                final[arg_name] = def_value

        # make sure we've got everything
        for arg_name in self.proc_args:
            # it's ok if args is missing
            if not arg_name in final and arg_name != "args":
                raise TypeError(f"required arg '{arg_name}' missing")

        # assemble the final argument list
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

        #print(f"trampoline calling {self.proc} with final of '{repr(final_arg_list)}'")
        return call(self.proc, *final_arg_list, to=to_type)

class TclNamespace:
    def __init__(self, namespace):
        print(f"{self} importing namespace '{namespace}'")
        # be able to find TclProcs by proc name and function name, for convenience,
        # not actually used for anything yet
        self.__tohil_procs__ = dict()
        self.__tohil_functions__ = dict()

        # keep track of subordinate namespaces
        self.__tohil_namespaces__ = dict()
        self.__tohil_import_namespace__(namespace)
        print(f"{self} done importing namespace '{namespace}'")

    def __tohil_import_proc__(self, proc):
        # strip off namespace qualifiers
        #print(f"        importing proc '{proc}'")
        #last_colons = proc.rfind("::")
        #if last_colons >= 0:
        #    short_proc = proc[last_colons + 2:]
        #else:
        #    short_proc = proc

        tclproc = TclProc(proc)
        #print(f"setting name '{tclproc.function_name}'")
        self.__tohil_procs__[proc] = tclproc
        self.__tohil_functions__[tclproc.function_name] = tclproc

        # create a new method in this tcl namespace comprising the
        # tclproc function name and tclproc function we just compiled.
        # when this namespace object.function_name() is invoked, it'll
        # be called as a method of tclproc, i.e. self will be the first
        # argument and will be the tclproc object ergo we can get to the
        # trampoline and other stuff about the proc like its arguments,
        # defaults, etc.
        self.__setattr__(tclproc.function_name, types.MethodType(tclproc.function, tclproc))

    def __tohil_import_procs__(self, pattern=None):
        """import all the commands in one namespace"""
        print(f"    importing procs pattern '{pattern}', '{info_commands(pattern)}'")
        for proc in info_commands(pattern):
            try:
                self.__tohil_import_proc__(proc)
            except Exception as exception:
                #except NameError:
                # DES::Pad has a null byte for a default argument
                if proc != "::DES::Pad":
                    print(f"failed to import proc '{proc}', exception '{exception}', {getvar('::errorInfo')} continuing...", file=sys.stderr)
        print(f"    done importing procs pattern '{pattern}'")

    def __tohil_import_namespace__(self, namespace="::"):
        """import from a tcl namespace.  create a TclNamespace object
        and import all the procs into it and return it

        and import all the subordinate namespaces into them as well"""

        self.__tohil_import_procs__(namespace + "::*")

        for child in namespace_children(namespace):
            print(f"  importing child namespace {child}")
            new_namespace = TclNamespace(child)

            last_colons = child.rfind("::")
            if last_colons >= 0:
                child = child[last_colons + 2:]

            print(f"  {self} storing child {child} namespace {new_namespace}")
            self.__setattr__(child, new_namespace)

def import_tcl():
    return TclNamespace("")

### end of trampoline stuff

