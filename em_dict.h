#ifndef _EM_DICT_H_
#define _EM_DICT_H_

#include <Python.h>

#include "mapped_file.h"

#define PERTURB_SHIFT 5

#define EM_DICT_ITER_ITEMS  0
#define EM_DICT_ITER_KEYS   1
#define EM_DICT_ITER_VALUES 2


/* In-file header; "index.bin" begins with this structure. */
typedef struct em_dict_index_hdr
{
    uint64_t magic;           /* Memory mapped file magic */
    size_t used;              /* Number of used hash slots */
    size_t mask;              /* Hash table size mask */
} em_dict_index_hdr_t;

/* In-file header; each entry in "index.bin" has the following format. */
typedef struct em_dict_index_ent
{
    Py_ssize_t hash;          /* Hash value of index entry */
    size_t key_pos;           /* Offset of key object in "keys.bin" */
    size_t value_pos;         /* Offset of value object in "values.bin" */
} em_dict_index_ent_t;


/* In-file header; "keys.bin" begins with this structure. */
typedef struct em_dict_keys_hdr
{
    uint64_t magic;           /* Memory mapped file magic */
} em_dict_keys_hdr_t;


/* In-file header; "values.bin" begins with this structure. */
typedef struct em_dict_values_hdr
{
    uint64_t magic;           /* Memory mapped file magic */
} em_dict_values_hdr_t;



/* Represents a Python `EMDict' object. */
typedef struct em_dict
{
    PyObject_HEAD
    /* See `em_common_t' in "common.h" for an explanation of the fields below. */
    PyObject *pickler;
    PyObject *pickle;
    PyObject *unpickler;
    PyObject *unpickle;
    char *dirname;            /* Directory holding memory mapped files */
    mapped_file_t *index;     /* Memory mapped file for indeces */
    mapped_file_t *keys;      /* Memory mapped file for keys */
    mapped_file_t *values;    /* Memory mapped file for values */
    char is_open;             /* Non-zero if `EMDict' is open */
} em_dict_t;


/* Represents a Python `_EMDictIter' object. */
typedef struct em_dict_iter
{
    PyObject_HEAD
    em_dict_t *em_dict;       /* `EMDict' object this iterator refers to */
    size_t pos;               /* Current iterator position */
    size_t max_pos;           /* Maximum iterator position */
    char type;                /* Iterator type, `EM_DIC_ITER_XXX' constants */
} em_dict_iter_t;



void register_em_dict_object(PyObject *);

#endif /* _EM_DICT_H_ */
