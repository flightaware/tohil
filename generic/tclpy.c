
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <tcl.h>

#include <assert.h>
#include <dlfcn.h>

/* TCL LIBRARY BEGINS HERE */

// Need an integer we can use for detecting python errors, assume we'll never
// use TCL_BREAK
#define PY_ERROR TCL_BREAK

static PyObject *pFormatException = NULL;
static PyObject *pFormatExceptionOnly = NULL;

PyObject *
tclListObjToPyListObject(Tcl_Interp *interp, Tcl_Obj *inputObj) {
	Tcl_Obj **list;
	int count;

	if (Tcl_ListObjGetElements(interp, inputObj, &count, &list) == TCL_ERROR) {
		return NULL;
	}

	PyObject *plist = PyList_New(count);

	for (int i = 0; i < count; i++) {
		PyList_SET_ITEM(plist, i, Py_BuildValue("s", Tcl_GetString(list[i])));
	}

	return plist;
}

PyObject *
tclListObjToPyDictObject(Tcl_Interp *interp, Tcl_Obj *inputObj) {
	Tcl_Obj **list;
	int count;

	if (Tcl_ListObjGetElements(interp, inputObj, &count, &list) == TCL_ERROR) {
		return NULL;
	}

	if (count % 2 != 0) {
		// list doesn't have an even number of elements
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

// Returns a string that must be 'free'd containing an error and traceback, or
// NULL if there was no Python error
static char *
pyTraceAsStr(void)
{
	// Shouldn't call this function unless Python has excepted
	if (PyErr_Occurred() == NULL)
		return NULL;

	/* USE PYTHON TRACEBACK MODULE */

	// TODO: save the error and reraise in python if we have no idea
	// TODO: prefix everything with 'PY:'?
	// TODO: use extract_tb to get stack, print custom traceback myself

	PyObject *pType = NULL, *pVal = NULL, *pTrace = NULL;
	PyObject *pTraceList = NULL, *pTraceStr = NULL, *pTraceDesc = NULL, *pTraceBytes = NULL;
	PyObject *pNone = NULL, *pEmptyStr = NULL;
	char *traceStr = NULL;
	Py_ssize_t traceLen = 0;

	PyErr_Fetch(&pType, &pVal, &pTrace); /* Clears exception */
	PyErr_NormalizeException(&pType, &pVal, &pTrace);

	/* Get traceback as a python list */
	if (pTrace != NULL) {
		pTraceList = PyObject_CallFunctionObjArgs(
			pFormatException, pType, pVal, pTrace, NULL);
	} else {
		pTraceList = PyObject_CallFunctionObjArgs(
			pFormatExceptionOnly, pType, pVal, NULL);
	}
	if (pTraceList == NULL)
		return strdup("[Failed to get python exception details (#e_ltp01)]\n");

	/* Put the list in tcl order (top stack level at top) */
	pNone = PyObject_CallMethod(pTraceList, "reverse", NULL);
	if (pNone == NULL) {
		Py_DECREF(pTraceList);
		return strdup("[Failed to get python exception details (#e_ltp02)]\n");
	}
	assert(pNone == Py_None);
	Py_DECREF(pNone);

	/* Remove "Traceback (most recent call last):" if the trace len > 1 */
	/* TODO: this feels like a hack, there must be a better way */
	traceLen = PyObject_Length(pTraceList);
	if (traceLen > 1) {
		pTraceDesc = PyObject_CallMethod(pTraceList, "pop", NULL);
	}
	if (traceLen <= 0 || (pTraceDesc == NULL && traceLen > 1)) {
		Py_DECREF(pTraceList);
		return strdup("[Failed to get python exception details (#e_ltp03)]\n");
	}
	Py_XDECREF(pTraceDesc);

	/* Turn the python list into a python string */
	pEmptyStr = PyUnicode_FromString("");
	if (pEmptyStr == NULL) {
		Py_DECREF(pTraceList);
		return strdup("[Failed to get python exception details (#e_ltp04)]\n");
	}
	pTraceStr = PyObject_CallMethod(pEmptyStr, "join", "O", pTraceList);
	Py_DECREF(pTraceList);
	Py_DECREF(pEmptyStr);
	if (pTraceStr == NULL)
		return strdup("[Failed to get python exception details (#e_ltp05)]\n");

	/* Turn the python string into a string */
	pTraceBytes = PyUnicode_AsASCIIString(pTraceStr);
	Py_DECREF(pTraceStr);
	if (pTraceBytes == NULL)
		return strdup("[Failed to convert python exception details to ascii bytes (#e_ltp06)]\n");
	traceStr = strdup(PyBytes_AS_STRING(pTraceBytes));
	Py_DECREF(pTraceBytes);

	return traceStr;
}

static int
PyCall_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "func ?arg ...?");
		return TCL_ERROR;
	}

	const char *objandfn = Tcl_GetString(objv[2]);

	/* Borrowed ref, do not decrement */
	PyObject *pMainModule = PyImport_AddModule("__main__");
	if (pMainModule == NULL)
		return PY_ERROR;

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
			return PY_ERROR;
		}

		pObj = PyObject_GetAttr(pObjParent, pObjStr);
		Py_DECREF(pObjStr);
		Py_DECREF(pObjParent);
		if (pObj == NULL)
			return PY_ERROR;

		objandfn = dot + 1;
		dot = index(objandfn, '.');
	}

	PyObject *pFn = PyObject_GetAttrString(pObj, objandfn);
	Py_DECREF(pObj);
	if (pFn == NULL)
		return PY_ERROR;

	if (!PyCallable_Check(pFn)) {
		Py_DECREF(pFn);
		return PY_ERROR;
	}

	int i;
	PyObject *pArgs = PyTuple_New(objc-3);
	PyObject* curarg = NULL;
	for (i = 0; i < objc-3; i++) {
		curarg = PyUnicode_FromString(Tcl_GetString(objv[i+3]));
		if (curarg == NULL) {
			Py_DECREF(pArgs);
			Py_DECREF(pFn);
			return PY_ERROR;
		}
		/* Steals a reference */
		PyTuple_SET_ITEM(pArgs, i, curarg);
	}

	PyObject *pRet = PyObject_Call(pFn, pArgs, NULL);
	Py_DECREF(pFn);
	Py_DECREF(pArgs);
	if (pRet == NULL)
		return PY_ERROR;

	Tcl_Obj *tRet = pyObjToTcl(interp, pRet);
	Py_DECREF(pRet);
	if (tRet == NULL)
		return PY_ERROR;

	Tcl_SetObjResult(interp, tRet);
	return TCL_OK;
}

