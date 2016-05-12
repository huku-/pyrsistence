/* pyrsistence - A Python extension for External Memory Data Structures (EMDs)
 * huku <huku@grhack.net>
 *
 * pyrsistence.c - Python extension entry point.
 */
#include "includes.h"
#include "marshaller.h"
#include "common.h"
#include "em_dict.h"
#include "em_list.h"

static PyMethodDef methods[] =
{
    M_NULL
};

PyMODINIT_FUNC initpyrsistence(void)
{
    PyObject *module;

    module = Py_InitModule("pyrsistence", methods);
    register_em_dict_object(module);
    register_em_list_object(module);

    marshaller_init();
}

