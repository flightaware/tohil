// tohil C interface library
//
// ...contains all the C code for both Python and TCL
//
// GO TOE HEELS
//
// There is also python support code in pysrc/tohil,
// and TCL support code in tclsrc
//
// https://github.com/flightaware/tohil
//

// include Python.h before including any standard header files
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <tcl.h>

#include <assert.h>
#include <dlfcn.h>

#include <stdio.h>

// forward definitions

// tcl obj python data type
typedef struct {
	PyObject_HEAD
	Tcl_Obj *tclobj;
} PyTclObj;

int PyTclObj_Check(PyObject *pyObj);
static PyTypeObject PyTclObjType;

PyObject *
tohil_python_return(Tcl_Interp *interp, int tcl_result, PyObject *toType, Tcl_Obj *resultObj);

static PyObject * PyTclObj_FromTclObj(Tcl_Obj *obj);

/* TCL library begins here */

Tcl_Interp *tcl_interp = NULL;
static PyObject *pTohilHandleException = NULL;

// turn a tcl list into a python list
PyObject *
tclListObjToPyListObject(Tcl_Interp *interp, Tcl_Obj *inputObj) {
	Tcl_Obj **list;
	int count;

	if (Tcl_ListObjGetElements(interp, inputObj, &count, &list) == TCL_ERROR) {
		PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
		return NULL;
	}

	PyObject *plist = PyList_New(count);

	for (int i = 0; i < count; i++) {
		PyList_SET_ITEM(plist, i, Py_BuildValue("s", Tcl_GetString(list[i])));
	}

	return plist;
}

// turn a tcl list into a python set
PyObject *
tclListObjToPySetObject(Tcl_Interp *interp, Tcl_Obj *inputObj) {
	Tcl_Obj **list;
	int count;

	if (Tcl_ListObjGetElements(interp, inputObj, &count, &list) == TCL_ERROR) {
		PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
		return NULL;
	}

	PyObject *pset = PySet_New(NULL);

	for (int i = 0; i < count; i++) {
		if (PySet_Add(pset, Py_BuildValue("s", Tcl_GetString(list[i]))) < 0) {
			return NULL;
		}
	}

	return pset;
}

// turn a tcl list into a python tuple
PyObject *
tclListObjToPyTupleObject(Tcl_Interp *interp, Tcl_Obj *inputObj) {
	Tcl_Obj **list;
	int count;

	if (Tcl_ListObjGetElements(interp, inputObj, &count, &list) == TCL_ERROR) {
		PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
		return NULL;
	}

	PyObject *ptuple = PyTuple_New(count);

	for (int i = 0; i < count; i++) {
		PyTuple_SET_ITEM(ptuple, i, Py_BuildValue("s", Tcl_GetString(list[i])));
	}

	return ptuple;
}


// turn a tcl list of key-value pairs into a python dict
PyObject *
tclListObjToPyDictObject(Tcl_Interp *interp, Tcl_Obj *inputObj) {
	Tcl_Obj **list;
	int count;

	if (Tcl_ListObjGetElements(interp, inputObj, &count, &list) == TCL_ERROR) {
		PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
		return NULL;
	}

	if (count % 2 != 0) {
		// list doesn't have an even number of elements
		PyErr_SetString(PyExc_RuntimeError, "list doesn't have an even number of elements");
		return NULL;
	}

	PyObject *pdict = PyDict_New();

	for (int i = 0; i < count; i += 2) {
		PyDict_SetItem(pdict,
					   Py_BuildValue("s", Tcl_GetString(list[i])),
					   Py_BuildValue("s", Tcl_GetString(list[i+1]))
					   );
	}

	return pdict;
}

// turn a tcl object into a python object by trying to convert it as a boolean,
// then a long, then a double and finally a string
static PyObject *
tclObjToPy(Tcl_Obj *tObj) {
	int intValue;
	long longValue;
	double doubleValue;

	if (Tcl_GetBooleanFromObj(NULL, tObj, &intValue) == TCL_OK) {
		PyObject *p = (intValue ? Py_True : Py_False);
		Py_INCREF(p);
		return p;
	}

	if (Tcl_GetLongFromObj(NULL, tObj, &longValue) == TCL_OK) {
		return PyLong_FromLong(longValue);
	}

	if (Tcl_GetDoubleFromObj(NULL, tObj, &doubleValue) == TCL_OK) {
		return PyFloat_FromDouble(doubleValue);
	}

	int tclStringSize;
	char *tclString;
	tclString = Tcl_GetStringFromObj(tObj, &tclStringSize);
	return Py_BuildValue("s#", tclString, tclStringSize);
}