static int
PyEval_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "evalString");
		return TCL_ERROR;
	}

	const char *cmd = Tcl_GetString(objv[2]);

	if (PyRun_SimpleString(cmd) == 0) {
		return TCL_OK;
	} else {
		return PY_ERROR;
	};
}

static int
PyImport_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	const char *modname, *topmodname;
	PyObject *pMainModule, *pTopModule;
	int ret = -1;

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "module");
		return TCL_ERROR;
	}

	modname = Tcl_GetString(objv[2]);

	/* Borrowed ref, do not decrement */
	pMainModule = PyImport_AddModule("__main__");
	if (pMainModule == NULL)
		return PY_ERROR;

	// We don't use PyImport_ImportModule so mod.submod works
	pTopModule = PyImport_ImportModuleEx(modname, NULL, NULL, NULL);
	if (pTopModule == NULL)
		return PY_ERROR;

	topmodname = PyModule_GetName(pTopModule);
	if (topmodname != NULL) {
		ret = PyObject_SetAttrString(pMainModule, topmodname, pTopModule);
	}
	Py_DECREF(pTopModule);

	if (ret != -1) {
		return TCL_OK;
	} else {
		return PY_ERROR;
	}
}

/* The two static variables below are related by order, keep alphabetical */
static const char *cmdnames[] = {
	"call", "eval", "import", NULL
};
static int (*cmds[]) (
	ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]
) = {
	PyCall_Cmd, PyEval_Cmd, PyImport_Cmd
};

