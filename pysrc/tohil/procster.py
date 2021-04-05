

import tohil

tcl_init = '''
proc safe_info_default {proc arg} {
    if {[info default $proc $arg var] == 1} {
        return [list 1 $var]
    }
    return [list 0 ""]
}
'''

tohil.eval(tcl_init)

def info_args(proc):
    return tohil.call("info", "args", proc, to=list)

def info_procs():
    return tohil.call("info", "procs", to=list)

def info_body(proc):
    return tohil.call("info", "body", proc, to=str)

def info_default(proc, var):
    return tohil.call("safe_info_default", proc, var, to=tuple)

class TclProc:
    def __init__(self, proc):
        self.proc = proc
        self.proc_args = info_args(proc)
        #self.body = info_body(proc)
        self.args = dict()
        self.defaults = dict()

        for arg in self.proc_args:
            has_default, default_value = info_default(proc,arg)
            print(f"proc {self.proc}, arg {arg}, has_default {has_default}, default_value {default_value}")
            if int(has_default):
                self.defaults[arg] = default_value

        #print(f"def-trampoline-func: {self.gen_function()}")
        #exec(self.gen_function())

    def __repr__(self):
        return(f"proc '{self.proc}', args '{repr(self.args)}', defaults '{repr(self.defaults)}'")

    def gen_function(self):
        string = f"def {self.proc}(*args, **kwargs):\n"
        string += f"    return tohil.procster.procs.procs['{self.proc}'].trampoline(args, kwargs)\n\n"
        return string


    #
    # the actual trampoline
    #
    def trampoline(self, args, kwargs):
        final = dict()

        if len(args) > len(self.proc_args):
            raise Exception("too many arguments")

        # pump the positional arguments into the "final" dict
        for arg_name, arg in zip(self.proc_args, args):
            print(f"trampoline filling in position arg {arg_name}, '{repr(arg)}'")
            final[arg_name] = arg

        # pump any named parameters into the "final" dict
        for arg_name, arg in kwargs.items():
            if arg_name in final:
                raise Exception(f"arg {arg_name} specified positionally and by name and that's ambiguous, so no")
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
                raise Exception(f"required arg '{arg_name}' missing")

        print(f"trampoline has final of '{repr(final)}' and is calling the values")
        return tohil.call(self.proc, *final.values())

class TclProcSet:
    def __init__(self):
        self.procs = dict()

    def probe_proc(self, proc):
        self.procs[proc] = TclProc(proc)
        return self.procs[proc].gen_function()

    def probe_procs(self):
        string = ''
        for proc in info_procs():
            string += self.probe_proc(proc)
        return string


procs = TclProcSet()

print("maybe try tohil.procster.procs.probe_procs(), then tohil.procster.procs['sin']")

def package_require(package, version=''):
    return tohil.eval(f"package require {package} {version}")

def use_vhost(vhost=''):
    if vhost == '':
        vhost = 'production'
    return tohil.call("use_vhost", vhost)

def test():
    package_require('flightaware')
    use_vhost("production")
    package_require('fa_simulate_web_environment')


if __name__ == "__main__":
    pass