// convert a python object to a tcl object - amazing code by aidan
static Tcl_Obj *
pyObjToTcl(Tcl_Interp *interp, PyObject *pObj)
{
	Tcl_Obj *tObj;
	PyObject *pBytesObj;
	PyObject *pStrObj;

	Py_ssize_t i, len;
	PyObject *pVal = NULL;
	Tcl_Obj *tVal;

	PyObject *pItems = NULL;
	PyObject *pItem = NULL;
	PyObject *pKey = NULL;
	Tcl_Obj *tKey;

	/*
	 * The ordering must always be more 'specific' types first. E.g. a
	 * string also obeys the sequence protocol...but we probably want it
	 * to be a string rather than a list. Suggested order below:
	 * - None -> {}
	 * - True -> 1, False -> 0
	 * - bytes -> tcl byte string
	 * - unicode -> tcl unicode string
	 * - number protocol -> tcl number
	 * - sequence protocol -> tcl list
	 * - mapping protocol -> tcl dict
	 * - other -> error (currently converts to string)
	 *
	 * Note that the sequence and mapping protocol are both determined by __getitem__,
	 * the only difference is that dict subclasses are excluded from sequence.
	 */

	if (pObj == Py_None) {
		tObj = Tcl_NewObj();
	} else if (pObj == Py_True || pObj == Py_False) {
		tObj = Tcl_NewBooleanObj(pObj == Py_True);
	} else if (PyTclObj_Check(pObj)) {
		PyTclObj *pyTclObj = (PyTclObj *)pObj;
		tObj = pyTclObj->tclobj;
	} else if (PyBytes_Check(pObj)) {
		tObj = Tcl_NewByteArrayObj(
			(const unsigned char *)PyBytes_AS_STRING(pObj),
			PyBytes_GET_SIZE(pObj)
		);
	} else if (PyUnicode_Check(pObj)) {
		pBytesObj = PyUnicode_AsUTF8String(pObj);
		if (pBytesObj == NULL)
			return NULL;
		tObj = Tcl_NewStringObj(
			PyBytes_AS_STRING(pBytesObj), PyBytes_GET_SIZE(pBytesObj)
		);
		Py_DECREF(pBytesObj);
	} else if (PyNumber_Check(pObj)) {
		/* We go via string to support arbitrary length numbers */
		if (PyLong_Check(pObj)) {
			pStrObj = PyNumber_ToBase(pObj, 10);
		} else {
			assert(PyComplex_Check(pObj) || PyFloat_Check(pObj));
			pStrObj = PyObject_Str(pObj);
		}
		if (pStrObj == NULL)
			return NULL;
		pBytesObj = PyUnicode_AsUTF8String(pStrObj);
		Py_DECREF(pStrObj);
		if (pBytesObj == NULL)
			return NULL;
		tObj = Tcl_NewStringObj(
			PyBytes_AS_STRING(pBytesObj), PyBytes_GET_SIZE(pBytesObj)
		);
		Py_DECREF(pBytesObj);
	} else if (PySequence_Check(pObj)) {
		tObj = Tcl_NewListObj(0, NULL);
		len = PySequence_Length(pObj);
		if (len == -1)
			return NULL;

		for (i = 0; i < len; i++) {
			pVal = PySequence_GetItem(pObj, i);
			if (pVal == NULL)
				return NULL;
			tVal = pyObjToTcl(interp, pVal);
			Py_DECREF(pVal);
			if (tVal == NULL)
				return NULL;
			Tcl_ListObjAppendElement(interp, tObj, tVal);
		}
	} else if (PyMapping_Check(pObj)) {
		tObj = Tcl_NewDictObj();
		len = PyMapping_Length(pObj);
		if (len == -1)
			return NULL;
		pItems = PyMapping_Items(pObj);
		if (pItems == NULL)
			return NULL;
#define ONERR(VAR) if (VAR == NULL) { Py_DECREF(pItems); return NULL; }
		for (i = 0; i < len; i++) {
			pItem = PySequence_GetItem(pItems, i);
			ONERR(pItem)
			pKey = PySequence_GetItem(pItem, 0);
			ONERR(pKey)
			pVal = PySequence_GetItem(pItem, 1);
			ONERR(pVal)
			tKey = pyObjToTcl(interp, pKey);
			Py_DECREF(pKey);
			ONERR(tKey);
			tVal = pyObjToTcl(interp, pVal);
			Py_DECREF(pVal);
			ONERR(tVal);
			Tcl_DictObjPut(interp, tObj, tKey, tVal);
		}
#undef ONERR
		Py_DECREF(pItems);
		/* Broke out of loop because of error */
		if (i != len) {
			Py_XDECREF(pItem);
			return NULL;
		}
	} else {
		/* Get python string representation of other objects */
		pStrObj = PyObject_Str(pObj);
		if (pStrObj == NULL)
			return NULL;
		pBytesObj = PyUnicode_AsUTF8String(pStrObj);
		Py_DECREF(pStrObj);
		if (pBytesObj == NULL)
			return NULL;
		tObj = Tcl_NewStringObj(
			PyBytes_AS_STRING(pBytesObj), PyBytes_GET_SIZE(pBytesObj)
		);
		Py_DECREF(pBytesObj);
	}

	return tObj;
}

//
// PyReturnTclError - return a tcl error to the tcl interpreter
//   with the specified string as an error message
//
static int
PyReturnTclError(Tcl_Interp *interp, char *string) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(string, -1));
	return TCL_ERROR;
}

//
// PyReturnException - return a python exception to tcl as a tcl error
//
static int
PyReturnException(Tcl_Interp *interp, char *description)
{
	// Shouldn't call this function unless Python has excepted
	if (PyErr_Occurred() == NULL) {
		return PyReturnTclError(interp, "bug in tohil - PyReturnException called without a python error having occurred");
	}

	// break out the exception
	PyObject *pType = NULL, *pVal = NULL, *pTrace = NULL;

	PyErr_Fetch(&pType, &pVal, &pTrace); /* Clears exception */
	PyErr_NormalizeException(&pType, &pVal, &pTrace);

	// set tcl interpreter result
	Tcl_SetObjResult(interp, pyObjToTcl(interp, pVal));

	// invoke python tohil.handle_exception(type, val, tracebackObject)
	// it returns a tuple consisting of the error code and error info (traceback)
	PyObject *pExceptionResult = PyObject_CallFunctionObjArgs(pTohilHandleException, pType, pVal, pTrace, NULL);

	// call tohil python exception handler function
	// return to me a tuple containing the error string, error code, and traceback
	if (pExceptionResult == NULL) {
		// NB debug break out the exception
		PyObject *pType = NULL, *pVal = NULL, *pTrace = NULL;
		PyErr_Fetch(&pType, &pVal, &pTrace); /* Clears exception */
		PyErr_NormalizeException(&pType, &pVal, &pTrace);
		PyObject_Print(pType, stdout, 0);
		PyObject_Print(pVal, stdout, 0);
		return PyReturnTclError(interp, "some problem running the tohil python exception handler");
	}

	if (!PyTuple_Check(pExceptionResult) || PyTuple_GET_SIZE(pExceptionResult) != 2) {
		return PyReturnTclError(interp, "malfunction in tohil python exception handler, did not return tuple or tuple did not contain 2 elements");
	}

	Tcl_SetObjErrorCode(interp, pyObjToTcl(interp, PyTuple_GET_ITEM(pExceptionResult, 0)));
	Tcl_AppendObjToErrorInfo(interp, pyObjToTcl(interp, PyTuple_GET_ITEM(pExceptionResult, 1)));
	Py_DECREF(pExceptionResult);
	return TCL_ERROR;
}

