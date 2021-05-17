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
#include <object.h>

#include <tcl.h>

#include <assert.h>
#include <dlfcn.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define STREQU(a, b) (*(a) == *(b) && strcmp((a), (b)) == 0)

// name we use for keeping track of python interpreter from tcl
// using tcl's Tcl_GetAssocData and friends
#define ASSOC_PYTERP_NAME "tohil_pyterp"
#define ASSOC_PYPARENT_NAME "tohil_pyparent"

// leave in asserts
#undef NDEBUG

// forward definitions

// tclobj python data type that consists of a standard python
// object header and then our sole addition, a pointer to
// a Tcl_Obj.  we dig into tclobj using the tcl C api in our
// methods and functions that implement the type.
typedef struct {
    PyObject_HEAD;
    PyTypeObject *to;
    Tcl_Interp *interp;
    Tcl_Obj *tclvar;
    Tcl_Obj *tclobj;
} TohilTclObj;

int TohilTclObj_Check(PyObject *pyObj);
int TohilTclDict_Check(PyObject *pyObj);
static PyTypeObject TohilTclObjType;
static PyTypeObject TohilTclObj_IterType;

int TohilTclDict_Check(PyObject *pyObj);
static PyTypeObject TohilTclDictType;
static PyObject *TohilTclDict_FromTclObj(Tcl_Interp *interp, Tcl_Obj *obj);

static Tcl_Obj *TohilTclObj_objptr(TohilTclObj *self);
static int TohilTclObj_stuff_var(TohilTclObj *self, Tcl_Obj *obj);

static int Tohil_ReturnExceptionToTcl(Tcl_Interp *interp, char *description);

static PyObject *tohil_python_return(Tcl_Interp *, int tcl_result, PyTypeObject *toType, Tcl_Obj *resultObj);

static int tohil_mod_exec(PyObject *m);

typedef struct {
    Tcl_Interp *interp;
} TohilModuleState;

#define tohilstate(o) ((TohilModuleState *)PyModule_GetState(o))

// TCL library begins here

//
// tohil_TclObjToUTF8DString - convert a Tcl object (string in WTF-8) to real UTF-8
// for Python. Use a DString for buffering.
//
static char *
tohil_TclObjToUTF8DString(Tcl_Interp *interp, Tcl_Obj *obj, Tcl_DString *ds)
{
    static Tcl_Encoding utf8encoding = NULL;
    if (!utf8encoding)
        utf8encoding = Tcl_GetEncoding(interp, "utf-8");
    int tclStringLen;
    char *tclString = Tcl_GetStringFromObj(obj, &tclStringLen);
    return Tcl_UtfToExternalDString(utf8encoding, tclString, tclStringLen, ds);
}

//
// tohil_UTF8ToTclDString - convert a Python utf-8 string to a Tcl "WTF-8" string.
// Use a DString for buffering.
//
static char *
tohil_UTF8ToTclDString(Tcl_Interp *interp, char *utf8String, int utf8StringLen, Tcl_DString *ds)
{
    static Tcl_Encoding utf8encoding = NULL;
    if (!utf8encoding)
        utf8encoding = Tcl_GetEncoding(interp, "utf-8");
    // Accepts -1 for string length but try to avoid it.
    if (utf8StringLen == -1) {
        utf8StringLen = strlen(utf8String);
    }
    return Tcl_ExternalToUtfDString(utf8encoding, utf8String, utf8StringLen, ds);
}

//
// turn a tcl list into a python list
//
static PyObject *
tclListObjToPyListObject(Tcl_Interp *interp, Tcl_Obj *inputObj)
{
    Tcl_Obj **list;
    int count;

    if (Tcl_ListObjGetElements(interp, inputObj, &count, &list) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
        return NULL;
    }

    PyObject *plist = PyList_New(count);

    for (int i = 0; i < count; i++) {
        Tcl_DString ds;
        PyList_SET_ITEM(plist, i, Py_BuildValue("s", tohil_TclObjToUTF8DString(interp, list[i], &ds)));
        Tcl_DStringFree(&ds);
    }

    return plist;
}

//
// turn a tcl list into a python set
//
static PyObject *
tclListObjToPySetObject(Tcl_Interp *interp, Tcl_Obj *inputObj)
{
    Tcl_Obj **list;
    int count;

    if (Tcl_ListObjGetElements(interp, inputObj, &count, &list) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
        return NULL;
    }

    PyObject *pset = PySet_New(NULL);

    for (int i = 0; i < count; i++) {
        Tcl_DString ds;
        if (PySet_Add(pset, Py_BuildValue("s", tohil_TclObjToUTF8DString(interp, list[i], &ds))) < 0) {
            // no need to set a python error; PySet_Add will do it
            return NULL;
        }
        Tcl_DStringFree(&ds);
    }

    return pset;
}

//
// turn a tcl list into a python tuple
//
static PyObject *
tclListObjToPyTupleObject(Tcl_Interp *interp, Tcl_Obj *inputObj)
{
    Tcl_Obj **list;
    int count;

    if (Tcl_ListObjGetElements(interp, inputObj, &count, &list) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
        return NULL;
    }

    PyObject *ptuple = PyTuple_New(count);

    for (int i = 0; i < count; i++) {
        Tcl_DString ds;
        PyTuple_SET_ITEM(ptuple, i, Py_BuildValue("s", tohil_TclObjToUTF8DString(interp, list[i], &ds)));
        Tcl_DStringFree(&ds);
    }

    return ptuple;
}

//
// turn a tcl list of key-value pairs into a python dict
//
static PyObject *
tclListObjToPyDictObject(Tcl_Interp *interp, Tcl_Obj *inputObj)
{
    Tcl_Obj **list;
    int count;

    if (Tcl_ListObjGetElements(interp, inputObj, &count, &list) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
        return NULL;
    }

    if (count % 2 != 0) {
        // list doesn't have an even number of elements
        PyErr_SetString(PyExc_TypeError, "list doesn't have an even number of elements");
        return NULL;
    }

    PyObject *pdict = PyDict_New();

    for (int i = 0; i < count; i += 2) {
        Tcl_DString kds, vds;
        char *key = tohil_TclObjToUTF8DString(interp, list[i], &kds);
        char *val = tohil_TclObjToUTF8DString(interp, list[i + 1], &vds);
        PyDict_SetItem(pdict, Py_BuildValue("s", key), Py_BuildValue("s", val));
        Tcl_DStringFree(&kds);
        Tcl_DStringFree(&vds);
    }

    return pdict;
}

//
// Convert a Python UTF8 string to a Tcl "WTF-8" string.
//
static int
tohil_UTF8ToTcl(Tcl_Interp *interp, char *src, int srclen, char **res, int *reslen)
{
    static Tcl_Encoding utf8encoding = NULL;
    int buflen = srclen;
    char *buf = ckalloc(buflen + 4);
    int written;
    int result;
    if (!utf8encoding)
        utf8encoding = Tcl_GetEncoding(interp, "utf-8");

    while (1) {
        result = Tcl_ExternalToUtf(interp, utf8encoding, src, srclen, 0, NULL, buf, buflen + 4, NULL, &written, NULL);
        if (result == TCL_OK) {
            *res = buf;
            *reslen = written;
            return TCL_OK;
        }
        if (result == TCL_CONVERT_NOSPACE) {
            ckfree(buf);
            buflen *= 2;
            buf = ckalloc(buflen + 4);
            continue;
        }
        ckfree(buf);
        return result;
    }
}

//
// Convert a Tcl "WTF-8" string to a Python UTF8 string
//
static int
tohil_TclToUTF8(Tcl_Interp *interp, char *src, int srclen, char **res, int *reslen)
{
    static Tcl_Encoding utf8encoding = NULL;
    int buflen = srclen;
    char *buf = ckalloc(buflen + 4);
    int written;
    if (!utf8encoding)
        utf8encoding = Tcl_GetEncoding(interp, "utf-8");
    int result;

    while (1) {
        result = Tcl_UtfToExternal(interp, utf8encoding, src, srclen, 0, NULL, buf, buflen + 4, NULL, &written, NULL);
        if (result == TCL_OK) {
            *res = buf;
            *reslen = written;
            return TCL_OK;
        }
        if (result == TCL_CONVERT_NOSPACE) {
            ckfree(buf);
            buflen *= 2;
            buf = ckalloc(buflen + 4);
            continue;
        }
        ckfree(buf);
        return result;
    }
}

//
// turn a tcl object into a python object by trying to convert it as a boolean,
// then a long, then a double and finally a string
//
// NB not currently used, and could be better by peeking first at the tcl
// object's internal rep.  i have concerns about using this because the python
// dev may be surprised to get an int or float back instead of a string.
// not sure but not ready to get rid of this; it still seems promising
//
static PyObject *
tclObjToPy(Tcl_Interp *interp, Tcl_Obj *tObj)
{
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

    int utf8len;
    char *utf8string;
    if (tohil_TclToUTF8(interp, tclString, tclStringSize, &utf8string, &utf8len) != TCL_OK) {
        PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(interp)));
        return NULL;
    }
    PyObject *pObj = Py_BuildValue("s#", utf8string, utf8len);
    ckfree(utf8string);
    return pObj;
}

//
// convert a python object to a tcl object - amazing code by aidan
//
static Tcl_Obj *
_pyObjToTcl(Tcl_Interp *interp, PyObject *pObj)
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

    char *utf8string;
    int utf8len;

    /*
     * The ordering must always be more 'specific' types first. E.g. a
     * string also obeys the sequence protocol...but we probably want it
     * to be a string rather than a list. Suggested order below:
     * - None -> {}
     * - True -> 1, False -> 0
     * - tclobj -> tclobj
     * - tcldict -> tcldict
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
    } else if (TohilTclObj_Check(pObj) || TohilTclDict_Check(pObj)) {
        TohilTclObj *pyTclObj = (TohilTclObj *)pObj;
        tObj = TohilTclObj_objptr(pyTclObj);
    } else if (PyBytes_Check(pObj)) {
        tObj = Tcl_NewByteArrayObj((const unsigned char *)PyBytes_AS_STRING(pObj), PyBytes_GET_SIZE(pObj));
    } else if (PyUnicode_Check(pObj)) {
        pBytesObj = PyUnicode_AsUTF8String(pObj);
        if (pBytesObj == NULL)
            return NULL;
        if (tohil_UTF8ToTcl(interp, PyBytes_AS_STRING(pBytesObj), PyBytes_GET_SIZE(pBytesObj), &utf8string, &utf8len) != TCL_OK) {
            Py_DECREF(pBytesObj);
            return NULL;
        }
        tObj = Tcl_NewStringObj(utf8string, utf8len);
        ckfree(utf8string);
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
        if (tohil_UTF8ToTcl(interp, PyBytes_AS_STRING(pBytesObj), PyBytes_GET_SIZE(pBytesObj), &utf8string, &utf8len) != TCL_OK) {
            Py_DECREF(pBytesObj);
            return NULL;
        }
        tObj = Tcl_NewStringObj(utf8string, utf8len);
        ckfree(utf8string);
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
            tVal = _pyObjToTcl(interp, pVal);
            Py_DECREF(pVal);
            if (tVal == NULL)
                return NULL;
            Tcl_ListObjAppendElement(interp, tObj, tVal);
        }
    } else if (PySet_Check(pObj)) {
        // NB heavily cribbed from the above
        // might be useful for other stuff besides sets, if it works
        PyObject *it = PyObject_GetIter(pObj);
        if (it == NULL)
            return NULL;
        tObj = Tcl_NewListObj(0, NULL);
        while ((pVal = PyIter_Next(it)) != NULL) {
            if (pVal == NULL)
                return NULL;
            tVal = _pyObjToTcl(interp, pVal);
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
#define ONERR(VAR)         \
    if (VAR == NULL) {     \
        Py_DECREF(pItems); \
        return NULL;       \
    }
        for (i = 0; i < len; i++) {
            pItem = PySequence_GetItem(pItems, i);
            ONERR(pItem)
            pKey = PySequence_GetItem(pItem, 0);
            ONERR(pKey)
            pVal = PySequence_GetItem(pItem, 1);
            ONERR(pVal)
            tKey = _pyObjToTcl(interp, pKey);
            Py_DECREF(pKey);
            ONERR(tKey);
            tVal = _pyObjToTcl(interp, pVal);
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
        if (tohil_UTF8ToTcl(interp, PyBytes_AS_STRING(pBytesObj), PyBytes_GET_SIZE(pBytesObj), &utf8string, &utf8len) != TCL_OK) {
            Py_DECREF(pBytesObj);
            return NULL;
        }
        tObj = Tcl_NewStringObj(utf8string, utf8len);
        ckfree(utf8string);
        Py_DECREF(pBytesObj);
    }

    return tObj;
}

static Tcl_Obj *
pyObjToTcl(Tcl_Interp *interp, PyObject *pObj)
{
    Tcl_Obj *ret = _pyObjToTcl(interp, pObj);
    // NB tcl is not prepared to accept null pointers to set interpreter results
    // and probably even variables and stuff yet pyObjToTcl can return them.
    // here we look to detect if it does.  later we might choose to cause an
    // error return or some kind, or, if necessary, do checks
    // everywhere in the code that currently assumes pyObjToTcl can't fail
    assert(ret != NULL);
    if (ret == NULL)
        abort();
    return ret;
}

// tohil_tcl_return - you call this routine when you are returning to
//   tcl after having done some work.
//
//   it checks to make sure
//
static int
tohil_tcl_return(Tcl_Interp *interp, int tcl_result)
{
    // check for an unhandled python exception -- that should not happen
    if (PyErr_Occurred() != NULL) {
        return Tohil_ReturnExceptionToTcl(interp, "Tohil C code returned to Tcl with an unprocessed Python exception");
    }

    return tcl_result;
}

//
// Tohil_ReturnTclError - return a tcl error to the tcl interpreter
//   with the specified string as an error message
//
static int
Tohil_ReturnTclError(Tcl_Interp *interp, char *string)
{
    Tcl_SetObjResult(interp, Tcl_NewStringObj(string, -1));
    return tohil_tcl_return(interp, TCL_ERROR);
}

//
// Tohil_ReturnExceptionToTcl - return a python exception to tcl as a tcl error
//
static int
Tohil_ReturnExceptionToTcl(Tcl_Interp *interp, char *description)
{
    // Shouldn't call this function unless Python has excepted
    if (PyErr_Occurred() == NULL) {
        return Tohil_ReturnTclError(interp, "bug in tohil - Tohil_ReturnExceptionToTcl called without a python error having occurred");
    }

    // break out the exception
    PyObject *pType = NULL, *pVal = NULL, *pTrace = NULL;

    PyErr_Fetch(&pType, &pVal, &pTrace); /* Clears exception */
    PyErr_NormalizeException(&pType, &pVal, &pTrace);
    // PyObject_Print(pType, stdout, 0);
    // PyObject_Print(pVal, stdout, 0);

    // set tcl interpreter result
    Tcl_SetObjResult(interp, pyObjToTcl(interp, pVal));

    Tcl_AddErrorInfo(interp, " (");
    Tcl_AddErrorInfo(interp, description);
    Tcl_AddErrorInfo(interp, ")");

    // find tohil.handle_exception python function
    PyObject *m = PyImport_AddModule("tohil");
    PyObject *handle_exception = PyObject_GetAttrString(m, "handle_exception");
    if (handle_exception == NULL || !PyCallable_Check(handle_exception)) {
        Py_XDECREF(handle_exception);
        return Tohil_ReturnTclError(interp, "unable to find tohil.handle_exception function in python interpreter");
    }

    // invoke python tohil.handle_exception(type, val, tracebackObject)
    // it returns a tuple consisting of the error code and error info (traceback)
    PyObject *pExceptionResult = PyObject_CallFunctionObjArgs(handle_exception, pType, pVal, pTrace, NULL);

    // call tohil python exception handler function
    // return to me a tuple containing the error string, error code, and traceback
    if (pExceptionResult == NULL) {
        // NB debug break out the exception
        PyObject *pType = NULL, *pVal = NULL, *pTrace = NULL;
        PyErr_Fetch(&pType, &pVal, &pTrace); /* Clears exception */
        PyErr_NormalizeException(&pType, &pVal, &pTrace);
        // PyObject_Print(pType, stdout, 0);
        // PyObject_Print(pVal, stdout, 0);
        return Tohil_ReturnTclError(interp, "some problem running the tohil python exception handler");
    }

    if (!PyTuple_Check(pExceptionResult) || PyTuple_GET_SIZE(pExceptionResult) != 2) {
        return Tohil_ReturnTclError(interp,
                                    "malfunction in tohil python exception handler, did not return tuple or tuple did not contain 2 elements");
    }

    Tcl_SetObjErrorCode(interp, pyObjToTcl(interp, PyTuple_GET_ITEM(pExceptionResult, 0)));
    Tcl_AppendObjToErrorInfo(interp, pyObjToTcl(interp, PyTuple_GET_ITEM(pExceptionResult, 1)));
    Py_DECREF(pExceptionResult);
    return TCL_ERROR;
}

