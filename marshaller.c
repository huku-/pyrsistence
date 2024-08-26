/* pyrsistence - A Python extension for External Memory Data Structures (EMDs)
 * huku <huku@grhack.net>
 *
 * marshaller.c - Python object marshalling and unmarshalling API.
 *
 * This file exports two API functions, namely `marshal()' and `unmarshal()',
 * which can be used to serialize and deserialize python objects to and from
 * string objects accordingly. The default behavior is to use "cPickle" on
 * Python 2.x and "_pickle" on Python 3.x.
 *
 * However, the API allows for EM types to implement their own serialization and
 * deserialization methods (see `em_common_t' in "common.h").
 */
#include <Python.h>

#include "marshaller.h"


static PyObject *module;
static PyObject *marshal_method;
static PyObject *unmarshal_method;
static PyObject *proto;


int marshaller_init(void)
{
    PyObject *dict;

    int ret = -1;

#if PY_MAJOR_VERSION >= 3
    if((module = PyImport_ImportModule("_pickle")) == NULL)
        goto _err;
#else
    if((module = PyImport_ImportModule("cPickle")) == NULL)
        goto _err;
#endif

    dict = PyModule_GetDict(module);

    if((marshal_method = PyDict_GetItemString(dict, "dumps")) == NULL)
    {
        Py_DECREF(module);
        goto _err;
    }

    if((unmarshal_method = PyDict_GetItemString(dict, "loads")) == NULL)
    {
        Py_DECREF(module);
        Py_DECREF(marshal_method);
        goto _err;
    }

    /* According to Python 2.7.x & 3.x documentation, specifying a negative
     * protocol version is equivalent to choosing the highest protocol.
     */
    if((proto = PyLong_FromLong(-1)) == NULL)
    {
        Py_DECREF(module);
        Py_DECREF(marshal_method);
        Py_DECREF(unmarshal_method);
        goto _err;
    }

    ret = 0;

_err:
    return ret;
}


/* Marshal object `obj' and return a string object. */
PyObject *marshal(em_common_t *em_obj, PyObject *obj)
{
    PyObject *r = NULL;

    /* A `NULL' value in `obj' will prematurely terminate the argument list of
     * the dynamic function call below!
     */
    if(obj != NULL)
    {
        if(em_obj->pickler)
            r = PyObject_CallFunctionObjArgs(em_obj->pickle, obj, NULL);
        else
            r = PyObject_CallFunctionObjArgs(marshal_method, obj, proto, NULL);
    }

    return r;
}


/* Unmarshal object from string object `obj'. */
PyObject *unmarshal(em_common_t *em_obj, PyObject *obj)
{
    PyObject *r = NULL;

    /* A `NULL' value in `obj' will prematurely terminate the argument list of
     * the dynamic function call below!
     */
    if(obj != NULL)
    {
        if(em_obj->unpickler)
            r = PyObject_CallFunctionObjArgs(em_obj->unpickle, obj, NULL);
        else
            r = PyObject_CallFunctionObjArgs(unmarshal_method, obj, NULL);
    }

    return r;
}


void marshaller_fini(void)
{
    Py_DECREF(proto);
    Py_DECREF(marshal_method);
    Py_DECREF(unmarshal_method);
    Py_DECREF(module);
}