//
// call python from tcl with very explicit arguments versus
//   slamming stuff through eval
//
//   NB we need one like this going the other direction
//
static int
TohilCall_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	if (objc < 2) {
	  wrongargs:
		Tcl_WrongNumArgs(interp, 1, objv, "?-kwlist list? func ?arg ...?");
		return TCL_ERROR;
	}

	PyObject *kwObj = NULL;
	const char *objandfn = Tcl_GetString(objv[1]);
	int objStart = 2;

	if (*objandfn == '-' && strcmp(objandfn, "-kwlist") == 0) {
		if (objc < 4) goto wrongargs;
		kwObj = tclListObjToPyDictObject(interp, objv[2]);
		objandfn = Tcl_GetString(objv[3]);
		objStart = 4;
		if (kwObj == NULL) {
			return TCL_ERROR;
		}
	}

	/* Borrowed ref, do not decrement */
	PyObject *pMainModule = PyImport_AddModule("__main__");
	if (pMainModule == NULL)
		return PyReturnException(interp, "unable to add module __main__ to python interpreter");

	/* So we don't have to special case the decref in the following loop */
	Py_INCREF(pMainModule);
	PyObject *pObjParent = NULL;
	PyObject *pObj = pMainModule;
	PyObject *pObjStr = NULL;
	char *dot = index(objandfn, '.');
	while (dot != NULL) {
		pObjParent = pObj;

		pObjStr = PyUnicode_FromStringAndSize(objandfn, dot-objandfn);
		if (pObjStr == NULL) {
			Py_DECREF(pObjParent);
			return PyReturnException(interp, "failed unicode translation of call function in python interpreter");
		}

		pObj = PyObject_GetAttr(pObjParent, pObjStr);
		Py_DECREF(pObjStr);
		Py_DECREF(pObjParent);
		if (pObj == NULL)
			return PyReturnException(interp, "failed to find dotted attribute in python interpreter");

		objandfn = dot + 1;
		dot = index(objandfn, '.');
	}

	PyObject *pFn = PyObject_GetAttrString(pObj, objandfn);
	Py_DECREF(pObj);
	if (pFn == NULL)
		return PyReturnException(interp, "failed to find object/function in python interpreter");

	if (!PyCallable_Check(pFn)) {
		Py_DECREF(pFn);
		return PyReturnException(interp, "function is not callable");
	}

	// if there are no positional arguments, we will
	// call PyTuple_New with a 0 argument, producing
	// a 0-length tuple.  whil PyObject_Call's kwargs
	// argument can be NULL, args must not; an empty
	// tuple should be used
	int i;
	PyObject *pArgs = PyTuple_New(objc - objStart);
	PyObject* curarg = NULL;
	for (i = objStart; i < objc; i++) {
		curarg = PyUnicode_FromString(Tcl_GetString(objv[i]));
		if (curarg == NULL) {
			Py_DECREF(pArgs);
			Py_DECREF(pFn);
			return PyReturnException(interp, "unicode string conversion failed");
		}
		/* Steals a reference */
		PyTuple_SET_ITEM(pArgs, i - objStart, curarg);
	}

	PyObject *pRet = PyObject_Call(pFn, pArgs, kwObj);
	Py_DECREF(pFn);
	Py_DECREF(pArgs);
	if (kwObj != NULL)
		Py_DECREF(kwObj);
	if (pRet == NULL)
		return PyReturnException(interp, "error in python object call");

	Tcl_Obj *tRet = pyObjToTcl(interp, pRet);
	Py_DECREF(pRet);
	if (tRet == NULL)
		return PyReturnException(interp, "error converting python object to tcl object");

	Tcl_SetObjResult(interp, tRet);
	return TCL_OK;
}

//
// implements tcl command tohil::import, to import a python module
//   into the python interpreter.
//
static int
TohilImport_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	const char *modname, *topmodname;
	PyObject *pMainModule, *pTopModule;
	int ret = -1;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "module");
		return TCL_ERROR;
	}

	modname = Tcl_GetString(objv[1]);

	/* Borrowed ref, do not decrement */
	pMainModule = PyImport_AddModule("__main__");
	if (pMainModule == NULL)
		return PyReturnException(interp, "add module __main__ failed");

	// We don't use PyImport_ImportModule so mod.submod works
	pTopModule = PyImport_ImportModuleEx(modname, NULL, NULL, NULL);
	if (pTopModule == NULL)
		return PyReturnException(interp, "import module failed");

	topmodname = PyModule_GetName(pTopModule);
	if (topmodname != NULL) {
		ret = PyObject_SetAttrString(pMainModule, topmodname, pTopModule);
	}
	Py_DECREF(pTopModule);

	if (ret < 0)
		return PyReturnException(interp, "while trying to import a module");

	return TCL_OK;
}

static int
TohilEval_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "evalString");
		return TCL_ERROR;
	}
	const char *cmd = Tcl_GetString(objv[1]);

    // PyCompilerFlags flags = _PyCompilerFlags_INIT;
	// PyObject *code = Py_CompileStringExFlags(cmd, "tohil", Py_eval_input, &flags, -1);
	PyObject *code = Py_CompileStringExFlags(cmd, "tohil", Py_eval_input, NULL, -1);

	if (code == NULL) {
		return PyReturnException(interp, "while compiling python eval code");
	}

	PyObject *main_module = PyImport_AddModule("__main__");
	PyObject *global_dict = PyModule_GetDict(main_module);
	PyObject *pyobj = PyEval_EvalCode(code, global_dict, global_dict);

	Py_XDECREF(code);

	if (pyobj == NULL) {
		return PyReturnException(interp, "while evaluating python code");
	}

	Tcl_SetObjResult(interp, pyObjToTcl(interp, pyobj));
	Py_XDECREF(pyobj);
	return TCL_OK;
}

