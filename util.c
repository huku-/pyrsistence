/* pyrsistence - A Python extension for External Memory Data Structures (EMDs)
 * huku <huku@grhack.net>
 *
 * util.c - Home of various helper functions that don't fit anywhere else.
 */
#include "util.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef DEBUG
/* Prints a debugging message if `DEBUG' is defined. */
void msgf(const char *fmt, ...)
{
    char ftime[BUFSIZ], new_fmt[BUFSIZ];
    time_t now;
    va_list va;

    now = time(NULL);
    strftime(ftime, sizeof(ftime), "%Y-%m-%d %H:%M:%S", localtime(&now));
    snprintf(new_fmt, sizeof(new_fmt), "(%s) [*] %s\n", ftime, fmt);

    va_start(va, fmt);
    vfprintf(stderr, new_fmt, va);
    va_end(va);
}
#endif


/* A version of `perror()' that works on Microsoft Windows and Unixoids too. */
void serror(const char *prefix)
{
#ifdef _WIN32
    char buf[BUFSIZ];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, BUFSIZ, NULL);
    fprintf(stderr, "%s: %s", prefix, buf);
#else
    perror(prefix);
#endif
}


/* Concatenate `dirname' and `filename' into a buffer of size `PATH_MAX'. */
char *path_combine(const char *dirname, const char *filename)
{
    static char combined[PATH_MAX];

    char *ret = NULL;

    if(snprintf(combined, PATH_MAX, "%s%c%s", dirname, PATH_SEP, filename) >= PATH_MAX)
    {
        serror("path_combine: snprintf");
        goto _err;
    }

    ret = &combined[0];

_err:
    return ret;
}


/* Platform independent function for creating a directory. */
int mk_dir(const char *dirname)
{
    int ret = -1;

#ifdef _WIN32
    if(CreateDirectoryA(dirname, NULL) == 0)
    {
        serror("makedir: CreateDirectoryA");
        goto _err;
    }
#else
    if(mkdir(dirname, 0700) != 0)
    {
        serror("makedir: mkdir");
        goto _err;
    }
#endif

    ret = 0;

_err:
    return ret;
}


/* Platform independent function for removing a directory. */
void rm_dir(const char *dirname)
{
#ifdef _WIN32
    RemoveDirectoryA(dirname);
#else
    rmdir(dirname);
#endif
}


/* Compare Python objects `obj1' and `obj2' and return true if equal. */
int equal_objects(PyObject *obj1, PyObject *obj2)
{
#if PY_MAJOR_VERSION >= 3
    return (obj1 != NULL && obj2 != NULL &&
        PyObject_RichCompareBool(obj1, obj2, Py_EQ) == 1);
#else
    int result;
    return (obj1 != NULL && obj2 != NULL &&
        PyObject_Cmp(obj1, obj2, &result) != -1 && result == 0);
#endif
}


/* Make sure `obj' has a `pickle()' method and return a reference to the latter. */
int valid_pickler(PyObject *obj, PyObject **picklep)
{
    PyObject *pickle;
    int ret = -1;

    if((pickle = PyObject_GetAttrString(obj, "pickle")) == NULL)
        goto _ret;

    if(PyCallable_Check(pickle) != 1)
        goto _ret;

    *picklep = pickle;

    ret = 0;

_ret:
    return ret;
}


/* Make sure `obj' has a `unpickle()' method and return a reference to the latter. */
int valid_unpickler(PyObject *obj, PyObject **unpicklep)
{
    PyObject *unpickle;
    int ret = -1;

    if((unpickle = PyObject_GetAttrString(obj, "unpickle")) == NULL)
        goto _ret;

    if(PyCallable_Check(unpickle) != 1)
        goto _ret;

    *unpicklep = unpickle;

    ret = 0;

_ret:
    return ret;
}