static int
Py_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?arg ...?");
		return TCL_ERROR;
	}

	int cmdindex;
	if (Tcl_GetIndexFromObj(interp, objv[1], cmdnames, "command", TCL_EXACT,
			&cmdindex) != TCL_OK)
		return TCL_ERROR;

	/* Actually call the command */
	int ret = (*(cmds[cmdindex]))(clientData, interp, objc, objv);

	if (ret == PY_ERROR) {
		ret = TCL_ERROR;
		// Not entirely sure if this is the correct way of doing things. Should
		// I be calling Tcl_AddErrorInfo instead?
		char *traceStr = pyTraceAsStr(); // clears exception
		if (traceStr == NULL) {
			// TODO: something went wrong in traceback
			PyErr_Clear();
			return TCL_ERROR;
		}
		Tcl_AppendResult(interp, traceStr, NULL);
		Tcl_AppendResult(interp, "----- tcl -> python interface -----", NULL);
		free(traceStr);
	}

	return ret;
}

static int
PyReturnException(Tcl_Interp *interp, char *description)
{
	char *traceStr = pyTraceAsStr(); // clears exception
	if (traceStr == NULL) {
		// TODO: something went wrong in traceback
		PyErr_Clear();
		return TCL_ERROR;
	}

	Tcl_SetObjResult(interp, Tcl_NewStringObj(traceStr, -1));
	Tcl_AddErrorInfo(interp, description);
	free(traceStr);
	return TCL_ERROR;
}

static int
PyNewEval_Cmd(
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

// awfully similar to PyNewEval_Cmd above
// but expecting to do more like capture stdout
static int
PyExec_Cmd(
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

/* PYTHON LIBRARY BEGINS HERE */

static PyObject *
tclpy_eval(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"tcl_code", NULL};
	char *tclCode = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &tclCode))
		return NULL;

	Tcl_Interp *interp = PyCapsule_Import("tclpy.interp", 0);

	int result = Tcl_Eval(interp, tclCode);
	Tcl_Obj *tResult = Tcl_GetObjResult(interp);

	int tclStringSize;
	char *tclString;
	tclString = Tcl_GetStringFromObj(tResult, &tclStringSize);

	if (result == TCL_ERROR) {
		PyErr_SetString(PyExc_RuntimeError, tclString);
		return NULL;
	}
	return Py_BuildValue("s#", tclString, tclStringSize);
}

static PyObject *
tclpy_megaval(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"tcl_code", "to", NULL};
	char *to = NULL;
	char *tclCode = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|s", kwlist, &tclCode, &to))
		return NULL;

	Tcl_Interp *interp = PyCapsule_Import("tclpy.interp", 0);

	int result = Tcl_Eval(interp, tclCode);
	Tcl_Obj *tResult = Tcl_GetObjResult(interp);

	if (result == TCL_ERROR) {
		PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(tResult));
		return NULL;
	}

	if (to == NULL || strcmp(to, "string") == 0) {
		int tclStringSize;
		char *tclString;
		tclString = Tcl_GetStringFromObj(tResult, &tclStringSize);
		return Py_BuildValue("s#", tclString, tclStringSize);
	}

	if (strcmp(to, "int") == 0) {
		long longValue;
		if (Tcl_GetLongFromObj(interp, tResult, &longValue) == TCL_OK) {
			return PyLong_FromLong(longValue);
		}
		PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(interp)));
		return NULL;
	}

	if (strcmp(to, "bool") == 0) {
		int boolValue;
		if (Tcl_GetBooleanFromObj(interp, tResult, &boolValue) == TCL_OK) {
			PyObject *p = (boolValue ? Py_True : Py_False);
			Py_INCREF(p);
			return p;
		}
		PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(interp)));
		return NULL;
	}

	if (strcmp(to, "float") == 0) {
		double doubleValue;

		if (Tcl_GetDoubleFromObj(interp, tResult, &doubleValue) == TCL_OK) {
			return PyFloat_FromDouble(doubleValue);
		}
		PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(interp)));
		return NULL;
	}


	if (strcmp(to, "list") == 0) {
		PyObject *p = tclListObjToPyListObject(interp, tResult);
		if (p == NULL) {
			PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(interp)));
			return NULL;
		}
		return p;
	}

	if (strcmp(to, "dict") == 0) {
		PyObject *p = tclListObjToPyDictObject(interp, tResult);
		if (p == NULL) {
			PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(interp)));
			return NULL;
		}
		return p;
	}

	PyErr_SetString(PyExc_RuntimeError, "'to' conversion must be one of 'string', 'int', 'bool', 'float', 'list', 'dict'");
	return NULL;
}

