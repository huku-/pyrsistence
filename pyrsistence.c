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


#if PY_MAJOR_VERSION >= 3
static PyModuleDef pyrsistence_module =
{
    PyModuleDef_HEAD_INIT,
    "pyrsistence",
    NULL,
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};
#endif


#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_pyrsistence(void)
#else
PyMODINIT_FUNC initpyrsistence(void)
#endif
{
    PyObject *module;

#if PY_MAJOR_VERSION >= 3
    module = PyModule_Create(&pyrsistence_module);
#else
    module = Py_InitModule("pyrsistence", NULL);
#endif

    register_em_dict_object(module);
    register_em_list_object(module);

    marshaller_init();

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}

