/* pyrsistence - A Python extension for External Memory Data Structures (EMDs)
 * huku <huku@grhack.net>
 *
 * marshaller.c - Python object marshalling and unmarshalling API.
 *
 * This file exports two API functions, namely `marshal()' and `unmarshal()',
 * which can be used to serialize and unserialize python objects to and from
 * string objects accordingly. Experienced Python developers may implement their
 * own object serialization methodologies, but, for now, we just make use of the
 * optimized "cPickle" module.
 */
#include "includes.h"
#include "marshaller.h"


static PyObject *module;
static PyObject *marshal_method;
static PyObject *unmarshal_method;
static PyObject *proto;


int marshaller_init(void)
{
    PyObject *dict;

    int ret = -1;

    /* Experienced developers may use other modules or even implement their own
     * marshalling and unmarshalling techniques. For now, use "cPickle", it's
     * more than enough.
     */
    if((module = PyImport_ImportModuleNoBlock("cPickle")) == NULL)
        goto _err;

    dict = PyModule_GetDict(module);

    if((marshal_method = PyDict_GetItemString(dict, "dumps")) == NULL)
    {
        Py_DECREF(module);
        goto _err;
    }

    if((unmarshal_method = PyDict_GetItemString(dict, "loads")) == NULL)
    {
        Py_DECREF(module);
        goto _err;
    }

    if((proto = PyDict_GetItemString(dict, "HIGHEST_PROTOCOL")) == NULL)
    {
        Py_DECREF(module);
        goto _err;
    }

    ret = 0;

_err:
    return ret;
}


/* Marshal object `obj' and return a string object. */
PyObject *marshal(PyObject *obj)
{
    return PyObject_CallFunctionObjArgs(marshal_method, obj, proto, NULL);
}


/* Unmarshal object from string object `obj'. */
PyObject *unmarshal(PyObject *obj)
{
    return PyObject_CallFunctionObjArgs(unmarshal_method, obj, NULL);
}


void marshaller_fini(void)
{
    Py_DECREF(marshal_method);
    Py_DECREF(unmarshal_method);
    Py_DECREF(proto);
    Py_DECREF(module);
}

