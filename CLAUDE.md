# CLAUDE.md - Tohil Project Overview

## What is Tohil?

Tohil is a bidirectional integration layer between Python and Tcl. It is simultaneously:
- A Python C extension module (`tohil._tohil`)
- A Tcl extension package (`package require tohil`)

This allows Python code to call Tcl and vice versa, with seamless data type conversions and proper error handling across both languages.

## Project Structure

```
tohil/
├── generic/tohil.c      # Main C implementation (both Python and Tcl extensions)
├── pysrc/tohil/         # Python package wrapper
│   └── __init__.py      # Python-side classes (TclProc, ShadowDict, TclError, etc.)
├── tclsrc/              # Tcl-side support code
├── configure.ac         # Autoconf configuration (TEA-based build)
├── Makefile.in          # Build template
├── setup.py.in          # Python setuptools template
└── Doc/                 # Sphinx documentation
```

## Building

```bash
autoreconf              # Generate configure script (requires autoconf 2.69+)
./configure --with-python-version=3.X
make
make test
sudo make install
```

## Key Concepts

### Python Types
- `tohil.tclobj` - Python wrapper around a Tcl object (Tcl_Obj)
- `tohil.tcldict` - Python wrapper for Tcl dict operations
- `tohil.TclError` - Exception class for Tcl errors
- `tohil.ShadowDict` - Python dict that shadows a Tcl array

### Reference Counting
The code manages reference counts for **both** languages:
- Python: `Py_INCREF`, `Py_DECREF`, `Py_XINCREF`, `Py_XDECREF`
- Tcl: `Tcl_IncrRefCount`, `Tcl_DecrRefCount`

### Subinterpreter Support
Tohil supports Python subinterpreters. Each Tcl interpreter can have an associated Python subinterpreter stored via `Tcl_SetAssocData`.

## Code Formatting

```bash
clang-format -style=file -i generic/tohil.c
```

## Testing

```bash
make test
```

Tests use Python's hypothesis framework for property-based testing.

## Key Functions in generic/tohil.c

### Entry Points
- `PyInit__tohil()` - Python module initialization
- `Tohil_Init()` - Tcl package initialization
- `tohil_mod_exec()` - Module population (multiphase init)

### Type Conversions
- `pyObjToTcl()` - Convert Python object to Tcl_Obj
- `tohil_python_return()` - Convert Tcl result to Python with type conversion
- `tclListObjToPyListObject()` - Tcl list to Python list
- `tclListObjToPyDictObject()` - Tcl key-value list to Python dict

### Python to Tcl Commands
- `tohil_eval()` - Evaluate Tcl code from Python
- `tohil_call()` - Call Tcl command with arguments
- `tohil_getvar()`/`tohil_setvar()` - Get/set Tcl variables

### Tcl to Python Commands
- `TohilEval_Cmd` - `tohil::eval` - Evaluate Python expression
- `TohilExec_Cmd` - `tohil::exec` - Execute Python statements
- `TohilCall_Cmd` - `tohil::call` - Call Python function

## Common Patterns

### Error Handling
```c
// From Tcl calling Python
if (PyErr_Occurred() != NULL) {
    return Tohil_ReturnExceptionToTcl(interp, prior, "description");
}

// From Python calling Tcl
if (tcl_result == TCL_ERROR) {
    // tohil_python_return() will set PyErr with TclError
}
```

### Thread State Management
When Tcl calls into Python, the code swaps to the correct Python subinterpreter:
```c
PyThreadState *prior = tohil_swap_subinterp(interp);
// ... do Python stuff ...
tohil_restore_subinterp(prior);
```

## Known Issues / Areas of Concern

### Memory Management
The code has complex reference counting across two garbage-collected runtimes. Key areas to audit:
- `TohilTclObj_dealloc()` - Python tclobj destructor
- `TohilTclObjIter_dealloc()` - Iterator destructor
- `TohilTclObj_objptr_for_write()` - Shared object handling
- Conversion functions like `_pyObjToTcl()` and `tohil_python_return()`