//
// subinterpreter support
//

enum SubinterpType { TclParent, PythonParent, TclChild };

//
// this is called when a tcl interpreter is deleted
//
static void
tohil_delete_subinterp(ClientData clientData, Tcl_Interp *interp)
{
    PyThreadState *subinterp = (PyThreadState *)clientData;
    // unless this python interpreter started the whole thing, end the python
    // interpreter associated with this interpreter, because this interpreter
    // is going bye-bye.
    //
    // we don't want to delete the python interpreter that started everything
    // with import tohil, because we belong to it, not the other way around.
    if (Tcl_GetAssocData(interp, ASSOC_PYPARENT_NAME, NULL) == NULL) {
        // printf("tcl interpreter %p being deleted, deleting subinterp %p\n", interp, subinterp);
        PyThreadState_Swap(subinterp);
        Py_EndInterpreter(subinterp);
    }
}

// this is called to associate a subinterpreter with a tcl interpreter
static void
tohil_set_subinterp(Tcl_Interp *interp, PyThreadState *pyterp)
{
    Tcl_SetAssocData(interp, ASSOC_PYTERP_NAME, tohil_delete_subinterp, (ClientData)pyterp);
    // printf("tcl interpreter %p set assoc pyterp thread state %p\n", interp, pyterp);
}

// this is called to see if there's a subinterpreter associated with a tcl interpreter
static int
tohil_subinterp_is_set(Tcl_Interp *interp)
{
    return (PyThreadState *)Tcl_GetAssocData(interp, ASSOC_PYTERP_NAME, NULL) != NULL;
}

//
// swap to the python subinterpreter associated with this tcl interpreter
//
static void
tohil_swap_subinterp(Tcl_Interp *interp)
{
    PyThreadState *pyterp = (PyThreadState *)Tcl_GetAssocData(interp, ASSOC_PYTERP_NAME, NULL);
    assert(pyterp != NULL);
    if (pyterp != PyThreadState_Get()) {
        // printf("tohil_swap_subinterp %p, swapping subinterpreter to %p\n", interp, pyterp);
        PyThreadState_Swap(pyterp);
    }
}

//
// tohil_setup_subinterp - setup the python subinterpreter and hook
//  it up to tohil.
//
//  if invoked with PythonParent, it means python is the parent,
//  we set this interpreter's "subinterpreter" to python's main
//  interpreter and set a second assoc data item to indicate
//  that it's the parent, so we'll know not to delete it if our
//  interpreter gets deleted.
//
static void
tohil_setup_subinterp(Tcl_Interp *interp, enum SubinterpType subtype)
{
    switch (subtype) {
        case PythonParent:
            tohil_set_subinterp(interp, PyThreadState_Get());
            Tcl_SetAssocData(interp, ASSOC_PYPARENT_NAME, NULL, (ClientData)1);
            break;

        case TclParent:
            tohil_set_subinterp(interp, PyThreadState_Get());
            break;

        case TclChild:
            tohil_set_subinterp(interp, Py_NewInterpreter());
            break;
    }
}

//
// call python from tcl with very explicit arguments versus
//   slamming stuff through eval
//
static int
TohilCall_Cmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    tohil_swap_subinterp(interp);

    if (objc < 2) {
    wrongargs:
        Tcl_WrongNumArgs(interp, 1, objv, "?-kwlist list? func ?arg ...?");
        return tohil_tcl_return(interp, TCL_ERROR);
    }

    PyObject *kwObj = NULL;
    Tcl_DString ds;
    const char *objandfn = tohil_TclObjToUTF8DString(interp, objv[1], &ds);
    int objStart = 2;

    if (*objandfn == '-' && STREQU(objandfn, "-kwlist")) {
        if (objc < 4)
            goto wrongargs;
        kwObj = tclListObjToPyDictObject(interp, objv[2]);
        Tcl_DStringFree(&ds);
        objandfn = tohil_TclObjToUTF8DString(interp, objv[3], &ds);
        objStart = 4;
        if (kwObj == NULL) {
            return tohil_tcl_return(interp, TCL_ERROR);
        }
    }

    /* Borrowed ref, do not decrement */
    PyObject *pMainModule = PyImport_AddModule("__main__");
    if (pMainModule == NULL) {
        Tcl_DStringFree(&ds);
        return Tohil_ReturnExceptionToTcl(interp, "unable to add module __main__ to python interpreter");
    }

    /* So we don't have to special case the decref in the following loop */
    Py_INCREF(pMainModule);
    PyObject *pObjParent = NULL;
    PyObject *pObj = pMainModule;
    PyObject *pObjStr = NULL;
    char *dot = index(objandfn, '.');
    while (dot != NULL) {
        pObjParent = pObj;

        pObjStr = PyUnicode_FromStringAndSize(objandfn, dot - objandfn);
        if (pObjStr == NULL) {
            Py_DECREF(pObjParent);
            Tcl_DStringFree(&ds);
            return Tohil_ReturnExceptionToTcl(interp, "failed unicode translation of call function in python interpreter");
        }

        pObj = PyObject_GetAttr(pObjParent, pObjStr);
        Py_DECREF(pObjStr);
        Py_DECREF(pObjParent);
        if (pObj == NULL) {
            Tcl_DStringFree(&ds);
            return Tohil_ReturnExceptionToTcl(interp, "failed to find dotted attribute in python interpreter");
        }

        objandfn = dot + 1;
        dot = index(objandfn, '.');
    }

    PyObject *pFn = PyObject_GetAttrString(pObj, objandfn);
    Py_DECREF(pObj);
    Tcl_DStringFree(&ds);
    if (pFn == NULL)
        return Tohil_ReturnExceptionToTcl(interp, "failed to find object/function in python interpreter");

    if (!PyCallable_Check(pFn)) {
        Py_DECREF(pFn);
        return Tohil_ReturnExceptionToTcl(interp, "function is not callable");
    }

    // if there are no positional arguments, we will
    // call PyTuple_New with a 0 argument, producing
    // a 0-length tuple.  whil PyObject_Call's kwargs
    // argument can be NULL, args must not; an empty
    // tuple should be used
    int i;
    PyObject *pArgs = PyTuple_New(objc - objStart);
    PyObject *curarg = NULL;
    for (i = objStart; i < objc; i++) {
        curarg = PyUnicode_FromString(tohil_TclObjToUTF8DString(interp, objv[i], &ds));
        Tcl_DStringFree(&ds);
        if (curarg == NULL) {
            Py_DECREF(pArgs);
            Py_DECREF(pFn);
            return Tohil_ReturnExceptionToTcl(interp, "unicode string conversion failed");
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
        return Tohil_ReturnExceptionToTcl(interp, "error in python object call");

    Tcl_Obj *tRet = pyObjToTcl(interp, pRet);
    Py_DECREF(pRet);
    if (tRet == NULL)
        return Tohil_ReturnExceptionToTcl(interp, "error converting python object to tcl object");

    Tcl_SetObjResult(interp, tRet);
    return tohil_tcl_return(interp, TCL_OK);
}

//
// implements tcl command tohil::import, to import a python module
//   into the python interpreter.
//
static int
TohilImport_Cmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    const char *modname, *topmodname;
    PyObject *pMainModule, *pTopModule;
    int ret = -1;
    tohil_swap_subinterp(interp);

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "module");
        return tohil_tcl_return(interp, TCL_ERROR);
    }

    modname = Tcl_GetString(objv[1]);

    /* Borrowed ref, do not decrement */
    pMainModule = PyImport_AddModule("__main__");
    if (pMainModule == NULL)
        return Tohil_ReturnExceptionToTcl(interp, "add module __main__ failed");

    // We don't use PyImport_ImportModule so mod.submod works
    pTopModule = PyImport_ImportModuleEx(modname, NULL, NULL, NULL);
    if (pTopModule == NULL)
        return Tohil_ReturnExceptionToTcl(interp, "import module failed");

    // attach tohil module to __main__
    topmodname = PyModule_GetName(pTopModule);
    if (topmodname != NULL) {
        ret = PyObject_SetAttrString(pMainModule, topmodname, pTopModule);
    }
    Py_DECREF(pTopModule);

    if (ret < 0)
        return Tohil_ReturnExceptionToTcl(interp, "while trying to import a module");

    return tohil_tcl_return(interp, TCL_OK);
}

static int
TohilEval_Cmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    tohil_swap_subinterp(interp);

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "evalString");
        return tohil_tcl_return(interp, TCL_ERROR);
    }
    Tcl_DString ds;
    const char *cmd = tohil_TclObjToUTF8DString(interp, objv[1], &ds);

    // PyCompilerFlags flags = _PyCompilerFlags_INIT;
    // PyObject *code = Py_CompileStringExFlags(cmd, "tohil", Py_eval_input, &flags, -1);
    PyObject *code = Py_CompileStringExFlags(cmd, "tohil", Py_eval_input, NULL, -1);
    Tcl_DStringFree(&ds);

    if (code == NULL) {
        return Tohil_ReturnExceptionToTcl(interp, "while compiling python eval code");
    }

    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *global_dict = PyModule_GetDict(main_module);
    PyObject *pyobj = PyEval_EvalCode(code, global_dict, global_dict);

    Py_XDECREF(code);

    if (pyobj == NULL) {
        return Tohil_ReturnExceptionToTcl(interp, "while evaluating python code");
    }

    Tcl_SetObjResult(interp, pyObjToTcl(interp, pyobj));
    Py_XDECREF(pyobj);
    return tohil_tcl_return(interp, TCL_OK);
}

// awfully similar to TohilEval_Cmd above
// but expecting to do more like capture stdout
static int
TohilExec_Cmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    tohil_swap_subinterp(interp);

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "execString");
        return tohil_tcl_return(interp, TCL_ERROR);
    }
    Tcl_DString ds;
    const char *cmd = tohil_TclObjToUTF8DString(interp, objv[1], &ds);

    PyObject *code = Py_CompileStringExFlags(cmd, "tohil", Py_file_input, NULL, -1);
    Tcl_DStringFree(&ds);

    if (code == NULL) {
        return Tohil_ReturnExceptionToTcl(interp, "while compiling python exec code");
    }

    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *global_dict = PyModule_GetDict(main_module);
    PyObject *pyobj = PyEval_EvalCode(code, global_dict, global_dict);

    Py_XDECREF(code);

    if (pyobj == NULL) {
        return Tohil_ReturnExceptionToTcl(interp, "while evaluating python code");
    }

    Tcl_SetObjResult(interp, pyObjToTcl(interp, pyobj));
    Py_XDECREF(pyobj);
    return tohil_tcl_return(interp, TCL_OK);
}

//
// implements tcl-side tohil::interact command to launch the python
//   interpreter's interactive loop
//
static int
TohilInteract_Cmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    tohil_swap_subinterp(interp);

    if (objc != 1) {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return tohil_tcl_return(interp, TCL_ERROR);
    }

    int result = PyRun_InteractiveLoop(stdin, "stdin");
    if (result < 0) {
        return Tohil_ReturnExceptionToTcl(interp, "interactive loop failure");
    }

    return tohil_tcl_return(interp, TCL_OK);
}

/* Python library begins here */

//
//
// python tcl object "tclobj"
//
//

//
// return true if python object is a tclobj type
//
int
TohilTclObj_Check(PyObject *pyObj)
{
    return PyObject_TypeCheck(pyObj, &TohilTclObjType);
}

//
// create a new python tclobj object from a tclobj
//
static PyObject *
TohilTclObj_FromTclObj(Tcl_Interp *interp, Tcl_Obj *obj)
{
    TohilTclObj *self = (TohilTclObj *)TohilTclObjType.tp_alloc(&TohilTclObjType, 0);
    if (self != NULL) {
        self->interp = interp;
        self->tclobj = obj;
        self->to = NULL;
        self->tclvar = NULL;
        Tcl_IncrRefCount(obj);
    }
    return (PyObject *)self;
}

static void
tohil_dict_dump(PyObject *pt)
{
    PyObject *repr = PyObject_Repr(pt);
    PyObject *str = PyUnicode_AsEncodedString(repr, "utf-8", "~E~");
    const char *bytes = PyBytes_AS_STRING(str);
    printf("dict_dump: %s\n", bytes);
}

//
// create a new python tclobj object
//
// creates an empty object if no argument is given, but if an
// argument is provided, performs pyObjToTcl conversion on it
// and makes the new tclobj object point to that
//
static PyObject *
TohilTclObj_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *pDefault = NULL;
    PyObject *toType = NULL;
    char *tVar = NULL;
    static char *kwlist[] = {"default", "to", "var", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O$Os", kwlist, &pDefault, &toType, &tVar)) {
        return NULL;
    }

    // tohil_dict_dump(type);

    if (toType != NULL) {
        if (!PyType_Check(toType)) {
            PyErr_SetString(PyExc_TypeError, "to type is not a valid python data type");
            return NULL;
        }
    }

    // grab pointer to tcl interp out of tclobj type's dictionary
    PyObject *pCap = PyDict_GetItemString(type->tp_dict, "_interp");
    assert(pCap != NULL);
    Tcl_Interp *interp = PyCapsule_GetPointer(pCap, "tohil.interp");

    TohilTclObj *self = (TohilTclObj *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->interp = interp;
        self->tclvar = NULL;
        self->tclobj = NULL;

        Tcl_Obj *newObj;
        // it's from or empty if it gets to here but
        // if they specified a var= then we don't
        // want to clobber it

        // if there's a var, connect to that
        if (tVar != NULL) {
            self->tclvar = Tcl_NewStringObj(tVar, -1);
            Tcl_IncrRefCount(self->tclvar);

            // if they specified a default and the var doesn't
            // exist, stuff the var, but if it does exist,
            // just leave it alone.
            if (pDefault != NULL) {
                if (Tcl_ObjGetVar2(self->interp, self->tclvar, NULL, 0) == NULL) {
                    newObj = pyObjToTcl(self->interp, pDefault);
                    TohilTclObj_stuff_var(self, newObj);
                }
            }
        } else {
            // there isn't a var, fill the tclobj
            // if there's no source, make a new tclobj
            if (pDefault == NULL) {
                if (STREQU(type->tp_name, "tohil.tcldict")) {
                    newObj = Tcl_NewDictObj();
                } else {
                    newObj = Tcl_NewObj();
                }
            } else {
                // there's a source=, use that for the new tclobj
                newObj = pyObjToTcl(self->interp, pDefault);
            }
            self->tclobj = newObj;
            Tcl_IncrRefCount(self->tclobj);
        }
        self->to = (PyTypeObject *)toType;
        Py_XINCREF(toType);
    }
    return (PyObject *)self;
}