// awfully similar to TohilEval_Cmd above
// but expecting to do more like capture stdout
static int
TohilExec_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "execString");
		return TCL_ERROR;
	}
	const char *cmd = Tcl_GetString(objv[1]);

	PyObject *code = Py_CompileStringExFlags(cmd, "tohil", Py_file_input, NULL, -1);

	if (code == NULL) {
		return PyReturnException(interp, "while compiling python exec code");
	}

	PyObject *main_module = PyImport_AddModule("__main__");
	PyObject *global_dict = PyModule_GetDict(main_module);
	PyObject *pyobj = PyEval_EvalCode(code, global_dict, global_dict);

	Py_XDECREF(code);

	if (pyobj == NULL) {
		return PyReturnException(interp, "while evaluating python code");
	}

	Tcl_SetObjResult(interp, pyObjToTcl(interp, pyobj));
	Py_XDECREF(pyobj);
	return TCL_OK;
}

//
// implements tcl-side tohil::interact command to launch the python
//   interpreter's interactive loop
//
static int
TohilInteract_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	if (objc != 1) {
		Tcl_WrongNumArgs(interp, 1, objv, "");
		return TCL_ERROR;
	}

	int result = PyRun_InteractiveLoop(stdin, "stdin");
	if (result < 0) {
		return PyReturnException(interp, "interactive loop failure");
	}

	return TCL_OK;
}

/* Python library begins here */

// python tcl object "tclobj"

//
// return true if python object is a tclobj type
//
int
PyTclObj_Check(PyObject *pyObj)
{
	return PyObject_TypeCheck(pyObj, &PyTclObjType);
}

//
// create a new python tclobj object from a tclobj
//
static PyObject *
PyTclObj_FromTclObj(Tcl_Obj *obj)
{
	PyTclObj *self = (PyTclObj *)PyTclObjType.tp_alloc(&PyTclObjType, 0);
	if (self != NULL) {
		self->tclobj = obj;
		Tcl_IncrRefCount(obj);
	}
	return (PyObject *) self;
}

//
// create a new python tclobj object
//
// currently just creates an empty object but could use args
// and keywords to do other interesting stuff?
//
static PyObject *
PyTclObj_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	PyObject *pSource = NULL;
	static char *kwlist[] = {"from", NULL};
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &pSource))
		return NULL;

	PyTclObj *self = (PyTclObj *)type->tp_alloc(type, 0);
	if (self != NULL) {
		if (pSource == NULL) {
			self->tclobj = Tcl_NewObj();
		} else {
			self->tclobj = pyObjToTcl(tcl_interp, pSource);
			Py_XDECREF(pSource);
		}
		Tcl_IncrRefCount(self->tclobj);
	}
	return (PyObject *) self;
}

static void
PyTclObj_dealloc(PyTclObj *self)
{
	Tcl_DecrRefCount(self->tclobj);
	Py_TYPE(self)->tp_free((PyObject *) self);
}


static int
PyTclObj_init(PyTclObj *self, PyObject *args, PyObject *kwds)
{
	return 0;
}

static PyObject *
PyTclObj_repr(PyTclObj *self)
{
	int tclStringSize;
	char *tclString = Tcl_GetStringFromObj(self->tclobj, &tclStringSize);
	return Py_BuildValue("s#", tclString, tclStringSize);
}

//
// tclobj.reset()
//
static PyObject *
PyTclObj_reset(PyTclObj *self, PyObject *pyobj)
{
	Tcl_DecrRefCount(self->tclobj);
	self->tclobj = Tcl_NewObj();
	Tcl_IncrRefCount(self->tclobj);
	Py_INCREF(Py_None);
	return Py_None;
}

//
// tclobj.as_int()
//
static PyObject *
PyTclObj_as_int(PyTclObj *self, PyObject *pyobj)
{
	long longValue;

	if (Tcl_GetLongFromObj(tcl_interp, self->tclobj, &longValue) == TCL_OK) {
		return PyLong_FromLong(longValue);
	}
	PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(tcl_interp)));
	return NULL;
}

//
// tclobj.as_float()
//
static PyObject *
PyTclObj_as_float(PyTclObj *self, PyObject *pyobj)
{
	double doubleValue;

	if (Tcl_GetDoubleFromObj(tcl_interp, self->tclobj, &doubleValue) == TCL_OK) {
		return PyFloat_FromDouble(doubleValue);
	}
	PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(tcl_interp)));
	return NULL;
}

//
// tclobj.as_bool()
//
static PyObject *
PyTclObj_as_bool(PyTclObj *self, PyObject *pyobj)
{
	int intValue;
	if (Tcl_GetBooleanFromObj(tcl_interp, self->tclobj, &intValue) == TCL_OK) {
		PyObject *p = (intValue ? Py_True : Py_False);
		Py_INCREF(p);
		return p;
	}
	PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(tcl_interp)));
	return NULL;
}

//
// tclobj.as_string()
//
static PyObject *
PyTclObj_as_string(PyTclObj *self, PyObject *pyobj)
{
	int tclStringSize;
	char *tclString = Tcl_GetStringFromObj(self->tclobj, &tclStringSize);
	return Py_BuildValue("s#", tclString, tclStringSize);
}

//
// tclobj.as_list()
//
static PyObject *
PyTclObj_as_list(PyTclObj *self, PyObject *pyobj)
{
	return tclListObjToPyListObject(tcl_interp, self->tclobj);
}

//
// tclobj.as_set()
//
static PyObject *
PyTclObj_as_set(PyTclObj *self, PyObject *pyobj)
{
	return tclListObjToPySetObject(tcl_interp, self->tclobj);
}

