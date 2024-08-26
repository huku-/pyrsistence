#ifndef _EM_LIST_H_
#define _EM_LIST_H_

#include <Python.h>

#include "mapped_file.h"


/* In-file header; "index.bin" begins with this structure. */
typedef struct em_list_index_hdr
{
    uint64_t magic;             /* Memory mapped file magic */
    size_t used;                /* Number of used elements */
    size_t capacity;            /* Total number of elements */
} em_list_index_hdr_t;

/* In-file header; each entry in "index.bin" has the following format. */
typedef struct em_list_index_ent
{
    size_t value_pos;           /* File offset of value in "values.bin" */
} em_list_index_ent_t;


/* In-file header; "values.bin" begins with this structure. */
typedef struct em_list_values_hdr
{
    uint64_t magic;             /* Memory mapped file magic */
} em_list_values_hdr_t;


/* Represents a Python `EMList' object. */
typedef struct em_list
{
    PyObject_HEAD
    /* See `em_common_t' in "common.h" for an explanation of the fields below. */
    PyObject *pickler;
    PyObject *pickle;
    PyObject *unpickler;
    PyObject *unpickle;
    char *dirname;              /* Directory holding memory mapped files */
    mapped_file_t *index;       /* Memory mapped file for indeces */
    mapped_file_t *values;      /* Memory mapped file for values */
    char is_open;               /* Non-zero if list is open */
} em_list_t;


/* Represents a Python `_EMListIter' object. */
typedef struct em_list_iter
{
    PyObject_HEAD
    em_list_t *em_list;         /* `EMList' object this iterator refers to */
    size_t pos;                 /* Current iterator position */
    size_t maxpos;              /* Maximum iterator position */
} em_list_iter_t;


void register_em_list_object(PyObject *);

#endif /* _EM_LIST_H_ */