//
// deallocate function for python tclobj type
//
static void
TohilTclObj_dealloc(TohilTclObj *self)
{
    if (self->tclobj != NULL)
        Tcl_DecrRefCount(self->tclobj);
    if (self->tclvar != NULL)
        Tcl_DecrRefCount(self->tclvar);
    Py_XDECREF(self->to);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

//
// init function for python tclobj type
//
static int
TohilTclObj_init(TohilTclObj *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static Tcl_Obj *
TohilTclObj_objptr(TohilTclObj *self)
{
    if (self->tclobj != NULL) {
        assert(self->tclvar == NULL);
        return self->tclobj;
    }

    Tcl_Obj *obj = Tcl_ObjGetVar2(self->interp, self->tclvar, NULL, TCL_LEAVE_ERR_MSG);
    if (obj == NULL) {
        PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }
    return obj;
}

static int
TohilTclObj_stuff_var(TohilTclObj *self, Tcl_Obj *obj)
{
    assert(self->tclobj == NULL);
    Tcl_Obj *res = Tcl_ObjSetVar2(self->interp, self->tclvar, NULL, obj, TCL_LEAVE_ERR_MSG);
    if (res == NULL) {
        PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return -1;
    }
    Tcl_IncrRefCount(res);
    return 0;
}

static int
TohilTclObj_possibly_stuff_var(TohilTclObj *self, Tcl_Obj *obj)
{
    if (self->tclobj != NULL)
        return 0;

    return TohilTclObj_stuff_var(self, obj);
}

static int
TohilTclObj_stuff_objptr(TohilTclObj *self, Tcl_Obj *obj)
{
    if (self->tclobj != NULL) {
        assert(self->tclobj->refCount > 0);
        Tcl_DecrRefCount(self->tclobj);
        self->tclobj = obj;
        Tcl_IncrRefCount(self->tclobj);
        return 0;
    }

    return TohilTclObj_stuff_var(self, obj);
}

static Tcl_Obj *
TohilTclObj_writable_objptr(TohilTclObj *self)
{
    if (self->tclobj != NULL) {
        // assert(self->tclobj->refCount > 0);
        if (Tcl_IsShared(self->tclobj)) {
            Tcl_DecrRefCount(self->tclobj);
            self->tclobj = Tcl_DuplicateObj(self->tclobj);
        }
        return self->tclobj;
    }

    return Tcl_NewObj();

#if 0
    Tcl_Obj *obj = Tcl_ObjGetVar2(self->interp, self->tclvar, NULL, TCL_LEAVE_ERR_MSG);
    if (obj == NULL) {
        PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }
    if (Tcl_IsShared(obj)) {
        Tcl_DecrRefCount(obj);
        obj = Tcl_DuplicateObj(obj);
        // NB obj returned by set var 2 can change due to traces
        // this is probably no good; we need to set the var after
        // not before we set a value into it
        obj = Tcl_ObjSetVar2(self->interp, self->tclvar, NULL, obj, TCL_LEAVE_ERR_MSG);
        if (obj == NULL) {
            PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
            return NULL;
        }
    }
    return obj;
#endif
}

//
// str() method for python tclobj type
//
static PyObject *
TohilTclObj_str(TohilTclObj *self)
{
    int tclStringSize;
    Tcl_Obj *tclobj = TohilTclObj_objptr(self);
    if (tclobj == NULL)
        return NULL;
    char *tclString = Tcl_GetStringFromObj(tclobj, &tclStringSize);

    int utf8len;
    char *utf8string;
    if (tohil_TclToUTF8(self->interp, tclString, tclStringSize, &utf8string, &utf8len) != TCL_OK) {
        PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }
    PyObject *pObj = Py_BuildValue("s#", utf8string, utf8len);
    ckfree(utf8string);
    return pObj;
}

//
// repr() method for python tclobj type
//
static PyObject *
TohilTclObj_repr(TohilTclObj *self)
{
    Tcl_DString ds;
    Tcl_Obj *tclobj = TohilTclObj_objptr(self);
    if (tclobj == NULL)
        return NULL;
    char *utf8string = tohil_TclObjToUTF8DString(self->interp, tclobj, &ds);

    PyObject *stringRep = PyUnicode_FromFormat("%s", utf8string);
    // char *format = PyUnicode_GET_LENGTH(stringRep) > 100 ? "<%s: %.100R...>" : "<%s: %.100R>";
    char *format = "<%s: %R>";
    Tcl_DStringFree(&ds);
    PyObject *repr = PyUnicode_FromFormat(format, Py_TYPE(self)->tp_name, stringRep);
    Py_DECREF(stringRep);
    return repr;
}

//
// richcompare() method for python tclobj type
//
static PyObject *
TohilTclObj_richcompare(TohilTclObj *self, PyObject *other, int op)
{
    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;
    Tcl_Obj *otherobj = NULL;

    // if you want equal and they point to the exact same object,
    // we are donezo
    if (op == Py_EQ && (TohilTclObj_Check(other) || TohilTclDict_Check(other))) {
        otherobj = pyObjToTcl(self->interp, other);
        if (selfobj == otherobj) {
            Py_INCREF(Py_True);
            return Py_True;
        }
    }

    char *selfString = Tcl_GetString(selfobj);
    char *otherString = NULL;
    if (TohilTclObj_Check(other) || TohilTclDict_Check(other)) {
        otherobj = TohilTclObj_objptr((TohilTclObj *)other);

        if (otherobj == NULL)
            return NULL;

        otherString = Tcl_GetString(otherobj);
    } else {
        otherobj = pyObjToTcl(self->interp, other);
        otherString = Tcl_GetString(otherobj);
    }

    int cmp = strcmp(selfString, otherString);
    int res = 0;

    switch (op) {
    case Py_LT:
        res = (cmp < 0);
        break;

    case Py_LE:
        res = (cmp <= 0);
        break;

    case Py_EQ:
        res = (cmp == 0);
        break;

    case Py_NE:
        res = (cmp != 0);
        break;

    case Py_GT:
        res = (cmp > 0);
        break;

    case Py_GE:
        res = (cmp >= 0);
        break;

    default:
        assert(0 == 1);
    }
    PyObject *p = (res ? Py_True : Py_False);
    Py_INCREF(p);
    return p;
}

//
// tclobj.clear() - clear a tclobj or tcldict to an empty tcl object
//
static PyObject *
TohilTclObj_clear(TohilTclObj *self, PyObject *Py_UNUSED(ignored))
{
    if (self->tclvar != NULL) {
        Tcl_DecrRefCount(self->tclvar);
        self->tclvar = NULL;
    }

    if (self->tclobj != NULL) {
        Tcl_DecrRefCount(self->tclobj);
    }

    self->tclobj = Tcl_NewObj();
    Py_XDECREF(self->to);
    self->to = NULL;
    Tcl_IncrRefCount(self->tclobj);
    Py_RETURN_NONE;
}

//
// tclobj.as_dict()
//
static PyObject *
TohilTclObj_as_dict(TohilTclObj *self, PyObject *Py_UNUSED(ignored))
{
    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;
    return tclListObjToPyDictObject(self->interp, selfobj);
}

//
// tclobj.as_byte_array()
//
static PyObject *
TohilTclObj_as_byte_array(TohilTclObj *self, PyObject *Py_UNUSED(ignored))
{
    int size;
    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;
    unsigned char *byteArray = Tcl_GetByteArrayFromObj(selfobj, &size);
    return PyByteArray_FromStringAndSize((const char *)byteArray, size);
}

static void
TohilTclObj_dup_if_shared(TohilTclObj *self)
{
    if (!Tcl_IsShared(self->tclobj)) {
        return;
    }

    // decrement the old object.  It's safe because refcount
    // must be 2 or more.  then duplicate and increment
    // the new, duplicated object's 0 refcount to 1
    Tcl_DecrRefCount(self->tclobj);
    self->tclobj = Tcl_DuplicateObj(self->tclobj);
    Tcl_IncrRefCount(self->tclobj);
}

//
// tclobj.incr() - increment a python tclobj object
//
static PyObject *
TohilTclObj_incr(TohilTclObj *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"incr", NULL};
    long longValue = 0;
    long increment = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|l", kwlist, &increment)) {
        return NULL;
    }

    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;

    if (Tcl_GetLongFromObj(self->interp, selfobj, &longValue) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }

    longValue += increment;

    Tcl_Obj *writeObj = TohilTclObj_writable_objptr(self);
    if (writeObj == NULL)
        return NULL;

    Tcl_SetLongObj(writeObj, longValue);

    if (TohilTclObj_possibly_stuff_var(self, writeObj) < 0)
        return NULL;

    return PyLong_FromLong(longValue);
}

//
// convert a python list into a tcl c-level objv and objc
//
// pyListToTclObjv(interp, pList, &objc, &objv);
//
// you must call pyListToObjv_teardown when done or you'll
// leak memory
//
static void
pyListToTclObjv(Tcl_Interp *interp, PyListObject *pList, int *intPtr, Tcl_Obj ***objvPtr)
{
    int i;

    assert(PyList_Check(pList));
    Py_ssize_t objc = PyList_GET_SIZE(pList);
    // build up a tcl objv of the list elements
    Tcl_Obj **objv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * objc);
    for (i = 0; i < objc; i++) {
        objv[i] = pyObjToTcl(interp, PyList_GET_ITEM(pList, i));
        Tcl_IncrRefCount(objv[i]);
    }
    *objvPtr = objv;
    *intPtr = objc;
}

//
// teardown an objv created by pyListToObjv
//
static void
pyListToObjv_teardown(int objc, Tcl_Obj **objv)
{
    int i;

    // tear down the objv of the keys we created
    for (i = 0; i < objc; i++) {
        Tcl_DecrRefCount(objv[i]);
    }
    ckfree(objv);
}

//
// getvar - set python tclobj to contain the value of a tcl var
//
static PyObject *
TohilTclObj_getvar(TohilTclObj *self, PyObject *var)
{
    char *varString = (char *)PyUnicode_1BYTE_DATA(var);
    Tcl_Obj *newObj = Tcl_GetVar2Ex(self->interp, varString, NULL, (TCL_LEAVE_ERR_MSG));
    if (newObj == NULL) {
        PyErr_SetString(PyExc_NameError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }

    if (TohilTclObj_stuff_objptr(self, newObj) < 0)
        return NULL;

    Py_RETURN_NONE;
}

//
// setvar - set set tcl var to point to the tcl object of a python tclobj
//
static PyObject *
TohilTclObj_setvar(TohilTclObj *self, PyObject *var)
{
    char *varString = (char *)PyUnicode_1BYTE_DATA(var);
    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;
    // setvar handles incrementing the reference count
    if (Tcl_SetVar2Ex(self->interp, varString, NULL, selfobj, (TCL_LEAVE_ERR_MSG)) == NULL) {
        PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }
    Py_RETURN_NONE;
}

//
// set - tclobj type set method can set an object to a lot
// of possible python stuff -- NB there must be a better way
//
static PyObject *
TohilTclObj_set(TohilTclObj *self, PyObject *pyObject)
{
    Tcl_Obj *newObj = pyObjToTcl(self->interp, pyObject);
    if (newObj == NULL) {
        return NULL;
    }

    if (TohilTclObj_stuff_objptr(self, newObj) < 0)
        return NULL;

    Py_RETURN_NONE;
}

//
// lindex - obtain the n'th element of tclobj as a tcl list.
//
// to=type can be used to control what python type is returned.
//
static PyObject *
TohilTclObj_lindex(TohilTclObj *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"index", "to", NULL};
    PyTypeObject *to = NULL;
    int index = 0;
    int length = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|$O", kwlist, &index, &to))
        return NULL;

    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;

    if (Tcl_ListObjLength(self->interp, selfobj, &length) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }

    if (index < 0 || index >= length) {
        PyErr_SetString(PyExc_IndexError, "list index out of range");
        return NULL;
    }

    Tcl_Obj *resultObj = NULL;
    if (Tcl_ListObjIndex(self->interp, selfobj, index, &resultObj) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }

    if (to == NULL && self->to != NULL)
        to = self->to;

    return tohil_python_return(self->interp, TCL_OK, to, resultObj);
}

//
// lappend something to the tclobj
//
static PyObject *
TohilTclObj_lappend(TohilTclObj *self, PyObject *pObject)
{
    Tcl_Obj *newObj = pyObjToTcl(self->interp, pObject);
    if (newObj == NULL) {
        return NULL;
    }

    Tcl_Obj *writeObj = TohilTclObj_writable_objptr(self);
    if (writeObj == NULL)
        return NULL;

    if (Tcl_ListObjAppendElement(self->interp, writeObj, newObj) == TCL_ERROR) {
        PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        Tcl_DecrRefCount(newObj);
        return NULL;
    }

    if (TohilTclObj_possibly_stuff_var(self, writeObj) < 0)
        return NULL;

    // it worked
    Py_RETURN_NONE;
}

//
// lappend_list, lappend a list of something to the tclobj
//
// if append object is a tclobj and the tclobj contents can be
// treated as a list, it will be appended as the list
//
// if append object is a python list then its contents will be
// appended to the list while being converted to tcl objects
//
static PyObject *
TohilTclObj_lappend_list(TohilTclObj *self, PyObject *pObject)
{
    Tcl_Obj *writeObj = TohilTclObj_writable_objptr(self);
    if (writeObj == NULL)
        return NULL;

    // if passed python object is a tclobj, use tcl list C stuff
    // to append the list to the list
    if (TohilTclObj_Check(pObject)) {
        Tcl_Obj *appendListObj = TohilTclObj_objptr((TohilTclObj *)pObject);
        if (appendListObj == NULL)
            return NULL;

        if (Tcl_ListObjAppendList(self->interp, writeObj, appendListObj) == TCL_ERROR) {
            PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
            Tcl_DecrRefCount(appendListObj);
            return NULL;
        }
        // if passed python object is a python list, use that and our
        // python-to-tcl stuff to make a tcl list of it, and append
        // that list to the tclobj's object, which is a list or an error
        // is forthcoming
    } else if (PyList_Check(pObject)) {
        int objc;
        Tcl_Obj **objv = NULL;

        pyListToTclObjv(self->interp, (PyListObject *)pObject, &objc, &objv);
        Tcl_Obj *appendListObj = Tcl_NewListObj(objc, objv);
        pyListToObjv_teardown(objc, objv);

        if (Tcl_ListObjAppendList(self->interp, writeObj, appendListObj) == TCL_ERROR) {
            PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
            Tcl_DecrRefCount(appendListObj);
            return NULL;
        }
    } else {
        PyErr_Format(PyExc_TypeError, "lappend_list argument must be a tclobj or list, not %.200s", Py_TYPE(pObject)->tp_name);
        return NULL;
    }

    if (TohilTclObj_possibly_stuff_var(self, writeObj) < 0)
        return NULL;

    // it worked
    Py_RETURN_NONE;
}

//
// TohilTclObj_refcount - return the reference count of the
//   tclobj or tcldict's internal tcl object
//
static PyObject *
TohilTclObj_refcount(TohilTclObj *self, PyObject *Py_UNUSED(ignored))
{
    Tcl_Obj *obj = TohilTclObj_objptr(self);
    if (obj == NULL)
        return NULL;
    return PyLong_FromLong(obj->refCount);
}

//
// TohilTclObj_pyrefcount - return the python reference count of the
//   tclobj or tcldict object
//
static PyObject *
TohilTclObj_pyrefcount(TohilTclObj *self, PyObject *Py_UNUSED(ignored))
{
    return PyLong_FromLong(Py_REFCNT(self));
}

//
// TohilTclObj_type - return the internal tcl data type of
//   the corresponding tclobj/tcldict python type
//
//   this can use useful but it's not canonical - the type
//   may not have been set to anything if the string therein
//   hasn't been used as a list, dict, etc.
//
static PyObject *
TohilTclObj_type(TohilTclObj *self, PyObject *Py_UNUSED(ignored))
{
    Tcl_Obj *obj = TohilTclObj_objptr(self);
    if (obj == NULL)
        return NULL;
    const Tcl_ObjType *typePtr = obj->typePtr;
    if (typePtr == NULL) {
        Py_RETURN_NONE;
    }
    return Py_BuildValue("s", obj->typePtr->name);
}

//
// insert something to the tclobj list
//
static PyObject *
TohilTclObj_insert(TohilTclObj *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"i", "x", NULL};
    int index = 0;
    PyObject *pyInsertObj = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iO", kwlist, &index, &pyInsertObj))
        return NULL;

    Tcl_Obj *newObj = pyObjToTcl(self->interp, pyInsertObj);
    if (newObj == NULL) {
        return NULL;
    }

    Tcl_Obj *writeObj = TohilTclObj_writable_objptr(self);
    if (writeObj == NULL)
        return NULL;

    if (Tcl_ListObjReplace(self->interp, writeObj, index, 0, 1, &newObj) == TCL_ERROR) {
        PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        Tcl_DecrRefCount(newObj);
        return NULL;
    }

    if (TohilTclObj_possibly_stuff_var(self, writeObj) < 0)
        return NULL;

    // it worked
    Py_RETURN_NONE;
}

//
// TohilTclObj_pop - remove the item at the given position in the list, and
//   return it.  if no index is specified, remove and return the last item
//   in the list.  raise IndexError: pop from empty list if list is empty.
//
//
static PyObject *
TohilTclObj_pop(TohilTclObj *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"i", "to", NULL};
    int i = -999;
    PyTypeObject *to = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i$O", kwlist, &i, &to)) {
        return NULL;
    }

    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;

    int size = 0;
    if (Tcl_ListObjLength(self->interp, selfobj, &size) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }

    if (size == 0) {
        PyErr_SetString(PyExc_IndexError, "pop from empty list");
        return NULL;
    }

    if (i == -999)
        i = size - 1;
    else if (i < 0 || i >= size) {
        PyErr_SetString(PyExc_IndexError, "pop index out of range");
        return NULL;
    }

    // get the item
    Tcl_Obj *resultObj = NULL;
    if (Tcl_ListObjIndex(self->interp, selfobj, i, &resultObj) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }
    Tcl_IncrRefCount(resultObj);

    if (Tcl_IsShared(selfobj)) {
        Tcl_DecrRefCount(selfobj);
        selfobj = Tcl_DuplicateObj(selfobj);
    }

    // remove the item for the list
    if (Tcl_ListObjReplace(self->interp, selfobj, i, 1, 0, NULL) == TCL_ERROR) {
        PyErr_SetString(PyExc_IndexError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }
    if (TohilTclObj_possibly_stuff_var(self, selfobj) < 0)
        return NULL;

    if (to == NULL && self->to != NULL)
        to = self->to;

    return tohil_python_return(self->interp, TCL_OK, to, resultObj);
}

