""" tohil """

from collections.abc import MutableMapping
from io import StringIO
import sys
import traceback

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
### procster
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

def info_procs(pattern=None):
    """wrapper for 'info procs'"""
    if pattern is None:
        return call("info", "procs", to=list)
    else:
        return call("info", "procs", pattern, to=list)

def info_body(proc):
    return call("info", "body", proc, to=str)

def namespace_children(namespace):
    return call("namespace", "children", namespace, to=list)

def info_default(proc, var):
    """wrapper for 'info default'"""
    return call("safe_info_default", proc, var, to=tuple)

class TclProc:
    """probe results and trampoline for a single proc"""
    def __init__(self, proc):
        self.proc = proc
        self.function = self._proc_to_function(proc)
        self.proc_args = info_args(proc)
        #self.body = info_body(proc)
        self.defaults = dict()

        for arg in self.proc_args:
            has_default, default_value = info_default(proc,arg)
            print(f"proc {self.proc}, arg {arg}, has_default {has_default}, default_value {default_value}")
            if int(has_default):
                self.defaults[arg] = default_value

        #print(f"def-trampoline-func: {self.gen_function()}")
        #exec(self.gen_function())

    def _proc_to_function(self, proc):
        """conver ta tcl proc name to a python function name"""
        function = proc
        if function[:2] == "::":
            function = function[2:]
        function = function.replace("-", "_").replace("::","__")
        return function

    def __repr__(self):
        """repr function"""
        return(f"<class 'TclProc' '{self.proc}', args '{repr(self.proc_args)}', defaults '{repr(self.defaults)}'>")

    def gen_function(self):
        """generate a python function for the proc that calls our trampoline"""
        string = f"def {self.function}(*args, **kwargs):\n"
        string += f"    return tohil.procs.procs['{self.proc}'].trampoline(args, kwargs)\n\n"
        return string


    def trampoline(self, args, kwargs):
        """trampoline function takes our proc probe data, positional parameters
        and named parameters, figures out if everything's there that the proc
        needs and calls the proc, or generates an exception for missing parameters,
        too many parameters, unrecognized parameters, etc"""
        final = dict()

        if len(args) > len(self.proc_args):
            raise TypeError("too many arguments")

        # pump the positional arguments into the "final" dict
        for arg_name, arg in zip(self.proc_args, args):
            print(f"trampoline filling in position arg {arg_name}, '{repr(arg)}'")
            final[arg_name] = arg

        # pump any named parameters into the "final" dict
        for arg_name, arg in kwargs.items():
            if arg_name in final:
                raise TypeError(f"parameter '{arg_name}' specified positionally and by name and that's ambiguous, so no")
            if arg_name not in self.proc_args:
                raise TypeError(f"named parameter '{arg_name}' is not a valid arument for proc '{self.proc}'")
            print(f"trampoline filling in named parameter {arg_name}, '{repr(arg)}'")
            final[arg_name] = arg

        # pump any default values if needed
        for arg_name, def_value in self.defaults.items():
            if arg_name not in final:
                print(f"trampoline filling in default value {arg_name}, '{def_value}'")
                final[arg_name] = def_value

        # make sure we've got everything
        for arg_name in self.proc_args:
            if not arg_name in final:
                raise TypeError(f"required arg '{arg_name}' missing")

        print(f"trampoline has final of '{repr(final)}' and is calling the values")
        return call(self.proc, *final.values())

class TclProcSet:
    """holds in its procs dict a TclProc object for each proc probed"""
    def __init__(self):
        self.procs = dict()

    def probe_proc(self, proc):
        self.procs[proc] = TclProc(proc)
        return self.procs[proc].gen_function()

    def probe_procs(self, pattern=None):
        string = ''
        for proc in info_procs(pattern):
            string += self.probe_proc(proc)
        return string

    def probe_namespace(self, namespace):
        """probe a namespace and probe any procs found and
        then recursively probe any child namespaces of the
        specified namespace"""
        print(f"probing namespace '{namespace}'")
        string = self.probe_procs(namespace + "::*")
        for child in namespace_children(namespace):
            string += self.probe_namespace(child)

        return string



procs = TclProcSet()

def probe_procs():
    return procs.probe_procs()

#print("maybe try tohil.procs.probe_procs(), then tohil.procs['sin']")

### end of trampoline stuff

