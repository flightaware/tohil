

from collections.abc import MutableMapping

import tohil

class ShadowDictIterator():
    def __init__(self, tcl_array):
        self.keys = tohil.eval(f"array names {tcl_array}", to=list)
        self.keys.sort()

    def __iter__(self):
        return self

    def __next__(self):
        if len(self.keys) == 0:
            raise StopIteration

        return self.keys.pop(0)

class ShadowDict(MutableMapping):
    def __init__(self, tcl_array):
        self.tcl_array = tcl_array

    def __getitem__(self, key):
        return tohil.getvar(f"{self.tcl_array}({key})")

    def __delitem__(self, key):
        tohil.unset(f"{self.tcl_array}({key})")
        return

    def __setitem__(self, key, value):
        tohil.setvar(f"{self.tcl_array}({key})", value)

    def __len__(self):
        return tohil.eval(f"array size {self.tcl_array}")

    def __repr__(self):
        return str(tohil.eval(f"array get {self.tcl_array}", to=dict))

    def __iter__(self):
        return ShadowDictIterator(self.tcl_array)