//
// tclobj.as_tuple()
//
static PyObject *
PyTclObj_as_tuple(PyTclObj *self, PyObject *pyobj)
{
	return tclListObjToPyTupleObject(tcl_interp, self->tclobj);
}

//
// tclobj.as_dict()
//
static PyObject *
PyTclObj_as_dict(PyTclObj *self, PyObject *pyobj)
{
	return tclListObjToPyDictObject(tcl_interp, self->tclobj);
}

//
// tclobj.as_tclobj()
//
static PyObject *
PyTclObj_as_tclobj(PyTclObj *self, PyObject *pyobj)
{
	return PyTclObj_FromTclObj(self->tclobj);
}

//
// llength - return the length of a python tclobj's tcl object
//   as a list.  exception thrown if tcl object isn't a list
//
static PyObject *
PyTclObj_llength(PyTclObj *self, PyObject *pyobj)
{
	int length;
	if (Tcl_ListObjLength(tcl_interp, self->tclobj, &length) == TCL_OK) {
		return PyLong_FromLong(length);
	}
	PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(tcl_interp)));
	return NULL;
}

//
// getvar - set python tclobj to contain the value of a tcl var
//
static PyObject *
PyTclObj_getvar(PyTclObj *self, PyObject *var)
{
	char *varString = (char *)PyUnicode_1BYTE_DATA(var);
	Tcl_Obj *newObj = Tcl_GetVar2Ex(tcl_interp, varString, NULL, (TCL_LEAVE_ERR_MSG));
	if (newObj == NULL) {
		PyErr_SetString(PyExc_NameError, Tcl_GetString(Tcl_GetObjResult(tcl_interp)));
		return NULL;
	}
	Tcl_DecrRefCount(self->tclobj);
	self->tclobj = newObj;
	Tcl_IncrRefCount(self->tclobj);
	Py_INCREF(Py_None);
	return Py_None;
}

//
// setvar - set set tcl var to point to the tcl object of a python tclobj
//
static PyObject *
PyTclObj_setvar(PyTclObj *self, PyObject *var)
{
	char *varString = (char *)PyUnicode_1BYTE_DATA(var);
	// setvar handles incrementing the reference count
	if (Tcl_SetVar2Ex(tcl_interp, varString, NULL, self->tclobj, (TCL_LEAVE_ERR_MSG)) == NULL) {
		PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(tcl_interp)));
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

// set - tclobj type set method can set an object to a lot
// of possible python stuff -- NB there must be a better way
static PyObject *
PyTclObj_set(PyTclObj *self, PyObject *pyObject)
{
	Tcl_Obj *newObj = pyObjToTcl(tcl_interp, pyObject);
	if (newObj == NULL) {
		return NULL;
	}
	Tcl_DecrRefCount(self->tclobj);
	self->tclobj = newObj;
	Tcl_IncrRefCount(self->tclobj);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
PyTclObj_lindex(PyTclObj *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"index", "to", NULL};
	PyObject *to = NULL;
	int index = 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|$O", kwlist, &index, &to))
		return NULL;

	Tcl_Obj *resultObj = NULL;
	if (Tcl_ListObjIndex(tcl_interp, self->tclobj, index, &resultObj) == TCL_ERROR) {
		PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(tcl_interp)));
		return NULL;
	}
	return tohil_python_return(tcl_interp, TCL_OK, to, resultObj);
}

static PyObject *
PyTclObj_refcount(PyTclObj *self, PyObject *dummy)
{
	return PyLong_FromLong(self->tclobj->refCount);
}

static PyObject *
PyTclObj_type(PyTclObj *self, PyObject *dummy)
{
	const Tcl_ObjType *typePtr = self->tclobj->typePtr;
	if (typePtr == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return Py_BuildValue("s", self->tclobj->typePtr->name);
}


static PyMethodDef PyTclObj_methods[] = {
	{"reset", (PyCFunction) PyTclObj_reset, METH_NOARGS, "reset the tclobj"},
	{"as_str", (PyCFunction) PyTclObj_as_string, METH_NOARGS, "return tclobj as str"},
	{"as_int", (PyCFunction) PyTclObj_as_int, METH_NOARGS, "return tclobj as int"},
	{"as_float", (PyCFunction) PyTclObj_as_float, METH_NOARGS, "return tclobj as float"},
	{"as_bool", (PyCFunction) PyTclObj_as_bool, METH_NOARGS, "return tclobj as bool"},
	{"as_list", (PyCFunction) PyTclObj_as_list, METH_NOARGS, "return tclobj as list"},
	{"as_set", (PyCFunction) PyTclObj_as_set, METH_NOARGS, "return tclobj as set"},
	{"as_tuple", (PyCFunction) PyTclObj_as_tuple, METH_NOARGS, "return tclobj as tuple"},
	{"as_dict", (PyCFunction) PyTclObj_as_dict, METH_NOARGS, "return tclobj as dict"},
	{"as_tclobj", (PyCFunction) PyTclObj_as_tclobj, METH_NOARGS, "return tclobj as tclobj"},
	{"llength", (PyCFunction) PyTclObj_llength, METH_NOARGS, "length of tclobj tcl list"},
	{"getvar", (PyCFunction) PyTclObj_getvar, METH_O, "set tclobj to tcl var or array element"},
	{"setvar", (PyCFunction) PyTclObj_setvar, METH_O, "set tcl var or array element to tclobj's tcl object"},
	{"set", (PyCFunction) PyTclObj_set, METH_O, "set tclobj from some python object"},
	{"lindex", (PyCFunction) PyTclObj_lindex, METH_VARARGS | METH_KEYWORDS, "get value from tclobj as tcl list"},
	{"refcount", (PyCFunction) PyTclObj_refcount, METH_NOARGS, "get tclobj's reference count"},
	{"type", (PyCFunction) PyTclObj_type, METH_NOARGS, "return the tclobj's type from tcl, or None if it doesn't have one"},
	{NULL} // sentinel
};

// possibly nascent iterator stuff
typedef struct {
	PyObject_HEAD
} PyTclObj_Iter;

PyObject *PyTclObj_iter(PyObject *self)
{
	Py_INCREF(self);
	return self;
}

PyObject *PyTclObj_next(PyObject *self)
{
	return NULL;
}

static PyTypeObject PyTclObjType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "tohil.tclobj",
	.tp_doc = "Tcl Object",
	.tp_basicsize = sizeof(PyTclObj),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_new = PyTclObj_new,
	.tp_init = (initproc) PyTclObj_init,
	.tp_dealloc = (destructor) PyTclObj_dealloc,
	.tp_methods = PyTclObj_methods,
	.tp_str = (reprfunc)PyTclObj_repr,
};
	// .tp_repr = (reprfunc)PyTclObj_repr,

