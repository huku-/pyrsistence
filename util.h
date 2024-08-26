#ifndef _UTIL_H_
#define _UTIL_H_

#include <Python.h>

#ifdef _WIN32
#define PATH_MAX MAX_PATH
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

#ifdef DEBUG
void msgf(const char *, ...);
#else
#define msgf(fmt, ...)
#endif /* DEBUG */

void serror(const char *);
char *path_combine(const char *, const char *);
int mk_dir(const char *);
void rm_dir(const char *);
int equal_objects(PyObject *, PyObject *);
int valid_pickler(PyObject *, PyObject **);
int valid_unpickler(PyObject *, PyObject **);

#endif /* _UTIL_H_ */