static PyObject *TohilTclObj_subscript(TohilTclObj *, PyObject *);

//
// TohilTclObj_slice - return a python list containing a slice
//   of the referenced tclobj as a list.
//
// significantly cribbed from cpython source for listobjects...
//
static PyObject *
TohilTclObj_slice(TohilTclObj *self, Py_ssize_t ilow, Py_ssize_t ihigh)
{
    PyListObject *np;
    int listObjc;
    Tcl_Obj **listObjv;
    Tcl_Obj **src;
    Py_ssize_t i, len;
    len = ihigh - ilow;

    int size = 0;

    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;

    // get the list size and crack the list into a list objc and objv.
    // any failure here probably means the object can't be represented as a list.
    if (Tcl_ListObjLength(self->interp, selfobj, &size) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }

    if (Tcl_ListObjGetElements(self->interp, selfobj, &listObjc, &listObjv) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }

    np = (PyListObject *)PyList_New(len);
    if (np == NULL) {
        return NULL;
    }

    // src is a pointer to an array of pointers of obj,
    // adjust it to the starting object and we'll walk it forward
    src = &listObjv[ilow];
    for (i = 0; i < len; i++) {
        // create a new tclobj object and store
        // it into the python list we are making
        PyObject *v = tohil_python_return(self->interp, TCL_OK, self->to, src[i]);
        PyList_SET_ITEM(np, i, v);
    }
    return (PyObject *)np;
}

//
// TohilTclObj_item - return the i'th element of a tclobj containing
//   a list, or set an error and return NULL if something's wrong
//
static PyObject *
TohilTclObj_item(TohilTclObj *self, Py_ssize_t i)
{
    int size = 0;

    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;

    if (Tcl_ListObjLength(self->interp, selfobj, &size) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }

    if (i < 0 || i >= size) {
        PyErr_SetString(PyExc_IndexError, "list index out of range");
        return NULL;
    }

    int listObjc;
    Tcl_Obj **listObjv;

    if (Tcl_ListObjGetElements(self->interp, selfobj, &listObjc, &listObjv) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }

    PyObject *ret = tohil_python_return(self->interp, TCL_OK, self->to, listObjv[i]);
    Py_INCREF(ret);
    return ret;
}

//
// TohilTclObj_ass_item - assign an item into a tclobj containing a list
//
static int
TohilTclObj_ass_item(TohilTclObj *self, Py_ssize_t i, PyObject *v)
{
    int size = 0;
    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return -1;

    if (Tcl_ListObjLength(self->interp, selfobj, &size) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return -1;
    }

    if (i < 0 || i >= size) {
        PyErr_SetString(PyExc_IndexError, "list assignment index out of range");
        return -1;
    }

    if (v == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "slice assignment not implemented");
        return -1;
        // return list_ass_slice(self, i, i+1, v);
    }

    Tcl_Obj *obj = pyObjToTcl(self->interp, v);
    if (obj == NULL) {
        return -1;
    }

    Tcl_Obj *writeObj = TohilTclObj_objptr(self);
    if (writeObj == NULL)
        return -1;

    if (Tcl_IsShared(writeObj)) {
        Tcl_DecrRefCount(writeObj);
        writeObj = Tcl_DuplicateObj(writeObj);
    }

    if (Tcl_ListObjReplace(self->interp, writeObj, i, 1, 1, &obj) == TCL_ERROR) {
        PyErr_SetString(PyExc_IndexError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return -1;
    }

    if (TohilTclObj_possibly_stuff_var(self, writeObj) < 0)
        return -1;

    return 0;
}

//
// TohilTclObj_length - return the length of the tclobj, looking at it
//   as a tcl list.  returns -1 if string cannot be represented as
//   a list, or 0 or larger, representing the number of elements
//   in the list.
//
//
static Py_ssize_t
TohilTclObj_length(TohilTclObj *self, Py_ssize_t i)
{
    int size = 0;
    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return -1;

    if (Tcl_ListObjLength(self->interp, selfobj, &size) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return -1;
    }

    return size;
}

//
// concatenate a tclobj object with some other python
//   object, possible a tclobj object, but not necessarily,
//   and return the result as a str
//
//   will be invoked by tclobj "+" operator when tclobj doesn't
//   contain a number
//
static PyObject *
TohilTclObj_concat(TohilTclObj *self, PyObject *item)
{
    Tcl_Obj *tItem;
    if (TohilTclObj_Check(item)) {
        tItem = TohilTclObj_objptr((TohilTclObj *)item);
    } else {
        tItem = _pyObjToTcl(self->interp, item);
        if (tItem == NULL)
            Py_RETURN_NOTIMPLEMENTED;
    }

    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;

    Tcl_Obj *returnObj = Tcl_DuplicateObj(selfobj);
    Tcl_AppendObjToObj(returnObj, tItem);
    Tcl_DString ds;
    PyObject *pRet = Py_BuildValue("s", tohil_TclObjToUTF8DString(self->interp, returnObj, &ds));
    Tcl_DecrRefCount(returnObj);
    return pRet;
}

//
// in-place concatenation - concatenate something grabbed from
//   a python object onto the end of a tclobj object.
//
//   this will be invoked by tclobj "+=" operator when tclobj doesn't
//   contain a number
//
static PyObject *
TohilTclObj_inplace_concat(TohilTclObj *self, PyObject *item)
{
    Tcl_Obj *tItem = NULL;
    if (TohilTclObj_Check(item)) {
        tItem = TohilTclObj_objptr((TohilTclObj *)item);
        if (tItem == NULL)
            return NULL;
    } else {
        tItem = _pyObjToTcl(self->interp, item);
        if (tItem == NULL)
            Py_RETURN_NOTIMPLEMENTED;
    }

    Tcl_Obj *writeObj = TohilTclObj_objptr(self);
    if (writeObj == NULL)
        return NULL;

    if (Tcl_IsShared(writeObj)) {
        Tcl_DecrRefCount(writeObj);
        writeObj = Tcl_DuplicateObj(writeObj);
    }

    Tcl_AppendObjToObj(writeObj, tItem);

    if (self->tclobj != NULL) {
        self->tclobj = writeObj;
    } else {
        if (TohilTclObj_stuff_var(self, writeObj) < 0)
            return NULL;
    }

    Py_INCREF(self);
    return (PyObject *)self;
}

//
// TohilTclObj_subscript - subscript a tclobj.  we treat the tclobj
//   as a list, so item can be an integer (to obtain a single element),
//   or a slice (to obtain zero or more elements.)
//
static PyObject *
TohilTclObj_subscript(TohilTclObj *self, PyObject *item)
{
    int size = 0;

    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;

    if (Tcl_ListObjLength(self->interp, selfobj, &size) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }

    if (PyIndex_Check(item)) {
        Py_ssize_t i;
        i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += size;

        if (i < 0 || i >= size) {
            PyErr_SetString(PyExc_IndexError, "list index out of range");
            return NULL;
        }

        Tcl_Obj *resultObj = NULL;
        if (Tcl_ListObjIndex(self->interp, selfobj, i, &resultObj) == TCL_ERROR) {
            PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
            return NULL;
        }
        return tohil_python_return(self->interp, TCL_OK, self->to, resultObj);
    } else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength, i;
        size_t cur;
        PyObject *result;
        PyObject *it;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }

        slicelength = PySlice_AdjustIndices(size, &start, &stop, step);

        if (slicelength <= 0) {
            return tohil_python_return(self->interp, TCL_OK, self->to, Tcl_NewObj());
        } else if (step == 1) {
            return TohilTclObj_slice(self, start, stop);
        } else {
            result = PyList_New(slicelength);
            if (!result)
                return NULL;

            int listObjc;
            Tcl_Obj **listObjv;

            if (Tcl_ListObjGetElements(self->interp, selfobj, &listObjc, &listObjv) == TCL_ERROR) {
                PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
                return NULL;
            }

            for (cur = start, i = 0; i < slicelength; cur += (size_t)step, i++) {
                it = tohil_python_return(self->interp, TCL_OK, self->to, listObjv[cur]);
                PyList_SET_ITEM(result, i, it);
            }
            return result;
        }
    } else {
        PyErr_Format(PyExc_TypeError, "list indices must be integers or slices, not %.200s", Py_TYPE(item)->tp_name);
        return NULL;
    }
}

//
// tclobj iterator type
//

typedef struct {
    PyObject_HEAD;
    TohilTclObj *tohilObj;
    int i;
} TohilTclObj_IterObj;

//
// TohilTclObj_iter() - returns a tclobj iterator object that can
//   iterate over a tclobj.
//
static PyObject *
TohilTclObjIter(TohilTclObj *self)
{
    // printf("TohilTclObjIter\n");

    TohilTclObj_IterObj *pIter = (TohilTclObj_IterObj *)PyObject_New(TohilTclObj_IterObj, &TohilTclObj_IterType);

    pIter->tohilObj = self;
    if (self->tclobj != NULL) {
        Tcl_IncrRefCount(self->tclobj);
    }
    Py_INCREF(self);
    pIter->i = 0;
    Py_INCREF(pIter);
    return (PyObject *)pIter;
}

//
// tclobj's iternext
//
static PyObject *
TohilTclObj_iternext(TohilTclObj_IterObj *self)
{
    // printf("TohilTclObj_iternext\n");
    int length = 0;

    Tcl_Obj *selfobj = TohilTclObj_objptr(self->tohilObj);
    Tcl_Interp *interp = self->tohilObj->interp;
    if (Tcl_ListObjLength(interp, selfobj, &length) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
        return NULL;
    }

    if (self->i >= length) {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    Tcl_Obj *resultObj = NULL;
    if (Tcl_ListObjIndex(interp, selfobj, self->i, &resultObj) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
        return NULL;
    }

    self->i++;
    return tohil_python_return(interp, TCL_OK, NULL, resultObj);
}

//
// deallocate function for python tclobj iter object types
//
static void
TohilTclObjIter_dealloc(TohilTclObj_IterObj *self)
{
    // NB we need to do var shadowing with tcldicts too
    if (self->tohilObj->tclobj != NULL) {
        Tcl_DecrRefCount(self->tohilObj->tclobj);
    }
    Py_XDECREF(self->tohilObj);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyTypeObject TohilTclObj_IterType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "tohil_tclobj_iter",
    .tp_basicsize = sizeof(TohilTclObj_IterObj),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "tohil tclobj iterator type",
    .tp_dealloc = (destructor)TohilTclObjIter_dealloc,
    .tp_iter = (getiterfunc)TohilTclObjIter,
    .tp_iternext = (iternextfunc)TohilTclObj_iternext,
};

//
// end of python tcl object "tclobj"
//
//

//
//
// start of tcldict iterator python datatype
//
//

typedef struct {
    PyObject_HEAD;
    int started;
    int done;
    PyTypeObject *to;
    Tcl_Interp *interp;
    Tcl_Obj *dictObj;
    Tcl_DictSearch search;
} Tohil_TD_IterObj;

static PyObject *
Tohil_TD_iter(Tohil_TD_IterObj *self)
{
    // printf("Tohil_TD_iter\n");
    Py_INCREF(self);

    self->started = 0;
    self->done = 0;

    return (PyObject *)self;
}

enum td_itertype { Iter, Keys, Values, Items };

//
// td's iternext - we have to do the DictObjFirst here since
//   tcl's version, first gives you also the first result.
//
static PyObject *
Tohil_TD_multi_iternext(Tohil_TD_IterObj *self, enum td_itertype itertype)
{
    // printf("Tohil_TD_iternext\n");
    Tcl_Obj *keyObj = NULL;
    Tcl_Obj *valueObj = NULL;
    int done = 0;

    int utf8len;
    char *utf8string;

    if (self->done) {
    done:
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    if (self->started == 0) {
        self->started = 1;
        // substitute &valueObj for NULL below if you also want the value
        if (Tcl_DictObjFirst(self->interp, self->dictObj, &self->search, &keyObj, &valueObj, &done) == TCL_ERROR) {
            PyErr_Format(PyExc_TypeError, "tclobj contents cannot be converted into a td");
            return NULL;
        }
    } else {
        // not the first time though, get the next entry
        Tcl_DictObjNext(&self->search, &keyObj, &valueObj, &done);
    }

    if (done) {
        Tcl_DictObjDone(&self->search);
        self->done = 1;
        Tcl_DecrRefCount(self->dictObj);
        self->dictObj = NULL;
        Py_XDECREF(self->to);
        self->to = NULL;
        goto done;
    }

    if (itertype == Keys || itertype == Values || (itertype == Iter && self->to == NULL)) {
        int tclStringSize;
        char *tclString;

        if (itertype != Values) {
            tclString = Tcl_GetStringFromObj(keyObj, &tclStringSize);
        } else {
            tclString = Tcl_GetStringFromObj(valueObj, &tclStringSize);
        }

        if (tohil_TclToUTF8(self->interp, tclString, tclStringSize, &utf8string, &utf8len) != TCL_OK) {
            PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
            return NULL;
        }
        PyObject *pObj = Py_BuildValue("s#", utf8string, utf8len);
        ckfree(utf8string);
        return pObj;
    }

    // they specified a to, return a tuple
    PyObject *pRetTuple = PyTuple_New(2);

    int tclStringSize;
    char *tclString = Tcl_GetStringFromObj(keyObj, &tclStringSize);

    if (tohil_TclToUTF8(self->interp, tclString, tclStringSize, &utf8string, &utf8len) != TCL_OK) {
        PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }
    PyTuple_SET_ITEM(pRetTuple, 0, Py_BuildValue("s#", utf8string, utf8len));
    ckfree(utf8string);

    if (self->to == NULL) {
        // no "to" specified, stuff the value into item 1 of the tuple
        tclString = Tcl_GetStringFromObj(valueObj, &tclStringSize);
        if (tohil_TclToUTF8(self->interp, tclString, tclStringSize, &utf8string, &utf8len) != TCL_OK) {
            PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
            return NULL;
        }
        PyTuple_SET_ITEM(pRetTuple, 1, Py_BuildValue("s#", utf8string, utf8len));
        ckfree(utf8string);
    } else {
        PyTuple_SET_ITEM(pRetTuple, 1, tohil_python_return(self->interp, TCL_OK, self->to, valueObj));
    }

    return pRetTuple;
}

static PyObject *
Tohil_TD_iternext(Tohil_TD_IterObj *self)
{
    return Tohil_TD_multi_iternext(self, Iter);
}

static PyObject *
Tohil_TD_keys_iternext(Tohil_TD_IterObj *self)
{
    return Tohil_TD_multi_iternext(self, Keys);
}

static PyObject *
Tohil_TD_values_iternext(Tohil_TD_IterObj *self)
{
    return Tohil_TD_multi_iternext(self, Values);
}

static PyObject *
Tohil_TD_items_iternext(Tohil_TD_IterObj *self)
{
    return Tohil_TD_multi_iternext(self, Items);
}

static PyObject *
Tohil_TD_iter_repr(_PyDictViewObject *dv)
{
    PyObject *seq;
    PyObject *result = NULL;
    Py_ssize_t rc;

    rc = Py_ReprEnter((PyObject *)dv);
    if (rc != 0) {
        return rc > 0 ? PyUnicode_FromString("...") : NULL;
    }
    seq = PySequence_List((PyObject *)dv);
    if (seq == NULL) {
        goto Done;
    }
    result = PyUnicode_FromFormat("%s(%R)", Py_TYPE(dv)->tp_name, seq);
    Py_DECREF(seq);

Done:
    Py_ReprLeave((PyObject *)dv);
    return result;
}

//
// deallocate function for python tcldict iter object types
//
static void
TohilTclDictIter_dealloc(Tohil_TD_IterObj *self)
{
    // NB we need to do var shadowing with tcldicts too
    if (self->dictObj != NULL)
        Tcl_DecrRefCount(self->dictObj);
    Py_XDECREF(self->to);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyTypeObject Tohil_TD_IterType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "tohil.td_iter",
    .tp_basicsize = sizeof(Tohil_TD_IterObj),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "tohil TD iterator type",
    .tp_dealloc = (destructor)TohilTclDictIter_dealloc,
    .tp_iter = (getiterfunc)Tohil_TD_iter,
    .tp_iternext = (iternextfunc)Tohil_TD_iternext,
    .tp_repr = (reprfunc)Tohil_TD_iter_repr,
};

static PyTypeObject Tohil_TD_IterKeysType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "tohil.td_keys",
    .tp_basicsize = sizeof(Tohil_TD_IterObj),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "tohil TD keys iterator object",
    .tp_dealloc = (destructor)TohilTclDictIter_dealloc,
    .tp_iter = (getiterfunc)Tohil_TD_iter,
    .tp_iternext = (iternextfunc)Tohil_TD_keys_iternext,
    .tp_repr = (reprfunc)Tohil_TD_iter_repr,
};

static PyTypeObject Tohil_TD_IterValuesType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "tohil.td_values",
    .tp_basicsize = sizeof(Tohil_TD_IterObj),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "tohil TD values iterator object",
    .tp_dealloc = (destructor)TohilTclDictIter_dealloc,
    .tp_iter = (getiterfunc)Tohil_TD_iter,
    .tp_iternext = (iternextfunc)Tohil_TD_values_iternext,
    .tp_repr = (reprfunc)Tohil_TD_iter_repr,
};