// end of tcl obj python data type

// say return tohil_python_return(interp, tcl_result, to string, resultObject)
// from any python C function in this library that accepts a to=python_data_type argument,
// and this routine ought to handle it
PyObject *
tohil_python_return(Tcl_Interp *interp, int tcl_result, PyObject *toType, Tcl_Obj *resultObj) {
	const char *toString = NULL;
	PyTypeObject *pt = NULL;

	if (tcl_result == TCL_ERROR) {
		PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(resultObj));
		return NULL;
	}

	if (toType != NULL) {
		if (!PyType_Check(toType)) {
			PyErr_SetString(PyExc_RuntimeError, "to type is not a valid python data type");
			return NULL;
		}

		pt = (PyTypeObject *)toType;
		toString = pt->tp_name;
	}

	if (toType == NULL || strcmp(toString, "str") == 0) {
		int tclStringSize;
		char *tclString;

		Py_XDECREF(pt);
		tclString = Tcl_GetStringFromObj(resultObj, &tclStringSize);
		return Py_BuildValue("s#", tclString, tclStringSize);
	}

	if (strcmp(toString, "int") == 0) {
		long longValue;

		Py_XDECREF(pt);
		if (Tcl_GetLongFromObj(interp, resultObj, &longValue) == TCL_OK) {
			return PyLong_FromLong(longValue);
		}
		PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(interp)));
		return NULL;
	}

	if (strcmp(toString, "bool") == 0) {
		int boolValue;

		Py_XDECREF(pt);
		if (Tcl_GetBooleanFromObj(interp, resultObj, &boolValue) == TCL_OK) {
			PyObject *p = (boolValue ? Py_True : Py_False);
			Py_INCREF(p);
			return p;
		}
		PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(interp)));
		return NULL;
	}

	if (strcmp(toString, "float") == 0) {
		double doubleValue;

		Py_XDECREF(pt);
		if (Tcl_GetDoubleFromObj(interp, resultObj, &doubleValue) == TCL_OK) {
			return PyFloat_FromDouble(doubleValue);
		}
		PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(interp)));
		return NULL;
	}

	if (strcmp(toString, "tohil.tclobj") == 0) {
		Py_XDECREF(pt);
		return PyTclObj_FromTclObj(resultObj);
	}

	if (strcmp(toString, "list") == 0) {
		Py_XDECREF(pt);
		return tclListObjToPyListObject(interp, resultObj);
	}

	if (strcmp(toString, "set") == 0) {
		Py_XDECREF(pt);
		return tclListObjToPySetObject(interp, resultObj);
	}

	if (strcmp(toString, "dict") == 0) {
		Py_XDECREF(pt);
		return tclListObjToPyDictObject(interp, resultObj);
	}

	if (strcmp(toString, "tuple") == 0) {
		Py_XDECREF(pt);
		return tclListObjToPyTupleObject(interp, resultObj);
	}

	PyErr_SetString(PyExc_RuntimeError, "'to' conversion type must be str, int, bool, float, list, set, dict, tuple, or tohil.tclobj");
	return NULL;
}

//
// tohil.eval command for python to eval code in the tcl interpreter
//
static PyObject *
tohil_eval(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"tcl_code", "to", NULL};
	PyObject *to = NULL;
	char *tclCode = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|$O", kwlist, &tclCode, &to))
		return NULL;

	int result = Tcl_Eval(tcl_interp, tclCode);
	Tcl_Obj *resultObj = Tcl_GetObjResult(tcl_interp);

	return tohil_python_return(tcl_interp, result, to, resultObj);
}

//
// tohil.expr command for python to evaluate expressions using the tcl interpreter
//
static PyObject *
tohil_expr(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"expression", "to", NULL};
	char *expression = NULL;
	PyObject *to = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|$O", kwlist, &expression, &to))
		return NULL;

	Tcl_Obj *resultObj = NULL;
	if (Tcl_ExprObj(tcl_interp, Tcl_NewStringObj(expression, -1), &resultObj) == TCL_ERROR) {
		char *errMsg = Tcl_GetString(Tcl_GetObjResult(tcl_interp));
		PyErr_SetString(PyExc_RuntimeError, errMsg);
		return NULL;
	}

	return tohil_python_return(tcl_interp, TCL_OK, to, resultObj);
}

//
// tohil.getvar - from python get the contents of a variable
//
static PyObject *
tohil_getvar(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"var", "to", "default", NULL};
	char *var = NULL;
	PyObject *to = NULL;
	PyObject *defaultPyObj = NULL;
	Tcl_Obj *obj = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|$OO", kwlist, &var, &to, &defaultPyObj))
		return NULL;

	if (defaultPyObj == NULL) {
		// a default wasn't specified, it's an error if the var or array
		// element doesn't exist
		obj = Tcl_GetVar2Ex(tcl_interp, var, NULL, (TCL_LEAVE_ERR_MSG));

		if (obj == NULL) {
			PyErr_SetString(PyExc_NameError, Tcl_GetString(Tcl_GetObjResult(tcl_interp)));
			return NULL;
		}
	} else {
		// a default was specified, it's not an error if the var or array
		// element doesn't exist, we simply return the default value
		obj = Tcl_GetVar2Ex(tcl_interp, var, NULL, 0);
		if (obj == NULL) {
			Py_INCREF(defaultPyObj);
			return defaultPyObj;
		}
	}

	// the var or array element exists in tcl, return the value to python,
	// possibly to a specific datatype
	return tohil_python_return(tcl_interp, TCL_OK, to, obj);
}

