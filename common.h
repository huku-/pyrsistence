#ifndef _COMMON_H_
#define _COMMON_H_


/* Magic number common to all files is "EMD\0HDR\0". */
#define MAGIC 0x0052444800444d45

/* Define two macros used for laying out readable `PyMethodDef[]' definitions.
 * Document strings are set to `NULL'.
 */
#define M_NOARGS(x, y)  {x, ((PyCFunction)(y)), METH_NOARGS, NULL}
#define M_VARARGS(x, y) {x, ((PyCFunction)(y)), METH_VARARGS, NULL}
#define M_KWARGS(x, y)  {x, ((PyCFunctionWithKeywords)(y)), METH_VARARGS | METH_KEYWORDS, NULL}

/* Define `M_NULL' to avoid using `{NULL}' in `PyMethodDef[]' definitions. Fixes
 * several compiler warnings about missing initializers thrown on my Mac OS X
 * system by LLVM. According to K&R, however, using `{NULL}' is correct.
 */
#define M_NULL {NULL, NULL, 0, NULL}


#ifdef _WIN32

#ifdef _WIN64
#define HIDWORD(x) ((x) >> 32)
#else
#define HIDWORD(x) 0
#endif

#define LODWORD(x) ((x) & 0xffffffff)
#define SSIZE_MAX _I64_MAX

/* Looks like `F_OK' is not defined on Microsoft Windows. */
#define F_OK 0
#endif


/* Common fields in all types implemented by this extension. */
typedef struct em_common
{
    PyObject_HEAD
    PyObject *pickler;   /* `pickle.Pickler' object or `NULL' */
    PyObject *pickle;    /* Pickle method (`pickler.dump()' or `_pickle.dumps()') */
    PyObject *unpickler; /* `pickle.Unpickler' object or `NULL' */
    PyObject *unpickle;  /* Unpickle method (`pickler.load()' or `_pickle.loads()') */
} em_common_t;

#define EM_COMMON(x) ((em_common_t *)((x)))

#endif /* _COMMON_H_ */