static PyTypeObject Tohil_TD_IterItemsType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "tohil.td_items",
    .tp_basicsize = sizeof(Tohil_TD_IterObj),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "tohil TD items iterator object",
    .tp_dealloc = (destructor)TohilTclDictIter_dealloc,
    .tp_iter = (getiterfunc)Tohil_TD_iter,
    .tp_iternext = (iternextfunc)Tohil_TD_items_iternext,
    .tp_repr = (reprfunc)Tohil_TD_iter_repr,
};

//
//
// end of tclobj td_iterator python datatype
//
//

//
// tclobj number methods
//

static int
tclobj_nb_bool(TohilTclObj *self)
{
    int intValue = 0;

    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return -1;

    if (Tcl_GetBooleanFromObj(self->interp, selfobj, &intValue) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return -1;
    }
    return intValue;
}

static PyObject *
tclobj_nb_long(PyObject *p)
{
    long longValue = 0;

    TohilTclObj *self = (TohilTclObj *)p;
    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;

    if (Tcl_GetLongFromObj(self->interp, selfobj, &longValue) == TCL_ERROR) {
        double doubleValue = 0;
        if (Tcl_GetDoubleFromObj(NULL, selfobj, &doubleValue) == TCL_OK) {
            return PyLong_FromDouble(doubleValue);
        }
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }
    return PyLong_FromLong(longValue);
}

static PyObject *
tclobj_nb_float(PyObject *p)
{
    double doubleValue = 0;
    TohilTclObj *self = (TohilTclObj *)p;
    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;

    if (Tcl_GetDoubleFromObj(self->interp, selfobj, &doubleValue) == TCL_ERROR) {
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
        return NULL;
    }
    return PyFloat_FromDouble(doubleValue);
}

static int
tohil_pyobj_to_number(PyObject *v, long *longPtr, double *doublePtr, Tcl_Interp **interpPtr)
{
    if (PyLong_Check(v)) {
        *longPtr = PyLong_AsLong(v);
        return 0;
    }

    if (PyFloat_Check(v)) {
        *doublePtr = PyFloat_AsDouble(v);
        return 1;
    }

    if (TohilTclObj_Check(v)) {
        TohilTclObj *self = (TohilTclObj *)v;

        // if the passed in pointer to an interpreter pointer isn't null,
        // store the interpreter pointer
        if (interpPtr != NULL)
            *interpPtr = self->interp;

        // get the tclobj pointer or the tcl object referenced by the tclvar
        Tcl_Obj *selfobj = TohilTclObj_objptr(self);
        if (selfobj == NULL)
            return -1;

        if (Tcl_GetLongFromObj(self->interp, selfobj, longPtr) == TCL_OK) {
            return 0;
        }

        if (Tcl_GetDoubleFromObj(NULL, selfobj, doublePtr) == TCL_OK) {
            return 1;
        }

        return -1;
    }
    return 2;
}

enum tclobj_unary_op { Abs, Negative, Positive, Invert };

static PyObject *
tclobj_nb_unaryop(PyObject *v, enum tclobj_unary_op operator)
{
    Tcl_Interp *interp = NULL;
    double doubleV = 0.0;
    long longV = 0;
    int vFloat = tohil_pyobj_to_number(v, &longV, &doubleV, &interp);
    if (vFloat < 0) {
        assert(interp != NULL);
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
        return NULL;
    }

    if (vFloat) {
        switch (operator) {
        case Abs:
            return PyFloat_FromDouble(fabs(doubleV));

        case Negative:
            return PyFloat_FromDouble(-doubleV);

        case Positive:
            return PyFloat_FromDouble(doubleV);

        default:
            Py_RETURN_NOTIMPLEMENTED;
        }
    } else {
        switch (operator) {
        case Abs:
            return PyLong_FromLong(labs(longV));

        case Negative:
            return PyLong_FromLong(-longV);

        case Positive:
            return PyLong_FromLong(longV);

        case Invert:
            return PyLong_FromLong(~longV);

        default:
            Py_RETURN_NOTIMPLEMENTED;
        }
    }
}

enum tclobj_op { Add, Sub, Mul, And, Or, Xor, Lshift, Rshift, Remainder, Divmod, Truediv, Floordiv };

static PyObject *
tclobj_nb_binop(PyObject *v, PyObject *w, enum tclobj_op operator)
{
    double doubleV = 0.0;
    long longV = 0;
    Tcl_Interp *vInterp = NULL;
    int vFloat = tohil_pyobj_to_number(v, &longV, &doubleV, &vInterp);

    double doubleW = 0.0;
    long longW = 0;
    Tcl_Interp *wInterp = NULL;
    int wFloat = tohil_pyobj_to_number(w, &longW, &doubleW, &wInterp);

    ldiv_t ldiv_res;
    double quotient;
    double remainder;

    if (vFloat < 0 || wFloat < 0) {
        if (operator== Add) {
            Py_RETURN_NOTIMPLEMENTED;
        }
        Tcl_Interp *interp = (vInterp != NULL) ? vInterp : wInterp;
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
        return NULL;
    } else if ((vFloat == 2) || (wFloat == 2)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (vFloat || wFloat) {
        if (!wFloat) {
            doubleW = longW;
        } else if (!vFloat) {
            doubleV = longV;
        }

        switch (operator) {
        case Add:
            return PyFloat_FromDouble(doubleV + doubleW);

        case Sub:
            return PyFloat_FromDouble(doubleV - doubleW);

        case Mul:
            return PyFloat_FromDouble(doubleV * doubleW);

        case Truediv:
            if (doubleW == 0.0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "float division by zero");
                return NULL;
            }
            return PyFloat_FromDouble(doubleV / doubleW);

        case Floordiv:
            if (doubleW == 0.0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "float division by zero");
                return NULL;
            }
            return PyFloat_FromDouble(floor(doubleV / doubleW));

        case Remainder:
            if (doubleW == 0.0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "float division by zero");
                return NULL;
            }
            return PyFloat_FromDouble(fmod(fmod(doubleV, doubleW) + doubleW, doubleW));

        case Divmod:
            if (doubleW == 0.0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "float division by zero");
                return NULL;
            }
            quotient = doubleV / doubleW;
            remainder = fmod(doubleV, doubleW);
            return Py_BuildValue("dd", quotient, remainder);

        default:
            Py_RETURN_NOTIMPLEMENTED;
        }
    } else {
        switch (operator) {
        case Add:
            return PyLong_FromLong(longV + longW);

        case Sub:
            return PyLong_FromLong(longV - longW);

        case Mul:
            return PyLong_FromLong(longV * longW);

        case And:
            return PyLong_FromLong(longV & longW);

        case Or:
            return PyLong_FromLong(longV | longW);

        case Xor:
            return PyLong_FromLong(longV ^ longW);

        case Lshift:
            return PyLong_FromLong(longV << longW);

        case Rshift:
            return PyLong_FromLong(longV >> longW);

        case Truediv:
            if (longW == 0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "division by zero");
                return NULL;
            }
            return PyFloat_FromDouble((double)longV / (double)longW);

        case Floordiv:
            if (longW == 0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "division by zero");
                return NULL;
            }
            ldiv_res = ldiv(longV, longW);
            if (ldiv_res.rem != 0 && ((longV < 0) ^ (longW < 0))) {
                ldiv_res.quot--;
            }
            return PyLong_FromLong(ldiv_res.quot);

        case Remainder:
            if (longW == 0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "division by zero");
                return NULL;
            }
            return PyLong_FromLong(((longV % longW) + longW) % longW);

        case Divmod:
            if (longW == 0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "division by zero");
                return NULL;
            }
            ldiv_res = ldiv(longV, longW);
            return Py_BuildValue("ll", ldiv_res.quot, ldiv_res.rem);

        default:
            Py_RETURN_NOTIMPLEMENTED;
        }
    }
}

static PyObject *
tclobj_nb_add(PyObject *v, PyObject *w)
{
    return tclobj_nb_binop(v, w, Add);
}

static PyObject *
tclobj_nb_subtract(PyObject *v, PyObject *w)
{
    return tclobj_nb_binop(v, w, Sub);
}

static PyObject *
tclobj_nb_multiply(PyObject *v, PyObject *w)
{
    return tclobj_nb_binop(v, w, Mul);
}

static PyObject *
tclobj_nb_true_divide(PyObject *v, PyObject *w)
{
    return tclobj_nb_binop(v, w, Truediv);
}

static PyObject *
tclobj_nb_floor_divide(PyObject *v, PyObject *w)
{
    return tclobj_nb_binop(v, w, Floordiv);
}

static PyObject *
tclobj_nb_remainder(PyObject *v, PyObject *w)
{
    return tclobj_nb_binop(v, w, Remainder);
}

static PyObject *
tclobj_nb_divmod(PyObject *v, PyObject *w)
{
    return tclobj_nb_binop(v, w, Divmod);
}

static PyObject *
tclobj_nb_and(PyObject *v, PyObject *w)
{
    return tclobj_nb_binop(v, w, And);
}

static PyObject *
tclobj_nb_or(PyObject *v, PyObject *w)
{
    return tclobj_nb_binop(v, w, Or);
}

static PyObject *
tclobj_nb_xor(PyObject *v, PyObject *w)
{
    return tclobj_nb_binop(v, w, Xor);
}

static PyObject *
tclobj_nb_lshift(PyObject *v, PyObject *w)
{
    return tclobj_nb_binop(v, w, Lshift);
}

static PyObject *
tclobj_nb_rshift(PyObject *v, PyObject *w)
{
    return tclobj_nb_binop(v, w, Rshift);
}

static PyObject *
tclobj_nb_negative(PyObject *v, enum tclobj_op operator)
{
    return tclobj_nb_unaryop(v, Negative);
}

static PyObject *
tclobj_nb_positive(PyObject *v, enum tclobj_op operator)
{
    return tclobj_nb_unaryop(v, Positive);
}

static PyObject *
tclobj_nb_absolute(PyObject *v, enum tclobj_op operator)
{
    return tclobj_nb_unaryop(v, Abs);
}

static PyObject *
tclobj_nb_invert(PyObject *v, enum tclobj_op operator)
{
    return tclobj_nb_unaryop(v, Invert);
}

static PyObject *
tclobj_nb_inplace_binop(PyObject *v, PyObject *w, enum tclobj_op operator)
{
    // NB same chunk of code in tclobj_binop
    double doubleV = 0.0;
    long longV = 0;
    Tcl_Interp *vInterp = NULL;
    int vFloat = tohil_pyobj_to_number(v, &longV, &doubleV, &vInterp);

    double doubleW = 0.0;
    long longW = 0;
    Tcl_Interp *wInterp = NULL;
    int wFloat = tohil_pyobj_to_number(w, &longW, &doubleW, &wInterp);

    ldiv_t ldiv_res;

    if (vFloat < 0 || wFloat < 0) {
        if (operator== Add) {
            Py_RETURN_NOTIMPLEMENTED;
        }
        Tcl_Interp *interp = (vInterp != NULL) ? vInterp : wInterp;
        PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
        return NULL;
    } else if ((vFloat == 2) || (wFloat == 2)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    assert(TohilTclObj_Check(v));
    TohilTclObj *self = (TohilTclObj *)v;

    Tcl_Obj *writeObj = TohilTclObj_writable_objptr(self);

    if (vFloat || wFloat) {
        if (!wFloat) {
            doubleW = longW;
        } else if (!vFloat) {
            doubleV = longV;
        }

        switch (operator) {
        case Add:
            Tcl_SetDoubleObj(writeObj, doubleV + doubleW);
            break;

        case Sub:
            Tcl_SetDoubleObj(writeObj, doubleV - doubleW);
            break;

        case Mul:
            Tcl_SetDoubleObj(writeObj, doubleV * doubleW);
            break;

        case Remainder:
            if (doubleW == 0.0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "float division by zero");
                return NULL;
            }
            Tcl_SetDoubleObj(writeObj, fmod(fmod(doubleV, doubleW) + doubleW, doubleW));
            break;

        case Truediv:
            if (doubleW == 0.0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "float division by zero");
                return NULL;
            }
            Tcl_SetDoubleObj(writeObj, doubleV / doubleW);
            break;

        case Floordiv:
            if (doubleW == 0.0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "float division by zero");
                return NULL;
            }
            Tcl_SetDoubleObj(writeObj, floor(doubleV / doubleW));
            break;

        default:
            Py_RETURN_NOTIMPLEMENTED;
        }
    } else {
        switch (operator) {
        case Add:
            Tcl_SetLongObj(writeObj, longV + longW);
            break;

        case Sub:
            Tcl_SetLongObj(writeObj, longV - longW);
            break;

        case Mul:
            Tcl_SetLongObj(writeObj, longV * longW);
            break;

        case And:
            Tcl_SetLongObj(writeObj, longV & longW);
            break;

        case Or:
            Tcl_SetLongObj(writeObj, longV | longW);
            break;

        case Xor:
            Tcl_SetLongObj(writeObj, longV ^ longW);
            break;

        case Lshift:
            Tcl_SetLongObj(writeObj, longV << longW);
            break;

        case Rshift:
            Tcl_SetLongObj(writeObj, longV >> longW);
            break;

        case Truediv:
            if (longW == 0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "division by zero");
                return NULL;
            }
            Tcl_SetDoubleObj(writeObj, (double)longV / (double)longW);
            break;

        case Floordiv:
            if (longW == 0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "division by zero");
                return NULL;
            }
            ldiv_res = ldiv(longV, longW);
            if (ldiv_res.rem != 0 && ((longV < 0) ^ (longW < 0))) {
                ldiv_res.quot--;
            }
            Tcl_SetLongObj(writeObj, ldiv_res.quot);
            break;

        case Remainder:
            if (longW == 0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "division by zero");
                return NULL;
            }
            Tcl_SetLongObj(writeObj, ((longV % longW) + longW) % longW);
            break;

        default:
            Py_RETURN_NOTIMPLEMENTED;
        }
    }

    if (TohilTclObj_possibly_stuff_var(self, writeObj) < 0)
        return NULL;

    Py_INCREF(v);
    return v;
}

static PyObject *
tclobj_nb_inplace_add(PyObject *v, PyObject *w)
{
    return tclobj_nb_inplace_binop(v, w, Add);
}

static PyObject *
tclobj_nb_inplace_subtract(PyObject *v, PyObject *w)
{
    return tclobj_nb_inplace_binop(v, w, Sub);
}

static PyObject *
tclobj_nb_inplace_multiply(PyObject *v, PyObject *w)
{
    return tclobj_nb_inplace_binop(v, w, Mul);
}

static PyObject *
tclobj_nb_inplace_remainder(PyObject *v, PyObject *w)
{
    return tclobj_nb_inplace_binop(v, w, Remainder);
}

static PyObject *
tclobj_nb_inplace_and(PyObject *v, PyObject *w)
{
    return tclobj_nb_inplace_binop(v, w, And);
}

static PyObject *
tclobj_nb_inplace_or(PyObject *v, PyObject *w)
{
    return tclobj_nb_inplace_binop(v, w, Or);
}

static PyObject *
tclobj_nb_inplace_xor(PyObject *v, PyObject *w)
{
    return tclobj_nb_inplace_binop(v, w, Xor);
}

static PyObject *
tclobj_nb_inplace_lshift(PyObject *v, PyObject *w)
{
    return tclobj_nb_inplace_binop(v, w, Lshift);
}

static PyObject *
tclobj_nb_inplace_rshift(PyObject *v, PyObject *w)
{
    return tclobj_nb_inplace_binop(v, w, Rshift);
}