//
// tohil.exists - from python see if a variable or array element exists in tcl
//
static PyObject *
tohil_exists(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"var", NULL};
	char *var = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|$", kwlist, &var))
		return NULL;

	Tcl_Obj *obj = Tcl_GetVar2Ex(tcl_interp, var, NULL, 0);

	PyObject *p = (obj == NULL ? Py_False : Py_True);
	Py_INCREF(p);
	return p;
}

//
// tohil.setvar - set a variable or array element in tcl from python
//
static PyObject *
tohil_setvar(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"var", "value", NULL};
	char *var = NULL;
	PyObject *pyValue = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO", kwlist, &var, &pyValue))
		return NULL;

	Tcl_Obj *tclValue = pyObjToTcl(tcl_interp, pyValue);

	Tcl_Obj *obj = Tcl_SetVar2Ex(tcl_interp, var, NULL, tclValue, (TCL_LEAVE_ERR_MSG));

	if (obj == NULL) {
		char *errMsg = Tcl_GetString(Tcl_GetObjResult(tcl_interp));
		PyErr_SetString(PyExc_RuntimeError, errMsg);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

// tohil.unset - from python unset a variable or array element in the tcl
//   interpreter.  it is not an error if the variable or element doesn't
//   exist.  if passed the name of an array with no subscripted element,
//   the entire array is deleted
static PyObject *
tohil_unset(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"var", NULL};
	char *var = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|$", kwlist, &var))
		return NULL;

	Tcl_UnsetVar(tcl_interp, var, 0);
	Py_INCREF(Py_None);
	return Py_None;
}

//
// tohil.subst - perform tcl "subst" substitution on the passed string,
// evaluating square-bracketed stuff and expanding $-prefaced variables,
// without evaluating the ultimate result, like eval would
//
static PyObject *
tohil_subst(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"string", NULL};
	char *string = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &string))
		return NULL;

	Tcl_Obj *obj = Tcl_SubstObj(tcl_interp, Tcl_NewStringObj(string, -1), TCL_SUBST_ALL);
	if (obj == NULL) {
		char *errMsg = Tcl_GetString(Tcl_GetObjResult(tcl_interp));
		PyErr_SetString(PyExc_RuntimeError, errMsg);
		return NULL;
	}

	int tclStringSize;
	char *tclString;
	tclString = Tcl_GetStringFromObj(obj, &tclStringSize);
	return Py_BuildValue("s#", tclString, tclStringSize);
}

//
// tohil.call function for python that's like the tohil::call in tcl, something
// that lets you explicitly specify a tcl command and its arguments and lets
// you avoid passing everything through eval.  here it is.
//
static PyObject *
tohil_call(PyObject *self, PyObject *args, PyObject *kwargs)
{
	Py_ssize_t objc = PyTuple_GET_SIZE(args);
	int i;
	PyObject *to = NULL;
	//
	// allocate an array of Tcl object pointers the same size
	// as the number of arguments we received
	Tcl_Obj **objv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * objc);

	//PyObject_Print(kwargs, stdout, 0);

	// we need to process kwargs to get the to
	if (kwargs != NULL) {
		to = PyDict_GetItemString(kwargs, "to");
	}

	// for each argument convert the python object to a tcl object
	// and store it in the tcl object vector
	for (i = 0; i < objc; i++) {
		objv[i] = pyObjToTcl(tcl_interp, PyTuple_GET_ITEM(args, i));
		Tcl_IncrRefCount(objv[i]);
	}

	// invoke tcl using the objv array we just constructed
	int tcl_result = Tcl_EvalObjv(tcl_interp, objc, objv, 0);

	for (i = 0; i < objc; i++) {
		Tcl_DecrRefCount(objv[i]);
	}
	ckfree(objv);

	return tohil_python_return(tcl_interp, tcl_result, to, Tcl_GetObjResult(tcl_interp));
}

//
// python C extension structure defining functions
//
static PyMethodDef TohilMethods[] = {
	{"eval",  (PyCFunction)tohil_eval,
		METH_VARARGS | METH_KEYWORDS,
		"Evaluate tcl code"},
	{"getvar",  (PyCFunction)tohil_getvar,
		METH_VARARGS | METH_KEYWORDS,
		"get vars and array elements from the tcl interpreter"},
	{"setvar",  (PyCFunction)tohil_setvar,
		METH_VARARGS | METH_KEYWORDS,
		"set vars and array elements in the tcl interpreter"},
	{"exists",  (PyCFunction)tohil_exists,
		METH_VARARGS | METH_KEYWORDS,
		"check whether vars and array elements exist in the tcl interpreter"},
	{"unset",  (PyCFunction)tohil_unset,
		METH_VARARGS | METH_KEYWORDS,
		"unset variables, array elements, or arrays from the tcl interpreter"},
	{"subst",  (PyCFunction)tohil_subst,
		METH_VARARGS | METH_KEYWORDS,
		"perform Tcl command, variable and backslash substitutions on a string"},
	{"expr",  (PyCFunction)tohil_expr,
		METH_VARARGS | METH_KEYWORDS,
		"evaluate Tcl expression"},
	{"call",  (PyCFunction)tohil_call,
		METH_VARARGS | METH_KEYWORDS,
		"invoke a tcl command with arguments"},
	{NULL, NULL, 0, NULL} /* Sentinel */
};

// TODO: there should probably be some tcl deinit in the clear/free code
static struct PyModuleDef TohilModule = {
	PyModuleDef_HEAD_INIT,
	"tohil",
	"A module to permit interop with Tcl",
	-1,
	TohilMethods,
	NULL, // m_slots
	NULL, // m_traverse
	NULL, // m_clear
	NULL, // m_free
};