static PyObject *
tclpy_getvar(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"array", "var", NULL};
	char *array = NULL;
	char *var = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|s", kwlist, &array, &var))
		return NULL;

	Tcl_Interp *interp = PyCapsule_Import("tclpy.interp", 0);

	if (array == NULL && var != NULL) {
		array = var;
		var = NULL;
	}

	Tcl_Obj *obj = Tcl_GetVar2Ex(interp, array, var, 0);

	if (obj == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int tclStringSize;
	char *tclString;
	tclString = Tcl_GetStringFromObj(obj, &tclStringSize);
	return Py_BuildValue("s#", tclString, tclStringSize);
}

static PyObject *
tclpy_setvar(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"array", "value", "var", NULL};
	char *array = NULL;
	char *var = NULL;
	PyObject *pyValue = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|s", kwlist, &array, &pyValue, &var))
		return NULL;

	Tcl_Interp *interp = PyCapsule_Import("tclpy.interp", 0);

	Tcl_Obj *tclValue = pyObjToTcl(interp, pyValue);

	if (array == NULL && var != NULL) {
		array = var;
		var = NULL;
	}

	Tcl_Obj *obj = Tcl_SetVar2Ex(interp, array, var, tclValue, (TCL_LEAVE_ERR_MSG));

	if (obj == NULL) {
		char *errMsg = Tcl_GetString(Tcl_GetObjResult(interp));
		PyErr_SetString(PyExc_RuntimeError, errMsg);
		return NULL;
	}

	return Py_None;
}

static PyObject *
tclpy_subst(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"string", NULL};
	char *string = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &string))
		return NULL;

	Tcl_Interp *interp = PyCapsule_Import("tclpy.interp", 0);

	Tcl_Obj *obj = Tcl_SubstObj(interp, Tcl_NewStringObj(string, -1), TCL_SUBST_ALL);
	if (obj == NULL) {
		char *errMsg = Tcl_GetString(Tcl_GetObjResult(interp));
		PyErr_SetString(PyExc_RuntimeError, errMsg);
		return NULL;
	}

	int tclStringSize;
	char *tclString;
	tclString = Tcl_GetStringFromObj(obj, &tclStringSize);
	return Py_BuildValue("s#", tclString, tclStringSize);
}

static PyObject *
tclpy_expr(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"expression", NULL};
	char *expression = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &expression))
		return NULL;

	Tcl_Interp *interp = PyCapsule_Import("tclpy.interp", 0);

	Tcl_Obj *resultObj;
	if (Tcl_ExprObj(interp, Tcl_NewStringObj(expression, -1), &resultObj) == TCL_ERROR) {
		char *errMsg = Tcl_GetString(Tcl_GetObjResult(interp));
		PyErr_SetString(PyExc_RuntimeError, errMsg);
		return NULL;
	}

	return tclObjToPy(resultObj);
}


static PyMethodDef TclPyMethods[] = {
	{"eval",  (PyCFunction)tclpy_eval,
		METH_VARARGS | METH_KEYWORDS,
		"Evaluate some tcl code"},
	{"getvar",  (PyCFunction)tclpy_getvar,
		METH_VARARGS | METH_KEYWORDS,
		"dig vars and arrays out of the tcl interpreter"},
	{"setvar",  (PyCFunction)tclpy_setvar,
		METH_VARARGS | METH_KEYWORDS,
		"set vars and array elements in the tcl interpreter"},
	{"subst",  (PyCFunction)tclpy_subst,
		METH_VARARGS | METH_KEYWORDS,
		"perform Tcl command, variable and backslash substitutions on a string"},
	{"expr",  (PyCFunction)tclpy_expr,
		METH_VARARGS | METH_KEYWORDS,
		"evaluate Tcl expression"},
	{"megaval",  (PyCFunction)tclpy_megaval,
		METH_VARARGS | METH_KEYWORDS,
		"improved eval"},
	{NULL, NULL, 0, NULL} /* Sentinel */
};