static PyObject *
tclobj_nb_inplace_true_divide(PyObject *v, PyObject *w)
{
    return tclobj_nb_inplace_binop(v, w, Truediv);
}

static PyNumberMethods tclobj_as_number = {
    .nb_bool = (inquiry)tclobj_nb_bool,
    .nb_int = tclobj_nb_long,
    .nb_index = tclobj_nb_long,
    .nb_float = tclobj_nb_float,
    .nb_add = tclobj_nb_add,
    .nb_subtract = tclobj_nb_subtract,
    .nb_multiply = tclobj_nb_multiply,
    .nb_true_divide = tclobj_nb_true_divide,
    .nb_floor_divide = tclobj_nb_floor_divide,
    .nb_remainder = tclobj_nb_remainder,
    .nb_divmod = tclobj_nb_divmod,
    .nb_and = tclobj_nb_and,
    .nb_or = tclobj_nb_or,
    .nb_xor = tclobj_nb_xor,
    .nb_lshift = tclobj_nb_lshift,
    .nb_rshift = tclobj_nb_rshift,
    .nb_negative = (unaryfunc)tclobj_nb_negative,
    .nb_positive = (unaryfunc)tclobj_nb_positive,
    .nb_absolute = (unaryfunc)tclobj_nb_absolute,
    .nb_invert = (unaryfunc)tclobj_nb_invert,

    .nb_inplace_add = tclobj_nb_inplace_add,
    .nb_inplace_subtract = tclobj_nb_inplace_subtract,
    .nb_inplace_multiply = tclobj_nb_inplace_multiply,
    .nb_inplace_remainder = tclobj_nb_inplace_remainder,
    .nb_inplace_lshift = tclobj_nb_inplace_lshift,
    .nb_inplace_rshift = tclobj_nb_inplace_rshift,
    .nb_inplace_and = tclobj_nb_inplace_and,
    .nb_inplace_or = tclobj_nb_inplace_or,
    .nb_inplace_xor = tclobj_nb_inplace_xor,
    .nb_inplace_true_divide = tclobj_nb_inplace_true_divide,
};

//
// TohilTclObj_getto - get "to" value, settable attribute for what
//   type to convert tclobjs and tcldicts to
//
static PyObject *
TohilTclObj_getto(TohilTclObj *self, void *closure)
{
    if (self->to == NULL) {
        Py_RETURN_NONE;
    }
    Py_INCREF(self->to);
    return (PyObject *)self->to;
}

//
// TohilTclObj_getto - get "to" value, attribute for what
//   type to convert tclobjs and tcldicts to
//
static int
TohilTclObj_setto(TohilTclObj *self, PyTypeObject *toType, void *closure)
{
    if (!PyType_Check(toType)) {
        return -1;
    }
    PyTypeObject *tmp = self->to;
    self->to = toType;
    Py_INCREF(toType);
    Py_XDECREF(tmp);
    return 0;
}

static PyGetSetDef TohilTclObj_getsetters[] = {
    {"to", (getter)TohilTclObj_getto, (setter)TohilTclObj_setto, "python type to default returns to", NULL},
    {"_refcount", (getter)TohilTclObj_refcount, NULL, "reference count of the embedded tcl object", NULL},
    {"_pyrefcount", (getter)TohilTclObj_pyrefcount, NULL, "python reference count of the tcl object", NULL},
    {"_tcltype", (getter)TohilTclObj_type, NULL, "internal tcl data type of the tcl object", NULL},
    {NULL}};

static PyMappingMethods TohilTclObj_as_mapping = {(lenfunc)TohilTclObj_length, (binaryfunc)TohilTclObj_subscript, NULL};

static PySequenceMethods TohilTclObj_as_sequence = {
    .sq_length = (lenfunc)TohilTclObj_length,
    .sq_concat = (binaryfunc)TohilTclObj_concat,
    // .sq_repeat = (ssizeargfunc)tclobj_repeat,
    .sq_item = (ssizeargfunc)TohilTclObj_item,
    .sq_ass_item = (ssizeobjargproc)TohilTclObj_ass_item,
    // .sq_contains = (objobjproc)list_contains,
    .sq_inplace_concat = (binaryfunc)TohilTclObj_inplace_concat,
    //.sq_inplace_repeat = (ssizeargfunc)list_inplace_repeat,
};

PyDoc_STRVAR(pop__doc__, "Remove and return item at index (default last).\n\nRaises IndexError if list is empty or index is out of range.");

static PyMethodDef TohilTclObj_methods[] = {
    {"__getitem__", (PyCFunction)TohilTclObj_subscript, METH_O | METH_COEXIST, "x.__getitem__(y) <==> x[y]"},
    {"clear", (PyCFunction)TohilTclObj_clear, METH_NOARGS, "empty the tclobj"},
    {"as_dict", (PyCFunction)TohilTclObj_as_dict, METH_NOARGS, "return tclobj as dict"},
    {"as_byte_array", (PyCFunction)TohilTclObj_as_byte_array, METH_NOARGS, "return tclobj as a byte array"},
    {"incr", (PyCFunction)TohilTclObj_incr, METH_VARARGS | METH_KEYWORDS, "increment tclobj as int"},
    {"getvar", (PyCFunction)TohilTclObj_getvar, METH_O, "set tclobj to tcl var or array element"},
    {"setvar", (PyCFunction)TohilTclObj_setvar, METH_O, "set tcl var or array element to tclobj's tcl object"},
    {"set", (PyCFunction)TohilTclObj_set, METH_O, "set tclobj from some python object"},
    {"lindex", (PyCFunction)TohilTclObj_lindex, METH_VARARGS | METH_KEYWORDS, "get value from tclobj as tcl list"},
    {"append", (PyCFunction)TohilTclObj_lappend, METH_O, "lappend (list-append) something to tclobj"},
    {"extend", (PyCFunction)TohilTclObj_lappend_list, METH_O, "lappend another tclobj or a python list of stuff to tclobj"},
    {"pop", (PyCFunction)TohilTclObj_pop, METH_VARARGS | METH_KEYWORDS, pop__doc__},
    {"insert", (PyCFunction)TohilTclObj_insert, METH_VARARGS | METH_KEYWORDS, "Insert object before index."},
    {NULL} // sentinel
};

static PyTypeObject TohilTclObjType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "tohil.tclobj",
    .tp_doc = "Tcl Object",
    .tp_basicsize = sizeof(TohilTclObj),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = TohilTclObj_new,
    .tp_init = (initproc)TohilTclObj_init,
    .tp_dealloc = (destructor)TohilTclObj_dealloc,
    .tp_methods = TohilTclObj_methods,
    .tp_str = (reprfunc)TohilTclObj_str,
    .tp_iter = (getiterfunc)TohilTclObjIter,
    .tp_as_sequence = &TohilTclObj_as_sequence,
    .tp_as_mapping = &TohilTclObj_as_mapping,
    .tp_repr = (reprfunc)TohilTclObj_repr,
    .tp_richcompare = (richcmpfunc)TohilTclObj_richcompare,
    .tp_getset = TohilTclObj_getsetters,
    .tp_as_number = &tclobj_as_number,
};

//
// end of tclobj python datatype
//

//
// start of tcldict python datatype
//

// the object's python data structure is the same as tclobj
// so we can reuse that to the max.  we're going to have different
// iterators and stuff for dicts

//
// return true if python object is a tclobj type
//
int
TohilTclDict_Check(PyObject *pyObj)
{
    return PyObject_TypeCheck(pyObj, &TohilTclDictType);
}

//
// create a new python tcldict object from any Tcl_Obj
//
static PyObject *
TohilTclDict_FromTclObj(Tcl_Interp *interp, Tcl_Obj *obj)
{
    TohilTclObj *self = (TohilTclObj *)TohilTclDictType.tp_alloc(&TohilTclDictType, 0);
    if (self != NULL) {
        self->interp = interp;
        self->tclobj = obj;
        Tcl_IncrRefCount(obj);
        self->tclvar = NULL;
    }
    return (PyObject *)self;
}

//
// td - tcl dict stuff
//

//
// td_locate(key) - do a dict get on the tclobj object using
//   either a list of keys or a singleton key, and return
//   the target value object.
//
//   internal routine used by td_get and td_exists
//
static Tcl_Obj *
TohilTclDict_td_locate(TohilTclObj *self, PyObject *keys)
{
    // printf("TohilTclDict_td_locate\n");
    Tcl_Obj *keyObj = NULL;
    Tcl_Obj *valueObj = NULL;

    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;

    if (PyList_Check(keys)) {
        int i;
        Tcl_Obj *dictPtrObj = selfobj;
        Py_ssize_t nKeys = PyList_GET_SIZE(keys);

        for (i = 0; i < nKeys; i++) {
            PyObject *keyPyObj = PyList_GET_ITEM(keys, i);
            keyObj = pyObjToTcl(self->interp, keyPyObj);

            if (Tcl_DictObjGet(self->interp, dictPtrObj, keyObj, &valueObj) == TCL_ERROR) {
                Tcl_DecrRefCount(keyObj);
                PyErr_SetString(PyExc_TypeError, "tclobj contents cannot be converted into a td");
                return NULL;
            }
            if (valueObj == NULL) {
                Tcl_DecrRefCount(keyObj);
                return NULL;
            }
            dictPtrObj = valueObj;
            Tcl_DecrRefCount(keyObj);
        }

        // at this point if there's been no error valueObj has our guy
    } else {
        // it's a singleton
        Tcl_Obj *keyObj = pyObjToTcl(self->interp, keys);

        if (Tcl_DictObjGet(NULL, selfobj, keyObj, &valueObj) == TCL_ERROR) {
            Tcl_DecrRefCount(keyObj);
            PyErr_SetString(PyExc_TypeError, "tclobj contents cannot be converted into a td");
            return NULL;
        }
        Tcl_DecrRefCount(keyObj);
    }

    return valueObj;
}

//
// td_get(key) - do a dict get on the tcl object
//
static PyObject *
TohilTclDict_td_get(TohilTclObj *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"key", "to", "default", NULL};
    PyObject *keys = NULL;
    PyTypeObject *to = NULL;
    PyObject *pDefault = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|$OO", kwlist, &keys, &to, &pDefault)) {
        return NULL;
    }

    Tcl_Obj *valueObj = TohilTclDict_td_locate(self, keys);
    if (valueObj == NULL) {
        if (pDefault != NULL) {
            if (to == NULL) {
                // not there but they provided a default,
                // give them their default
                Py_INCREF(pDefault);
                return pDefault;
            } else {
                valueObj = pyObjToTcl(self->interp, pDefault);
            }
        } else {
            // not there, no default.  it's an error.
            // this is clean and the way python does it.
            PyErr_SetObject(PyExc_KeyError, keys);
            return NULL;
        }
    }

    if (to == NULL && self->to != NULL)
        to = self->to;

    return tohil_python_return(self->interp, TCL_OK, to, valueObj);
}

//
// TohilTclDict_subscript - return the value of an element in
//   the tcl dict.  value may itself be a sub-dictionary.
//
//   key can be a list of keys in which case the tcl dict is
//   treated as a hierarchy of dicts.
//
static PyObject *
TohilTclDict_subscript(TohilTclObj *self, PyObject *keys)
{
    // printf("TohilTclDict_subscript\n");
    Tcl_Obj *valueObj = TohilTclDict_td_locate(self, keys);
    if (valueObj == NULL) {
        // not there, no default.  it's an error.
        // this is clean and the way python does it.
        PyErr_SetObject(PyExc_KeyError, keys);
        return NULL;
    }

    return tohil_python_return(self->interp, TCL_OK, self->to, valueObj);
}

//
// TohilTclDict_delitem - removes an item from the tcldict.  if
//   keys is a python list, treats tcl dict as a hierarchy of
//   tcl dicts.
//
static int
TohilTclDict_delitem(TohilTclObj *self, PyObject *keys)
{
    Tcl_Obj *writeObj = NULL;
    if (PyList_Check(keys)) {
        int objc = 0;
        Tcl_Obj **objv = NULL;

        // build up a tcl objv of the keys
        pyListToTclObjv(self->interp, (PyListObject *)keys, &objc, &objv);

        // we are about to try to modify the object, so if it's shared we need to copy
        writeObj = TohilTclObj_writable_objptr(self);
        if (writeObj == NULL)
            return -1;

        int status = (Tcl_DictObjRemoveKeyList(self->interp, writeObj, objc, objv));

        // tear down the objv of the keys we created
        pyListToObjv_teardown(objc, objv);

        if (status == TCL_ERROR) {
            PyErr_SetString(PyExc_KeyError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
            return -1;
        }
    } else {
        Tcl_Obj *keyObj = _pyObjToTcl(self->interp, keys);

        if (keyObj == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "unable to fashion argument into a string to be used as a dictionary key");
            return -1;
        }

        // we are about to try to modify the object, so if it's shared we need to copy
        writeObj = TohilTclObj_writable_objptr(self);
        if (writeObj == NULL)
            return -1;

        if (Tcl_DictObjRemove(NULL, writeObj, keyObj) == TCL_ERROR) {
            Tcl_DecrRefCount(keyObj);
            PyErr_SetString(PyExc_TypeError, "tclobj contents cannot be converted into a td");
            return -1;
        }
        Tcl_DecrRefCount(keyObj);
    }

    if (TohilTclObj_possibly_stuff_var(self, writeObj) < 0)
        return -1;
    return 0;
}

//
// self.setitem(keys, value) - translates python value object to
//   tcl and sets it in the tcldict.  if keys is a python list,
//   treats it as a hierarchy of dictionaries.
//
static int
TohilTclDict_setitem(TohilTclObj *self, PyObject *keys, PyObject *pValue)
{
    Tcl_Obj *valueObj = pyObjToTcl(self->interp, pValue);

    // we are about to try to modify the object, so if it's shared we need to copy
    TohilTclObj_dup_if_shared(self);

    Tcl_Obj *writeObj = TohilTclObj_writable_objptr(self);
    if (writeObj == NULL)
        return -1;

    if (PyList_Check(keys)) {
        int objc;
        Tcl_Obj **objv;

        // build up a tcl objv of the keys
        pyListToTclObjv(self->interp, (PyListObject *)keys, &objc, &objv);

        int status = (Tcl_DictObjPutKeyList(self->interp, writeObj, objc, objv, valueObj));

        // tear down the objv of the keys we created
        pyListToObjv_teardown(objc, objv);

        if (status == TCL_ERROR) {
            Tcl_DecrRefCount(valueObj);
            PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(self->interp)));
            return -1;
        }
    } else {
        Tcl_Obj *keyObj = _pyObjToTcl(self->interp, keys);

        if (keyObj == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "unable to fashion argument into a string to be used as a dictionary key");
            return -1;
        }

        if (Tcl_DictObjPut(self->interp, writeObj, keyObj, valueObj) == TCL_ERROR) {
            Tcl_DecrRefCount(keyObj);
            Tcl_DecrRefCount(valueObj);
            PyErr_SetString(PyExc_TypeError, "tclobj contents cannot be converted into a td");
            return -1;
        }
        // something about this is a crash
        // Tcl_DecrRefCount(keyObj);
        // Tcl_DecrRefCount(valueObj);
    }

    if (TohilTclObj_possibly_stuff_var(self, writeObj) < 0)
        return -1;
    return 0;
}

//
// tclobj.td_set(key, value) - do a dict set on the tcl object.
//   if key is a python list, td_set operates on a nested hierarchy
//   of dictionaries.
//
static PyObject *
TohilTclDict_td_set(TohilTclObj *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"key", "value", NULL};
    PyObject *keys = NULL;
    PyObject *pValue = NULL;

    // remember, "O" sets our pointer to the object without incrementing its reference count
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|$", kwlist, &keys, &pValue)) {
        return NULL;
    }

    if (TohilTclDict_setitem(self, keys, pValue) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}

//
// TohilTclDict_ass_sub() - if a python key object and python value
//   object are present, set in the tcl dict for the key, the value.
//
//   the key may be a list in which case the dictionary is a hierarchy
//   of dictionaries.
//
//   if the value is NULL then deletes the item.
//
static int
TohilTclDict_ass_sub(TohilTclObj *self, PyObject *key, PyObject *val)
{
    if (val == NULL) {
        return TohilTclDict_delitem(self, key);
    } else {
        return TohilTclDict_setitem(self, key, val);
    }
}