/* Shared initialisation begins here */


// this is the entry point when tcl loads the tohil shared library
int
Tohil_Init(Tcl_Interp *interp)
{
	/* TODO: all TCL_ERRORs should set an error return */

	if (Tcl_InitStubs(interp, "8.6", 0) == NULL)
		return TCL_ERROR;

	if (Tcl_PkgRequire(interp, "Tcl", "8.6", 0) == NULL)
		return TCL_ERROR;

	if (Tcl_PkgProvide(interp, "tohil", PACKAGE_VERSION) != TCL_OK)
		return TCL_ERROR;

	if (Tcl_CreateNamespace(interp, "::tohil", NULL, NULL) == NULL)
		return TCL_ERROR;

	if (Tcl_CreateObjCommand(interp, "::tohil::eval",
		(Tcl_ObjCmdProc *) TohilEval_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL)
		== NULL)
		return TCL_ERROR;

	if (Tcl_CreateObjCommand(interp, "::tohil::exec",
		(Tcl_ObjCmdProc *) TohilExec_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL)
		== NULL)
		return TCL_ERROR;

	if (Tcl_CreateObjCommand(interp, "::tohil::call",
		(Tcl_ObjCmdProc *) TohilCall_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL)
		== NULL)
		return TCL_ERROR;

	if (Tcl_CreateObjCommand(interp, "::tohil::import",
		(Tcl_ObjCmdProc *) TohilImport_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL)
		== NULL)
		return TCL_ERROR;

	if (Tcl_CreateObjCommand(interp, "::tohil::interact",
		(Tcl_ObjCmdProc *) TohilInteract_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL)
		== NULL)
		return TCL_ERROR;

	// if i haven't been told python is up, tcl is the parent,
	// and we need to initialize the python interpreter and
	// our python module
	if (!Py_IsInitialized()) {
		Py_Initialize();
	}

	// stash the Tcl interpreter pointer so the python side can find it later
	PyObject *main_module = PyImport_AddModule("__main__");
	PyObject *pCap = PyCapsule_New(interp, "tohil.interp", NULL);
	if (PyObject_SetAttrString(main_module, "interp", pCap) == -1) {
		return TCL_ERROR;
	}
	Py_DECREF(pCap);

	// import tohil to get at the python parts
	// and grab a reference to tohil's exception handler
	PyObject *pTohilModStr, *pTohilMod;

	pTohilModStr = PyUnicode_FromString("tohil");
	pTohilMod = PyImport_Import(pTohilModStr);
	Py_DECREF(pTohilModStr);
	if (pTohilMod == NULL) {
		// NB debug break out the exception
		PyObject *pType = NULL, *pVal = NULL, *pTrace = NULL;
		PyErr_Fetch(&pType, &pVal, &pTrace); /* Clears exception */
		PyErr_NormalizeException(&pType, &pVal, &pTrace);
		PyObject_Print(pType, stdout, 0);
		PyObject_Print(pVal, stdout, 0);

		return PyReturnTclError(interp, "unable to import tohil module to python interpreter");
	}

	pTohilHandleException = PyObject_GetAttrString(pTohilMod, "handle_exception");
	Py_DECREF(pTohilMod);
	if (pTohilHandleException == NULL || !PyCallable_Check(pTohilHandleException)) {
		Py_XDECREF(pTohilHandleException);
		return PyReturnTclError(interp, "unable to find tohil.handle_exception function in python interpreter");
	}

	return TCL_OK;
}

//
// this is the entrypoint for when python loads us as a shared library
// note the double underscore, we are _tohil, not tohil, actually tohil._tohil.
// that helps us be able to load other tohil python stuff that's needed, like
// the handle_exception function, before we trigger a load of the shared library,
// by importing from it, see pysrc/tohil/__init__.py
//
PyMODINIT_FUNC
PyInit__tohil(void)
{
	Tcl_Interp *interp = NULL;

	// see if the tcl interpreter already exists by looking
	// for an attribute we stashed in __main__
	// NB i'm sure there's a better place to put this, but
	// it is opaque to python -- need expert help from
	// a python C API maven
	PyObject *main_module = PyImport_AddModule("__main__");
	PyObject *pCap = PyObject_GetAttrString(main_module, "interp");
	if (pCap == NULL) {
		// stashed attribute doesn't exist.
		// tcl interp hasn't been set up.
		// python is the parent.
		// create and initialize the tcl interpreter.
		PyErr_Clear();
		interp = Tcl_CreateInterp();

		if (Tcl_Init(interp) != TCL_OK) {
			return NULL;
		}

		// invoke Tohil_Init to load us into the tcl interpreter
		// NB uh this probably isn't enough and we need to do a
		// package require tohil as there is tcl code in files in
		// the package now
		// OTOH you know you've got the right shared library
		if (Tohil_Init(interp) == TCL_ERROR) {
			return NULL;
		}
	} else {
		// python interpreter-containing attribute exists, get the interpreter
		interp = PyCapsule_GetPointer(pCap, "tohil.interp");
		Py_DECREF(pCap);
	}
	tcl_interp = interp;

	// turn up the tclobj python type
	if (PyType_Ready(&PyTclObjType) < 0) {
		return NULL;
	}

	// create the python module
	PyObject *m = PyModule_Create(&TohilModule);
	if (m == NULL) {
		return NULL;
	}

	Py_INCREF(&PyTclObjType);
	if (PyModule_AddObject(m, "tclobj", (PyObject *) &PyTclObjType) < 0) {
		Py_DECREF(&PyTclObjType);
		Py_DECREF(m);
		return NULL;
	}

	// ..and stash a pointer to the tcl interpreter in a python
	// capsule so we can find it when we're doing python stuff
	// and need to talk to tcl
	pCap = PyCapsule_New(interp, "tohil.interp", NULL);
	if (PyObject_SetAttrString(m, "interp", pCap) == -1) {
		return NULL;
	}
	Py_DECREF(pCap);

	return m;
}