// TODO: there should probably be some tcl deinit in the clear/free code
static struct PyModuleDef TclPyModule = {
	PyModuleDef_HEAD_INIT,
	"tclpy",
	"A module to permit interop with Tcl",
	-1,
	TclPyMethods,
	NULL, // m_slots
	NULL, // m_traverse
	NULL, // m_clear
	NULL, // m_free
};

/* SHARED INITIALISATION BEGINS HERE */

/* Keep track of the top level interpreter */
typedef enum {
	NO_PARENT,
	TCL_PARENT,
	PY_PARENT
} ParentInterp;
static ParentInterp parentInterp = NO_PARENT;

int Tclpy_Init(Tcl_Interp *interp);
PyObject *init_python_tclpy(Tcl_Interp* interp);

int
Tclpy_Init(Tcl_Interp *interp)
{
	/* TODO: all TCL_ERRORs should set an error return */

	if (parentInterp == TCL_PARENT)
		return TCL_ERROR;
	if (parentInterp == NO_PARENT)
		parentInterp = TCL_PARENT;

	if (Tcl_InitStubs(interp, "8.6", 0) == NULL)
		return TCL_ERROR;
	if (Tcl_PkgRequire(interp, "Tcl", "8.6", 0) == NULL)
		return TCL_ERROR;
	if (Tcl_PkgProvide(interp, "tclpy", PACKAGE_VERSION) != TCL_OK)
		return TCL_ERROR;

	if (Tcl_CreateNamespace(interp, "::tohil", NULL, NULL) == NULL)
		return TCL_ERROR;
	if (Tcl_CreateObjCommand(interp, "::tohil::eval",
		(Tcl_ObjCmdProc *) PyNewEval_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL)
		== NULL)
		return TCL_ERROR;

	if (Tcl_CreateObjCommand(interp, "::tohil::exec",
		(Tcl_ObjCmdProc *) PyExec_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL)
		== NULL)
		return TCL_ERROR;

	if (Tcl_CreateObjCommand(interp, "py",
		(Tcl_ObjCmdProc *) Py_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL)
		== NULL)
		return TCL_ERROR;

	if (parentInterp != PY_PARENT) {
		Py_Initialize(); /* void */
		if (init_python_tclpy(interp) == NULL)
			return TCL_ERROR;
	}

	/* Get support for full tracebacks */
	PyObject *pTraceModStr, *pTraceMod;

	pTraceModStr = PyUnicode_FromString("traceback");
	if (pTraceModStr == NULL)
		return TCL_ERROR;
	pTraceMod = PyImport_Import(pTraceModStr);
	Py_DECREF(pTraceModStr);
	if (pTraceMod == NULL)
		return TCL_ERROR;
	pFormatException =     PyObject_GetAttrString(pTraceMod, "format_exception");
	pFormatExceptionOnly = PyObject_GetAttrString(pTraceMod, "format_exception_only");
	Py_DECREF(pTraceMod);
	if (pFormatException == NULL || pFormatExceptionOnly == NULL ||
			!PyCallable_Check(pFormatException) ||
			!PyCallable_Check(pFormatExceptionOnly)) {
		Py_XDECREF(pFormatException);
		Py_XDECREF(pFormatExceptionOnly);
		return TCL_ERROR;
	}

	return TCL_OK;
}

PyObject *
init_python_tclpy(Tcl_Interp* interp)
{
	if (parentInterp == PY_PARENT)
		return NULL;
	if (parentInterp == NO_PARENT)
		parentInterp = PY_PARENT;
	if (parentInterp == TCL_PARENT)
		assert(interp != NULL);

	if (interp == NULL)
		interp = Tcl_CreateInterp();
	if (Tcl_Init(interp) != TCL_OK)
		return NULL;
	if (parentInterp == PY_PARENT && Tclpy_Init(interp) == TCL_ERROR)
		return NULL;

	PyObject *m = PyModule_Create(&TclPyModule);
	if (m == NULL)
		return NULL;
	PyObject *pCap = PyCapsule_New(interp, "tclpy.interp", NULL);
	if (PyObject_SetAttrString(m, "interp", pCap) == -1)
		return NULL;
	Py_DECREF(pCap);

	return m;
}

PyMODINIT_FUNC
PyInit_tclpy(void)
{
	return init_python_tclpy(NULL);
}