//
// TohilTclDict_length() - return the dict size of a python tcldict's
//   tcl object or set a python error and return -1 if something went wrong.
//
static Py_ssize_t
TohilTclDict_length(TohilTclObj *self)
{
    int length;
    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return -1;

    if (Tcl_DictObjSize(self->interp, selfobj, &length) == TCL_OK) {
        return length;
    }
    PyErr_SetString(PyExc_TypeError, "tclobj contents cannot be converted into a td");
    return -1;
}

//
// TohilTclDict_size() - return a python object containing the dict size
//   of a python tcldict's tcl object.
//   returns null i.e. exception thrown if tcl object isn't a proper tcl dict.
//
static PyObject *
TohilTclDict_size(TohilTclObj *self, PyObject *Py_UNUSED(ignored))
{
    int length = TohilTclDict_length(self);
    if (length < 0) {
        return NULL;
    }
    return PyLong_FromLong(length);
}

//
// TohilTclDictIter_new() - returns a tcldict iterator object that can
//   iterate over a tcldict.
//
static PyObject *
TohilTclDictIter_new(TohilTclObj *self, PyTypeObject *itertype)
{
    Tcl_Obj *selfobj = TohilTclObj_objptr(self);
    if (selfobj == NULL)
        return NULL;

    // printf("TohilTclDictIter\n");
    // we don't need size but we use this to make tclobj is or can be a dict
    int size = 0;
    if (Tcl_DictObjSize(self->interp, selfobj, &size) == TCL_ERROR) {
        PyErr_Format(PyExc_TypeError, "tclobj contents cannot be converted into a td");
        return NULL;
    }

    Tohil_TD_IterObj *pIter = (Tohil_TD_IterObj *)PyObject_New(Tohil_TD_IterObj, itertype);

    pIter->interp = self->interp;
    pIter->started = 0;
    pIter->done = 0;
    pIter->to = self->to;
    Py_XINCREF(pIter->to);

    memset((void *)&pIter->search, 0, sizeof(Tcl_DictSearch));

    pIter->dictObj = selfobj;
    Tcl_IncrRefCount(pIter->dictObj);

    return (PyObject *)pIter;
}

//
// TohilTclDictIter() - returns a tcldict iterator object that can
//   iterate over a tcldict.
//
static PyObject *
TohilTclDictIter(TohilTclObj *self)
{
    return TohilTclDictIter_new(self, &Tohil_TD_IterType);
}

static PyObject *
TohilTclDict_keys_new(TohilTclObj *self, PyObject *Py_UNUSED(ignored))
{
    return TohilTclDictIter_new(self, &Tohil_TD_IterKeysType);
}

static PyObject *
TohilTclDict_values_new(TohilTclObj *self, PyObject *Py_UNUSED(ignored))
{
    return TohilTclDictIter_new(self, &Tohil_TD_IterValuesType);
}

static PyObject *
TohilTclDict_items_new(TohilTclObj *self, PyObject *Py_UNUSED(ignored))
{
    return TohilTclDictIter_new(self, &Tohil_TD_IterItemsType);
}

//
// TohilTclDict_Contains - return 0 if key or keys is not
//   contained in tcldict, 1 if it is, or -1 if there was
//   an error.
//
static int
TohilTclDict_Contains(PyObject *self, PyObject *keys)
{
    // printf("TohilTclDict_Contains\n");
    Tcl_Obj *valueObj = TohilTclDict_td_locate((TohilTclObj *)self, keys);
    if (valueObj == NULL) {
        if (PyErr_Occurred() != NULL) {
            return -1;
        }
        return 0;
    }
    return 1;
}

static PyMappingMethods TohilTclDict_as_mapping = {(lenfunc)TohilTclDict_length, (binaryfunc)TohilTclDict_subscript,
                                                   (objobjargproc)TohilTclDict_ass_sub};

static PySequenceMethods TohilTclDict_as_sequence = {
    .sq_contains = TohilTclDict_Contains,
};

PyDoc_STRVAR(keys__doc__, "D.keys() -> a set-like object providing a view on D's keys");
PyDoc_STRVAR(items__doc__, "D.items() -> a set-like object providing a view on D's items");
PyDoc_STRVAR(values__doc__, "D.values() -> an object providing a view on D's values");

static PyMethodDef TohilTclDict_methods[] = {
    {"__getitem__", (PyCFunction)(void (*)(void))TohilTclDict_subscript, METH_O | METH_COEXIST, "x.__getitem__(y) <==> x[y]"},
    {"get", (PyCFunction)TohilTclDict_td_get, METH_VARARGS | METH_KEYWORDS, "get from tcl dict"},
    // NB i don't know if this __len__ thing works -- python might
    // be doing something gross to get the len of the dict, like
    // enumerating the elements
    {"__len__", (PyCFunction)TohilTclDict_size, METH_VARARGS | METH_KEYWORDS, "get length of tcl dict"},
    {"keys", (PyCFunction)TohilTclDict_keys_new, METH_NOARGS, keys__doc__},
    {"items", (PyCFunction)TohilTclDict_items_new, METH_NOARGS, items__doc__},
    {"values", (PyCFunction)TohilTclDict_values_new, METH_NOARGS, values__doc__},
    {"td_set", (PyCFunction)TohilTclDict_td_set, METH_VARARGS | METH_KEYWORDS, "set item in tcl dict"},
    {"getvar", (PyCFunction)TohilTclObj_getvar, METH_O, "set tclobj to tcl var or array element"},
    {"setvar", (PyCFunction)TohilTclObj_setvar, METH_O, "set tcl var or array element to tclobj's tcl object"},
    {"set", (PyCFunction)TohilTclObj_set, METH_O, "set tclobj from some python object"},
    {"clear", (PyCFunction)TohilTclObj_clear, METH_NOARGS, "remove all items from the tcl dict"},
    {NULL} // sentinel
};

// NB need to change tp_itr, to_as_squence, to_as_mapping to do dict stuff

static PyTypeObject TohilTclDictType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        // .tp_base = &TohilTclObjType, NB - len() breaks when we inherit
        .tp_name = "tohil.tcldict",
    .tp_doc = "Tcl TD tcldict Object",
    .tp_basicsize = sizeof(TohilTclObj),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = TohilTclObj_new,
    .tp_init = (initproc)TohilTclObj_init,
    .tp_dealloc = (destructor)TohilTclObj_dealloc,
    .tp_methods = TohilTclDict_methods,
    .tp_str = (reprfunc)TohilTclObj_str,
    .tp_iter = (getiterfunc)TohilTclDictIter,
    .tp_as_mapping = &TohilTclDict_as_mapping,
    .tp_as_sequence = &TohilTclDict_as_sequence,
    .tp_repr = (reprfunc)TohilTclObj_repr,
    .tp_richcompare = (richcmpfunc)TohilTclObj_richcompare,
    .tp_getset = TohilTclObj_getsetters,
};

//
//
// end of tcldict python datatype
//
//

// tohil_python_return - you call this routine when you have a tcl object
//   that you want to turn into a python object.  usually you call it when
//   you are returning from a C function called from python, but it is
//   used repeatedly in TohilTclObj_slice to prepare a list of python objects
//   converted from tcl objects.
//
//   this code is called  a lot.  it also can return tcl errors, and it
//   can convert the tcl object to a specific python type if the "to" type
//   is not null.  (If null the conversion type defaults to str.)
//
// NB these strcmps could be replaced by more efficient direct
// address comparisons if you grabbed the addresses of the type objects
// we are insterested in compared to them
//
static PyObject *
tohil_python_return(Tcl_Interp *interp, int tcl_result, PyTypeObject *toType, Tcl_Obj *resultObj)
{
    const char *toString = NULL;
    PyTypeObject *pt = NULL;

    if (PyErr_Occurred() != NULL) {
        // printf("tohil_python_return invoked with a python error already present\n");
        // return NULL;
    }

    if (tcl_result == TCL_ERROR) {
        // dig out tcl error information and create a tohil tcldict containing it
        // (Tcl_GetReturnOptions returns a tcl dict object)
        Tcl_Obj *returnOptionsObj = Tcl_GetReturnOptions(interp, tcl_result);
        PyObject *pReturnOptionsObj = TohilTclDict_FromTclObj(interp, returnOptionsObj);

        // construct a two-element tuple comprising the interpreter result
        // and the tcldict containing the info grabbed from tcl
        PyObject *pRetTuple = PyTuple_New(2);
        int tclStringSize;
        char *tclString;
        tclString = Tcl_GetStringFromObj(resultObj, &tclStringSize);
        PyTuple_SET_ITEM(pRetTuple, 0, Py_BuildValue("s#", tclString, tclStringSize));
        PyTuple_SET_ITEM(pRetTuple, 1, pReturnOptionsObj);

        // borrowed ref, do not decrement
        PyObject *m = PyImport_AddModule("tohil");
        assert(m != NULL);
        PyObject *error_class = PyObject_GetAttrString(m, "TclError");
        if (error_class == NULL || !PyCallable_Check(error_class)) {
            Py_XDECREF(error_class);
            Py_DECREF(m);
            return NULL;
        }

        // ...and set the python error object to
        // TclError(interp_result_string, tcldict_object)
        PyErr_SetObject(error_class, pRetTuple);
        return NULL;
    }

    if (toType != NULL) {
        if (!PyType_Check(toType)) {
            PyErr_SetString(PyExc_TypeError, "to type is not a valid python data type");
            return NULL;
        }

        // toType/pt is a borrowed reference; do not decrement its reference count
        pt = (PyTypeObject *)toType;
        toString = pt->tp_name;
    }
    // printf("tohil_python_return called: tcl result %d, to=%s, resulObj '%s'\n", tcl_result, toString, Tcl_GetString(resultObj));

    if (toType == NULL || STREQU(toString, "str")) {
        int tclStringSize;
        char *tclString;
        int utf8len;
        char *utf8string;

        tclString = Tcl_GetStringFromObj(resultObj, &tclStringSize);
        if (tohil_TclToUTF8(interp, tclString, tclStringSize, &utf8string, &utf8len) != TCL_OK) {
            PyErr_SetString(PyExc_RuntimeError, Tcl_GetString(Tcl_GetObjResult(interp)));
            return NULL;
        }
        PyObject *pObj = Py_BuildValue("s#", utf8string, utf8len);
        ckfree(utf8string);
        return pObj;
    }

    if (STREQU(toString, "int")) {
        long longValue;

        if (Tcl_GetLongFromObj(interp, resultObj, &longValue) == TCL_OK) {
            return PyLong_FromLong(longValue);
        }
        PyErr_SetString(PyExc_ValueError, Tcl_GetString(Tcl_GetObjResult(interp)));
        return NULL;
    }

    if (STREQU(toString, "bool")) {
        int boolValue;

        if (Tcl_GetBooleanFromObj(interp, resultObj, &boolValue) == TCL_OK) {
            PyObject *p = (boolValue ? Py_True : Py_False);
            Py_INCREF(p);
            return p;
        }
        PyErr_SetString(PyExc_ValueError, Tcl_GetString(Tcl_GetObjResult(interp)));
        return NULL;
    }

    if (STREQU(toString, "float")) {
        double doubleValue;

        if (Tcl_GetDoubleFromObj(interp, resultObj, &doubleValue) == TCL_OK) {
            return PyFloat_FromDouble(doubleValue);
        }
        PyErr_SetString(PyExc_ValueError, Tcl_GetString(Tcl_GetObjResult(interp)));
        return NULL;
    }

    if (STREQU(toString, "tohil.tclobj")) {
        return TohilTclObj_FromTclObj(interp, resultObj);
    }

    if (STREQU(toString, "tohil.tcldict")) {
        return TohilTclDict_FromTclObj(interp, resultObj);
    }

    if (STREQU(toString, "list")) {
        return tclListObjToPyListObject(interp, resultObj);
    }

    if (STREQU(toString, "set")) {
        return tclListObjToPySetObject(interp, resultObj);
    }

    if (STREQU(toString, "dict")) {
        return tclListObjToPyDictObject(interp, resultObj);
    }

    if (STREQU(toString, "tuple")) {
        return tclListObjToPyTupleObject(interp, resultObj);
    }

    PyErr_SetString(PyExc_TypeError, "'to' conversion type must be str, int, bool, float, list, set, dict, tuple, tohil.tclobj or tohil.tcldict");
    return NULL;
}

//
// tohil.eval command for python to eval code in the tcl interpreter
//
static PyObject *
tohil_eval(PyObject *m, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"tcl_code", "to", NULL};
    PyTypeObject *to = NULL;
    char *utf8Code = NULL;
    Tcl_Interp *interp = tohilstate(m)->interp;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|$O", kwlist, &utf8Code, &to))
        return NULL;

    // TODO modify the above PyArg_ParseTupleAndKeywords to return a length?
    Tcl_DString ds;
    char *tclCode = tohil_UTF8ToTclDString(interp, utf8Code, -1, &ds);
    int result = Tcl_Eval(interp, tclCode);
    Tcl_DStringFree(&ds);
    Tcl_Obj *resultObj = Tcl_GetObjResult(interp);

    return tohil_python_return(interp, result, to, resultObj);
}

//
// tohil.expr command for python to evaluate expressions using the tcl interpreter
//
static PyObject *
tohil_expr(PyObject *m, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"expression", "to", NULL};
    char *utf8expression = NULL;
    PyTypeObject *to = NULL;
    Tcl_Interp *interp = tohilstate(m)->interp;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|$O", kwlist, &utf8expression, &to))
        return NULL;

    Tcl_DString ds;
    char *expression = tohil_UTF8ToTclDString(interp, utf8expression, -1, &ds);

    Tcl_Obj *resultObj = NULL;
    if (Tcl_ExprObj(interp, Tcl_NewStringObj(expression, -1), &resultObj) == TCL_ERROR) {
        char *errMsg = Tcl_GetString(Tcl_GetObjResult(interp));
        PyErr_SetString(PyExc_RuntimeError, errMsg);
        return NULL;
    }

    return tohil_python_return(interp, TCL_OK, to, resultObj);
}

//
// tohil.convert command for python to pass a python object through
// to a tcl object and then convert it to a to= destination type
//
static PyObject *
tohil_convert(PyObject *m, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"pyobject", "to", NULL};
    PyObject *pyInputObject = NULL;
    PyTypeObject *to = NULL;
    Tcl_Interp *interp = tohilstate(m)->interp;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|$O", kwlist, &pyInputObject, &to))
        return NULL;

    Tcl_Obj *interimObj = pyObjToTcl(interp, pyInputObject);
    if (interimObj == NULL) {
        return NULL;
    }

    return tohil_python_return(interp, TCL_OK, to, interimObj);
}

//
// tohil.getvar - from python get the contents of a variable
//
static PyObject *
tohil_getvar(PyObject *m, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"var", "to", "default", NULL};
    char *var = NULL;
    PyTypeObject *to = NULL;
    PyObject *defaultPyObj = NULL;
    Tcl_Obj *obj = NULL;
    Tcl_Interp *interp = tohilstate(m)->interp;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|$OO", kwlist, &var, &to, &defaultPyObj)) {
        return NULL;
    }

    if (defaultPyObj == NULL) {
        // a default wasn't specified, it's an error if the var or array
        // element doesn't exist
        obj = Tcl_GetVar2Ex(interp, var, NULL, (TCL_LEAVE_ERR_MSG));

        if (obj == NULL) {
            PyErr_SetString(PyExc_NameError, Tcl_GetString(Tcl_GetObjResult(interp)));
            return NULL;
        }
    } else {
        // a default was specified, it's not an error if the var or array
        // element doesn't exist, we simply return the default value
        obj = Tcl_GetVar2Ex(interp, var, NULL, 0);
        if (obj == NULL) {
            // not there but they provided a default
            if (to == NULL) {
                // they didn't provide a to= conversion,
                // give them their default as is
                Py_INCREF(defaultPyObj);
                return defaultPyObj;
            } else {
                // they provided a to= conversion, run
                // their python through that and return it.
                obj = pyObjToTcl(interp, defaultPyObj);
            }
        }
    }

    // the var or array element exists in tcl, return the value to python,
    // possibly to a specific datatype
    return tohil_python_return(interp, TCL_OK, to, obj);
}

//
// tohil.exists - from python see if a variable or array element exists in tcl
//
static PyObject *
tohil_exists(PyObject *m, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"var", NULL};
    char *var = NULL;
    Tcl_Interp *interp = tohilstate(m)->interp;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|$", kwlist, &var))
        return NULL;

    Tcl_Obj *obj = Tcl_GetVar2Ex(interp, var, NULL, 0);

    PyObject *p = (obj == NULL ? Py_False : Py_True);
    Py_INCREF(p);
    return p;
}

//
// tohil.setvar - set a variable or array element in tcl from python
//
static PyObject *
tohil_setvar(PyObject *m, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"var", "value", NULL};
    char *var = NULL;
    PyObject *pyValue = NULL;
    Tcl_Interp *interp = tohilstate(m)->interp;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO", kwlist, &var, &pyValue))
        return NULL;

    Tcl_Obj *tclValue = pyObjToTcl(interp, pyValue);

    Tcl_Obj *obj = Tcl_SetVar2Ex(interp, var, NULL, tclValue, (TCL_LEAVE_ERR_MSG));

    if (obj == NULL) {
        char *errMsg = Tcl_GetString(Tcl_GetObjResult(interp));
        PyErr_SetString(PyExc_RuntimeError, errMsg);
        return NULL;
    }
    Py_RETURN_NONE;
}

//
// tohil.incr - incr a variable or array element in tcl from python
//
static PyObject *
tohil_incr(PyObject *m, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"var", "incr", NULL};
    char *var = NULL;
    long longValue = 0;
    long increment = 1;
    Tcl_Interp *interp = tohilstate(m)->interp;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|l", kwlist, &var, &increment))
        return NULL;

    Tcl_Obj *obj = Tcl_GetVar2Ex(interp, var, NULL, 0);
    if (obj == NULL) {
        longValue = increment;
        obj = Tcl_NewLongObj(longValue);
        if (Tcl_SetVar2Ex(interp, var, NULL, obj, (TCL_LEAVE_ERR_MSG)) == NULL) {
            goto type_error;
        }
    } else {
        if (Tcl_GetLongFromObj(interp, obj, &longValue) == TCL_ERROR) {
        type_error:
            PyErr_SetString(PyExc_TypeError, Tcl_GetString(Tcl_GetObjResult(interp)));
            return NULL;
        }

        longValue += increment;

        if (Tcl_IsShared(obj)) {
            Tcl_DecrRefCount(obj);
            obj = Tcl_DuplicateObj(obj);
            Tcl_SetLongObj(obj, longValue);
            if (Tcl_SetVar2Ex(interp, var, NULL, obj, (TCL_LEAVE_ERR_MSG)) == NULL) {
                goto type_error;
            }
        } else {
            Tcl_SetLongObj(obj, longValue);
        }
    }
    return PyLong_FromLong(longValue);
}

// tohil.unset - from python unset a variable or array element in the tcl
//   interpreter.  it is not an error if the variable or element doesn't
//   exist.  if passed the name of an array with no subscripted element,
//   the entire array is deleted
static PyObject *
tohil_unset(PyObject *m, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t objc = PyTuple_GET_SIZE(args);
    int i;
    Tcl_Interp *interp = tohilstate(m)->interp;

    // for each argument convert the python object to a tcl object
    // and store it in the tcl object vector
    for (i = 0; i < objc; i++) {
        char *var = (char *)PyUnicode_1BYTE_DATA(PyTuple_GET_ITEM(args, i));
        Tcl_UnsetVar(interp, var, 0);
    }

    Py_RETURN_NONE;
}

//
// tohil.subst - perform tcl "subst" substitution on the passed string,
// evaluating square-bracketed stuff and expanding $-prefaced variables,
// without evaluating the ultimate result, like eval would
//
static PyObject *
tohil_subst(PyObject *m, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"string", "to", NULL};
    char *string = NULL;
    PyTypeObject *to = NULL;
    Tcl_Interp *interp = tohilstate(m)->interp;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|$O", kwlist, &string, &to)) {
        return NULL;
    }
    Tcl_Obj *obj = Tcl_SubstObj(interp, Tcl_NewStringObj(string, -1), TCL_SUBST_ALL);
    if (obj == NULL) {
        char *errMsg = Tcl_GetString(Tcl_GetObjResult(interp));
        PyErr_SetString(PyExc_RuntimeError, errMsg);
        return NULL;
    }

    return tohil_python_return(interp, TCL_OK, to, obj);
}

//
// tohil.call function for python that's like the tohil::call in tcl, something
// that lets you explicitly specify a tcl command and its arguments and lets
// you avoid passing everything through eval.  here it is.
//
static PyObject *
tohil_call(PyObject *m, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t objc = PyTuple_GET_SIZE(args);
    int i;
    PyTypeObject *to = NULL;
    Tcl_Interp *interp = tohilstate(m)->interp;

    //
    // allocate an array of Tcl object pointers the same size
    // as the number of arguments we received
    Tcl_Obj **objv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * objc);

    // PyObject_Print(kwargs, stdout, 0);

    // we need to process kwargs to get the to
    if (kwargs != NULL) {
        to = (PyTypeObject *)PyDict_GetItemString(kwargs, "to");
    }

    // for each argument convert the python object to a tcl object
    // and store it in the tcl object vector
    for (i = 0; i < objc; i++) {
        objv[i] = pyObjToTcl(interp, PyTuple_GET_ITEM(args, i));
        Tcl_IncrRefCount(objv[i]);
    }

    // invoke tcl using the objv array we just constructed
    int tcl_result = Tcl_EvalObjv(interp, objc, objv, 0);

    // cleanup and free the objv
    for (i = 0; i < objc; i++) {
        Tcl_DecrRefCount(objv[i]);
    }
    ckfree(objv);

    return tohil_python_return(interp, tcl_result, to, Tcl_GetObjResult(interp));
}

//
// python C extension structure defining functions
//
// these are the tohil.* ones like tohil.eval, tohil.call, etc
//
static PyMethodDef TohilMethods[] = {
    {"eval", (PyCFunction)tohil_eval, METH_VARARGS | METH_KEYWORDS, "Evaluate tcl code"},
    {"getvar", (PyCFunction)tohil_getvar, METH_VARARGS | METH_KEYWORDS, "get vars and array elements from the tcl interpreter"},
    {"setvar", (PyCFunction)tohil_setvar, METH_VARARGS | METH_KEYWORDS, "set vars and array elements in the tcl interpreter"},
    {"exists", (PyCFunction)tohil_exists, METH_VARARGS | METH_KEYWORDS, "check whether vars and array elements exist in the tcl interpreter"},
    {"unset", (PyCFunction)tohil_unset, METH_VARARGS, "unset variables, array elements, or arrays from the tcl interpreter"},
    {"incr", (PyCFunction)tohil_incr, METH_VARARGS | METH_KEYWORDS, "increment vars and array elements in the tcl interpreter"},
    {"subst", (PyCFunction)tohil_subst, METH_VARARGS | METH_KEYWORDS, "perform Tcl command, variable and backslash substitutions on a string"},
    {"expr", (PyCFunction)tohil_expr, METH_VARARGS | METH_KEYWORDS, "evaluate Tcl expression"},
    {"convert", (PyCFunction)tohil_convert, METH_VARARGS | METH_KEYWORDS,
     "convert python to tcl object then to whatever to= says or string and return"},
    {"call", (PyCFunction)tohil_call, METH_VARARGS | METH_KEYWORDS, "invoke a tcl command with arguments"},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static struct PyModuleDef_Slot tohil_slots[] = {
    {Py_mod_exec, tohil_mod_exec},
    {0, NULL},
};

// TODO: there should probably be some tcl deinit in the clear/free code
static struct PyModuleDef TohilModule = {
    PyModuleDef_HEAD_INIT,
    // this comment helps clang-format
    .m_name = "tohil",
    .m_doc = "A module to permit interop with Tcl",
    .m_size = sizeof(TohilModuleState),
    .m_methods = TohilMethods,
    .m_slots = tohil_slots,
};

/* Shared initialisation begins here */

//
// this is the entry point called when the tcl interpreter loads the tohil shared library
//
// tcl will only call it once per interpreter
//
// NB BUG after package forget, package require doesn't work
//
int
Tohil_Init(Tcl_Interp *interp)
{
    // printf("Tohil_Init\n");

    if (Tcl_InitStubs(interp, "8.6", 0) == NULL)
        return TCL_ERROR;

    if (Tcl_PkgRequire(interp, "Tcl", "8.6", 0) == NULL)
        return TCL_ERROR;

    if (Tcl_PkgProvide(interp, "tohil", PACKAGE_VERSION) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_CreateNamespace(interp, "::tohil", NULL, NULL) == NULL)
        return TCL_ERROR;

    // if i haven't been told python is up, tcl is the parent,
    // and we need to initialize the python interpreter and
    // our python module
    if (!Py_IsInitialized()) {
        // figure out argv0; it will help the python interpreter hopefully find
        // a path to the right python run-time libraries.
        const char *argv0 = Tcl_GetVar(interp, "::argv0", 0);
        if (argv0 != NULL) {
            wchar_t *wide_argv0 = Py_DecodeLocale(argv0, NULL);
            if (wide_argv0 != NULL) {
                Py_SetProgramName(wide_argv0);
            }
        }

        // NB without this ugly hack then on linux if tcl starts
        // python then python gets errors loading C shared libraries
        // such as "import sqlite3", where loading the shared library
        // causes a complaint about undefined symbols trying to access
        // python stuff
        char *python_lib = "libpython" PYTHON_VERSION ".so";
        if (dlopen(python_lib, RTLD_GLOBAL | RTLD_LAZY) == NULL) {
            // fprintf(stderr, "load %s failed\n", python_lib);
        }

        // initialize python but since tcl is the parent,
        // pass 0 for initsigs, so python will not register
        // signal handlers
        Py_InitializeEx(0);

        // printf("Tohil_Init: initialized python from scratch, setting subinterp to main thread state\n");
        tohil_setup_subinterp(interp, TclParent);
    } else {
        if (!tohil_subinterp_is_set(interp)) {
            // printf("Tohil_Init: python is there already and tcl interpreter %p doesn't have a subinterp set, making a subinterpreter\n", interp);
            tohil_setup_subinterp(interp, TclChild);
        }
    }

    // stash the Tcl interpreter pointer so the python side can find it later
    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *pCap = PyCapsule_New(interp, "tohil.interp", NULL);
    if (PyObject_SetAttrString(main_module, "interp", pCap) == -1) {
        return TCL_ERROR;
    }
    Py_DECREF(pCap);

    // import tohil module
    PyObject *pTohilModStr = PyUnicode_FromString("tohil");
    PyObject *m = PyImport_Import(pTohilModStr);
    Py_DECREF(pTohilModStr);

    if (m == NULL) {
        // NB debug break out the exception
        PyObject *pType = NULL, *pVal = NULL, *pTrace = NULL;
        PyErr_Fetch(&pType, &pVal, &pTrace); /* Clears exception */
        PyErr_NormalizeException(&pType, &pVal, &pTrace);
        // PyObject_Print(pType, stdout, 0);
        // PyObject_Print(pVal, stdout, 0);

        return Tohil_ReturnTclError(interp, "unable to import tohil module to python interpreter");
    }
    // printf("Tohil_Init: imported tohil module\n");

    // attach tohil module to __main__
    const char *tohil_modname = PyModule_GetName(m);
    if (tohil_modname != NULL) {
        int ret = PyObject_SetAttrString(main_module, tohil_modname, m);
        if (ret < 0)
            return Tohil_ReturnTclError(interp, "unable to setattr tohil module to __main__");
    }

    // define tohil C commands that extend the Tcl interpreter
    if (Tcl_CreateObjCommand(interp, "::tohil::eval", (Tcl_ObjCmdProc *)TohilEval_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL) == NULL)
        return TCL_ERROR;

    if (Tcl_CreateObjCommand(interp, "::tohil::exec", (Tcl_ObjCmdProc *) TohilExec_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL) == NULL)
        return TCL_ERROR;

    if (Tcl_CreateObjCommand(interp, "::tohil::call", (Tcl_ObjCmdProc *)TohilCall_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL) == NULL)
        return TCL_ERROR;

    if (Tcl_CreateObjCommand(interp, "::tohil::import", (Tcl_ObjCmdProc *)TohilImport_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL) == NULL)
        return TCL_ERROR;

    if (Tcl_CreateObjCommand(interp, "::tohil::interact", (Tcl_ObjCmdProc *)TohilInteract_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL) == NULL)
        return TCL_ERROR;

    return TCL_OK;
}

//
// populate the tohil module (multiphase-init)
//
// PyInit__tohil creates the python module.  once a python interpreter
// or subinterpreter imports it, this gets called to populate it.
//
static int
tohil_mod_exec(PyObject *m)
{
    Tcl_Interp *interp = NULL;
    PyObject *pTohilModStr, *pTohilMod;

    // if Tcl is the parent, it will have stashed an opaque
    // pointer to the Tcl interpreter in __main__'s dictionary.
    //
    // if there isn't such a pointer there, Python is the parent,
    // and we'll need to create initialize the Tcl interpreter.
    //
    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *pCap = PyObject_GetAttrString(main_module, "interp");
    if (pCap == NULL) {
        // stashed attribute doesn't exist.
        // tcl interp hasn't been set up.
        // python is the parent.
        // create and initialize the tcl interpreter.
        PyErr_Clear();
        interp = Tcl_CreateInterp();

        if (Tcl_Init(interp) != TCL_OK)
            goto fail;

        // since python is the parent, the python subinterpreter
        // isn't really a subinterpreter
        // printf("tohil_mod_exec: i created tcl interp %p, setting subinterp to main thread i hope %p\n", interp, PyThreadState_Get());
        tohil_setup_subinterp(interp, PythonParent);

        // invoke Tohil_Init to load us into the tcl interpreter
        // NB uh this probably isn't enough and we need to do a
        // package require tohil as there is tcl code in files in
        // the package now
        // OTOH you know you've got the right shared library
        if (Tohil_Init(interp) == TCL_ERROR)
            goto fail;
    } else {
        // python interpreter-containing attribute exists, get the interpreter
        interp = PyCapsule_GetPointer(pCap, "tohil.interp");
        Py_DECREF(pCap);
    }
    tohilstate(m)->interp = interp;

    // set the near-standard dunder version for our module (tohil._tohil)
    // to the package version passed to the compiler command line by
    // the build tools
    if (PyModule_AddStringConstant(m, "__version__", PACKAGE_VERSION) < 0)
        goto fail;

    // add our tclobj type to python
    Py_INCREF(&TohilTclObjType);
    if (PyModule_AddObject(m, "tclobj", (PyObject *)&TohilTclObjType) < 0) {
        Py_DECREF(&TohilTclObjType);
        goto fail;
    }

    // add our tcldict type to python
    Py_INCREF(&TohilTclDictType);
    if (PyModule_AddObject(m, "tcldict", (PyObject *)&TohilTclDictType) < 0) {
        Py_DECREF(&TohilTclDictType);
        goto fail;
    }

    // ..and stash a pointer to the tcl interpreter in a python
    // capsule so we can find it when we're doing python stuff
    // and need to talk to tcl
    pCap = PyCapsule_New(interp, "tohil.interp", NULL);
    if (PyObject_SetAttrString(m, "interp", pCap) == -1) {
        Py_DECREF(pCap);
        goto fail;
    }
    // stash capsule in the tclobj datatype's dictionary
    if (PyDict_SetItemString(TohilTclObjType.tp_dict, "_interp", pCap) == -1) {
        Py_DECREF(pCap);
        goto fail;
    }
    // stash capsule in the tcldict datatype's dictionary
    if (PyDict_SetItemString(TohilTclDictType.tp_dict, "_interp", pCap) == -1) {
        Py_DECREF(pCap);
        goto fail;
    }
    Py_DECREF(pCap);
    // tohil_dict_dump(TohilTclObjType.tp_dict);

    // import tohil to get at the python parts
    pTohilModStr = PyUnicode_FromString("tohil");
    pTohilMod = PyImport_Import(pTohilModStr);
    Py_DECREF(pTohilModStr);
    if (pTohilMod == NULL) {
        goto fail;
    }

    return 0;

fail:
    Py_XDECREF(m);
    return -1;
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
    // turn up the tclobj python type
    if (PyType_Ready(&TohilTclObjType) < 0)
        return NULL;

    // turn up the tcldict iterator type
    if (PyType_Ready(&TohilTclObj_IterType) < 0)
        return NULL;

    // turn up the tcldict iterator type
    if (PyType_Ready(&Tohil_TD_IterType) < 0)
        return NULL;

    // turn up the tcldict keys, items and values iterator types
    if (PyType_Ready(&Tohil_TD_IterKeysType) < 0)
        return NULL;

    if (PyType_Ready(&Tohil_TD_IterItemsType) < 0)
        return NULL;

    if (PyType_Ready(&Tohil_TD_IterValuesType) < 0)
        return NULL;

    if (PyType_Ready(&TohilTclDictType) < 0)
        return NULL;

    // create the python module
    PyObject *m = PyModuleDef_Init(&TohilModule);
    if (m == NULL)
        return NULL;

    return m;
}

// vim: set ts=4 sw=4 sts=4 et :
