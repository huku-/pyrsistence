/* pyrsistence - A Python extension for External Memory Data Structures (EMDs)
 * huku <huku@grhack.net>
 *
 * em_dict.c - External memory dictionary implementation.
 */
#include "includes.h"
#include "common.h"
#include "util.h"
#include "mapped_file.h"
#include "em_dict.h"



#define EM_DICT_E2S(x) \
    (sizeof(em_dict_index_hdr_t) + (x) * sizeof(em_dict_index_ent_t))

#define EM_DICT_S2E(x) \
    (((x) - sizeof(em_dict_index_hdr_t)) / sizeof(em_dict_index_ent_t))



/* Gets the "index.bin" entry at index `i'. */
static int em_dict_get_entry(mapped_file_t *mf, em_dict_index_ent_t *ent,
        size_t i)
{
    size_t size = sizeof(em_dict_index_ent_t);
    int ret = -1;

    if(mapped_file_seek(mf, EM_DICT_E2S(i), SEEK_SET) != 0)
        goto _err;

    if(mapped_file_read(mf, ent, size) != (ssize_t)size)
        goto _err;

    ret = 0;

_err:
    return ret;
}


/* Sets the "index.bin" entry at index `i'. */
static int em_dict_set_entry(mapped_file_t *mf, em_dict_index_ent_t *ent,
        size_t i)
{
    size_t size = sizeof(em_dict_index_ent_t);
    int ret = -1;

    if(mapped_file_seek(mf, EM_DICT_E2S(i), SEEK_SET) != 0)
        goto _err;

    if(mapped_file_write(mf, ent, size) != (ssize_t)size)
        goto _err;

    ret = 0;

_err:
    return ret;
}


/* Check if `ent' represents a free slot. */
static int em_dict_entry_is_free(em_dict_index_ent_t *ent)
{
    return (ent->hash == 0 && ent->key_pos == 0 && ent->value_pos == 0);
}



/* External memory dictionary iterator object definitions begin here. Normal
 * Python dictionaries have three kinds of iterators, one for items, one for
 * keys and one for values. The iteration logic is common to all three kinds of
 * iterators, what differs, is only the returned object type. In the original
 * Python sources, a common object named `_PyDictViewObject' is used as a basis
 * for implementing all iterators. We try to follow this design here as well.
 *
 * We define an object called `_EMDictIter', which implements the code common to
 * all iterators (notice that this object is private and thus not exported to
 * the end user). When an `_EMDictIter' object is instantiated, the object's
 * `type' member is set to the appropriate iterator type. The iterator's
 * implementation of `next()' decides the type of object to return based on the
 * value of `type'.
 */

/* Iterator's `__iter__()' method. */
static PyObject *em_dict_iter_iter(em_dict_iter_t *self)
{
    Py_INCREF(self);
    return (PyObject *)self;
}


/* Iterator's `next()' method. */
static PyObject *em_dict_iter_iternext(em_dict_iter_t *self)
{
    em_dict_index_ent_t ent;
    char type;

    size_t max_pos = self->max_pos;
    size_t pos = self->pos;
    em_dict_t *em_dict = self->em_dict;

    PyObject *key = NULL, *value = NULL, *r = NULL;


    /* If we haven't finished iterating the elements of the external memory
     * dictionary, lookup the next non-free slot.
     */
    if(pos < max_pos)
    {
        do
        {
            if(em_dict_get_entry(em_dict->index, &ent, pos) != 0)
                goto _err;
            pos += 1;
        }
        while(em_dict_entry_is_free(&ent) && pos < max_pos);
        self->pos = pos;
    }

    /* Have we found a non-free slot? If yes read key and value. */
    if(pos < max_pos)
    {
        type = self->type;

        if(type == EM_DICT_ITER_ITEMS || type == EM_DICT_ITER_KEYS)
            key = mapped_file_unmarshal_object(em_dict->keys, ent.key_pos);

        if(type == EM_DICT_ITER_ITEMS || type == EM_DICT_ITER_VALUES)
            value = mapped_file_unmarshal_object(em_dict->values, ent.value_pos);

        /* Return the appropriate object type based on the iterator's type. */
        if(type == EM_DICT_ITER_ITEMS)
        {
            if((r = Py_BuildValue("(NN)", key, value)) == NULL)
            {
                Py_DECREF(key);
                Py_DECREF(value);
            }
        }
        else if(type == EM_DICT_ITER_KEYS)
            r = key;
        else if(type == EM_DICT_ITER_VALUES)
            r = value;
    }
    else
        PyErr_SetNone(PyExc_StopIteration);

_err:
    return r;
}


static void em_dict_iter_dealloc(em_dict_iter_t *self)
{
    Py_DECREF(self->em_dict);
    PyObject_Del(self);
}


static PyTypeObject em_dict_iter_type;


/* Initialization of `pyrsistence._EMDictIter' type should go here. */
static void initialize_em_dict_iter_type(PyTypeObject *type)
{
    PyObject type_base =
    {
        PyObject_HEAD_INIT(NULL)
    };

    *(PyObject *)type = type_base;
    type->tp_name = "pyrsistence._EMDictIter";
    type->tp_basicsize = sizeof(em_dict_iter_t);
    type->tp_dealloc = (destructor)em_dict_iter_dealloc;
    type->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER;
    type->tp_doc = "Internal EMDict iterator object.";
    type->tp_iter = (getiterfunc)em_dict_iter_iter;
    type->tp_iternext = (iternextfunc)em_dict_iter_iternext;
}



/* Main external memory dictionary implementation begins here. */

/* Lookup `key' in external memory dictionary. If the key is found, 0 is
 * returned and `*pi' holds the index of the entry in "index.bin". If a free
 * slot is detected where the key should be, the return value is > 0 and `*pi'
 * holds the index of the free slot. Otherwise a value < 0 is returned and `*pi'
 * is unaffected.
 */
static int em_dict_lookup(em_dict_t *self, PyObject *key, size_t *pi)
{
    em_dict_index_hdr_t *index_hdr;
    em_dict_index_ent_t ent;
    Py_ssize_t hash;
    PyObject *r;
    size_t mask, i, perturb;
    int eq;

    mapped_file_t *keys = self->keys;
    int ret = -1;


    if((hash = PyObject_Hash(key)) == -1)
        goto _err;

    index_hdr = self->index->address;
    mask = index_hdr->mask;

    i = (size_t)hash & mask;

    if(em_dict_get_entry(self->index, &ent, i) != 0)
        goto _err;

    /* Hash value returned by `PyObject_Hash()' may be 0, so check if the entry
     * is free first.
     */
    if(em_dict_entry_is_free(&ent))
    {
        *pi = i;
        ret = 1;
        goto _err;
    }

    /* Now check if the hashes match. */
    else if(ent.hash == hash)
    {
        if((r = mapped_file_unmarshal_object(keys, ent.key_pos)) != NULL)
        {
            eq = equal_objects(key, r);
            Py_DECREF(r);

            if(eq)
            {
                *pi = i;
                ret = 0;
                goto _err;
            }
        }
    }

    /* If the dictionary becomes full, this loop will never terminate. However,
     * guaranteeing that the resize invariant is always obeyed, also guarantees
     * termination of this loop.
     */
    for(perturb = hash; ; perturb >>= PERTURB_SHIFT)
    {
        i = ((i << 2) + i + perturb + 1) & mask;

        if(em_dict_get_entry(self->index, &ent, i) != 0)
            goto _err;

        if(em_dict_entry_is_free(&ent))
        {
            *pi = i;
            ret = 1;
            goto _err;
        }

        else if(ent.hash == hash)
        {
            if((r = mapped_file_unmarshal_object(keys, ent.key_pos)) != NULL)
            {
                eq = equal_objects(key, r);
                Py_DECREF(r);

                if(eq)
                {
                    *pi = i;
                    ret = 0;
                    goto _err;
                }
            }
        }
    }

_err:
    return ret;
}


/* Resize external memory dictionary. */
static int em_dict_resize(em_dict_t *self)
{
    em_dict_index_hdr_t *index_hdr, *new_index_hdr;
    em_dict_index_ent_t ent, new_ent;
    size_t mask, new_mask, num_ents, new_num_ents, size, new_size, i, j, perturb;
    mapped_file_t *mf;
    char *filename;

    int ret = -1;


    /* Store current values for later use. */
    index_hdr = self->index->address;
    mask = index_hdr->mask;
    num_ents = mask + 1;
    size = self->index->size;

    /* Compute new values and do some sanity checking. */
    new_num_ents = num_ents << 1;
    new_size = EM_DICT_E2S(new_num_ents);
    if(new_num_ents < num_ents || new_size < size)
        goto _err;

    debug_printf("EMDict: Resizing");

    filename = path_combine(self->dirname, "index.bin.1");
    if((mf = mapped_file_create(filename, new_size)) == NULL)
        goto _err;

    new_mask = new_num_ents - 1;
    new_index_hdr = mf->address;
    new_index_hdr->magic = MAGIC;
    new_index_hdr->used = 0;
    new_index_hdr->mask = new_mask;

    debug_printf("EMDict: Rehashing");

    /* Rehash all dictionary entries in the new index file. */
    for(i = 0; i < num_ents; i++)
    {
        em_dict_get_entry(self->index, &ent, i);

        if(!em_dict_entry_is_free(&ent))
        {
            /* Locate empty slot in new index file. */
            j = ent.hash & new_mask;
            em_dict_get_entry(mf, &new_ent, j);

            perturb = ent.hash;
            while(em_dict_entry_is_free(&new_ent) == 0)
            {
                j = ((j << 2) + j + perturb + 1) & new_mask;
                perturb >>= PERTURB_SHIFT;
                em_dict_get_entry(mf, &new_ent, j);
            }

            /* Key and value offsets in "keys.bin" and "values.bin" are the same.
             * We just rehash the index entry in a (possibly) different position
             * in the new "index.bin".
             */
            em_dict_set_entry(mf, &ent, j);
            new_index_hdr->used += 1;
        }
    }

    debug_printf("EMDict: Resize successful");

    /* Move the new mapped file over the old one. When the file is closed below,
     * the kernel will remove it from our filesystem because the link count will
     * reach 0. This works on Microsoft Windows too, but with a slightly
     * different technique (see `mapped_file_unlink()' for more information).
     */
    filename = path_combine(self->dirname, "index.bin.0");
    if(mapped_file_rename(self->index, filename) != 0)
        goto _err;

    filename = path_combine(self->dirname, "index.bin");
    if(mapped_file_rename(mf, filename) != 0)
        goto _err;

    mapped_file_unlink(self->index);
    mapped_file_close(self->index);

    self->index = mf;

    ret = 0;

_err:
    return ret;
}



/* Mapping protocol implementation. */

/* Callback for Python's `len()'. */
static Py_ssize_t em_dict_len(em_dict_t *self)
{
    return ((em_dict_index_hdr_t *)(self->index->address))->used;
}


/* Retrieve item from external memory dictionary. */
static PyObject *em_dict_getitem(em_dict_t *self, PyObject *key)
{
    em_dict_index_ent_t ent;
    size_t i;

    PyObject *r = NULL;

    if(em_dict_lookup(self, key, &i) == 0)
    {
        memset(&ent, 0, sizeof(em_dict_index_ent_t));
        em_dict_get_entry(self->index, &ent, i);
        r = mapped_file_unmarshal_object(self->values, ent.value_pos);
    }
    else
        PyErr_SetString(PyExc_KeyError, "No such key");

    return r;
}


/* Insert item in external memory dictionary. */
static int em_dict_setitem(em_dict_t *self, PyObject *key, PyObject *value)
{
    em_dict_index_hdr_t *index_hdr;
    em_dict_index_ent_t ent;
    Py_ssize_t hash;
    ssize_t key_pos, value_pos;
    size_t i;

    mapped_file_t *index = self->index;
    mapped_file_t *keys = self->keys;
    mapped_file_t *values = self->values;
    int ret = -1;


    ret = em_dict_lookup(self, key, &i);

    /* If `ret > 0' a free slot was found where `key' and `value' can be placed.
     * If `ret == 0', `key' was already present in the dictionary and its slot
     * was returned.
     */
    if(ret >= 0)
    {
        if((hash = PyObject_Hash(key)) == -1)
            goto _err;

        /* If the key was already present in the dictionary, lookup the index
         * entry, free the old value object and re-use the key object.
         */
        key_pos = value_pos = 0;
        if(ret == 0)
        {
            em_dict_get_entry(index, &ent, i);
            key_pos = ent.key_pos;
            value_pos = ent.value_pos;
            mapped_file_free_chunk(values, value_pos);
        }

        /* Marshal the key object only if it's not already in the dictionary. */
        if(key_pos == 0 && (key_pos = mapped_file_marshal_object(keys, key)) < 0)
            goto _err;

        /* Marshal the new value object. */
        if((value_pos = mapped_file_marshal_object(values, value)) < 0)
            goto _err;

        /* Build a new index entry. */
        memset(&ent, 0, sizeof(ent));
        ent.hash = hash;
        ent.key_pos = key_pos;
        ent.value_pos = value_pos;
        em_dict_set_entry(index, &ent, i);

        index_hdr = index->address;

        /* Increase `used' only if a free slot was used. */
        if(ret > 0)
            index_hdr->used += 1;

        /* Check if we should resize. */
        if(index_hdr->used * 3 >= (index_hdr->mask + 1) * 2)
        {
            if(em_dict_resize(self) != 0)
                goto _err;
        }

        ret = 0;
    }

_err:
    return ret;
}



/* Standard interface to `open()' and `close()'. */

/* Create a new external memory dictionary. */
static int em_dict_create(em_dict_t *self)
{
    mapped_file_t *mf;
    em_dict_index_hdr_t index_hdr;
    em_dict_keys_hdr_t keys_hdr;
    em_dict_values_hdr_t values_hdr;
    char *filename;

    const char *dirname = self->dirname;


    /* Create directory to hold external memory dictionary files. */
    if(mk_dir(dirname) != 0)
        goto _err1;

    /* Create "index.bin" and write file header (initial size 65k entries). */
    filename = path_combine(dirname, "index.bin");
    if((mf = mapped_file_create(filename, EM_DICT_E2S(65536))) == NULL)
        goto _err2;

    index_hdr.magic = MAGIC;
    index_hdr.used = 0;
    index_hdr.mask = 65536 - 1;
    mapped_file_write(mf, &index_hdr, sizeof(em_dict_index_hdr_t));

    self->index = mf;

    /* Create "keys.bin" and write file header (initial size 65k). */
    filename = path_combine(dirname, "keys.bin");
    if((mf = mapped_file_create(filename, 65536)) == NULL)
        goto _err3;

    keys_hdr.magic = MAGIC;
    mapped_file_write(mf, &keys_hdr, sizeof(em_dict_keys_hdr_t));

    self->keys = mf;

    /* Create "values.bin" and write file header (initial size 65k). */
    filename = path_combine(dirname, "values.bin");
    if((mf = mapped_file_create(filename, 65536)) == NULL)
        goto _err4;

    values_hdr.magic = MAGIC;
    mapped_file_write(mf, &values_hdr, sizeof(em_dict_values_hdr_t));

    self->values = mf;
    return 0;

_err4:
    mapped_file_unlink(self->keys);
    mapped_file_close(self->keys);

_err3:
    mapped_file_unlink(self->index);
    mapped_file_close(self->index);

_err2:
    rm_dir(dirname);

_err1:
    PyErr_SetString(PyExc_RuntimeError, "Cannot open EMDict");
    return -1;
}


/* Open existing external memory dictionary. */
static int em_dict_open_existing(em_dict_t *self)
{
    mapped_file_t *mf;
    em_dict_index_hdr_t *index_hdr;
    em_dict_keys_hdr_t *keys_hdr;
    em_dict_values_hdr_t *values_hdr;
    size_t pos;

    const char *dirname = self->dirname;
    char *filename;


    /* Open and verify "index.bin". */
    filename = path_combine(dirname, "index.bin");
    if((mf = mapped_file_open(filename)) == NULL)
        goto _err1;

    self->index = mf;
    index_hdr = mf->address;

    if(index_hdr->magic != MAGIC)
        goto _err2;

    /* Open and verify "keys.bin". */
    filename = path_combine(dirname, "keys.bin");
    if((mf = mapped_file_open(filename)) == NULL)
        goto _err2;

    self->keys = mf;
    keys_hdr = mf->address;

    pos = mapped_file_get_eof(mf);
    if(keys_hdr->magic != MAGIC || mapped_file_seek(mf, pos, SEEK_SET) != 0)
        goto _err3;

    /* Open and verify "values.bin". */
    filename = path_combine(dirname, "values.bin");
    if((mf = mapped_file_open(filename)) == NULL)
        goto _err3;

    self->values = mf;
    values_hdr = mf->address;

    pos = mapped_file_get_eof(mf);
    if(values_hdr->magic != MAGIC || mapped_file_seek(mf, pos, SEEK_SET) != 0)
        goto _err4;

    return 0;

_err4:
    mapped_file_close(self->values);

_err3:
    mapped_file_close(self->keys);

_err2:
    mapped_file_close(self->index);

_err1:
    PyErr_SetString(PyExc_RuntimeError, "Cannot open EMDict");
    return -1;
}


/* Called by `em_dict_open()' and `em_dict_init()'. */
static int em_dict_open_common(em_dict_t *self, PyObject *args)
{
    char *dirname;

    int ret = -1;


    if(PyArg_ParseTuple(args, "s", &dirname) == 0)
        goto _err;

    if((self->dirname = PyMem_MALLOC(strlen(dirname) + 1)) == NULL)
    {
        PyErr_NoMemory();
        goto _err;
    }

    strcpy(self->dirname, dirname);

    /* XXX: There's an obvious race condition here, should we care? */
    if(access(self->dirname, F_OK) == 0)
        ret = em_dict_open_existing(self);
    else
        ret = em_dict_create(self);

    if(ret >= 0)
        self->is_open = 1;

_err:
    return ret;
}


/* Open external memory dictionary. */
static PyObject *em_dict_open(em_dict_t *self, PyObject *args)
{
    PyObject *r = NULL;

    if(self->is_open)
    {
        PyErr_SetString(PyExc_RuntimeError, "EMDict already open");
        goto _err;
    }

    if(em_dict_open_common(self, args) != 0)
        goto _err;

    Py_INCREF(Py_True);
    r = Py_True;

_err:
    return r;
}


/* Synchronize and close an external memory dictionary. */
static PyObject *em_dict_close(em_dict_t *self)
{
    mapped_file_t *index = self->index;
    mapped_file_t *keys = self->keys;
    mapped_file_t *values = self->values;

    if(self->is_open)
    {
        /* Sync and close "index.bin". */
        mapped_file_sync(index, 0, index->size);
        mapped_file_close(index);

        /* Sync, truncate and close "keys.bin". */
        mapped_file_sync(keys, 0, keys->size);
        mapped_file_truncate(keys, mapped_file_get_eof(keys));
        mapped_file_close(keys);

        /* Sync, truncate and close "values.bin". */
        mapped_file_sync(values, 0, values->size);
        mapped_file_truncate(values, mapped_file_get_eof(values));
        mapped_file_close(values);

        PyMem_FREE(self->dirname);
        self->is_open = 0;
    }

    Py_RETURN_NONE;
};



/* Standard interface to `items()', `keys()' and `values()'. */

/* Initialize and return an `EMDict' iterator of type `type'. */
static PyObject *em_dict_iterator_new(em_dict_t *self, char type)
{
    em_dict_iter_t *iter;

    iter = PyObject_New(em_dict_iter_t, &em_dict_iter_type);

    PyObject_Init((PyObject *)iter, &em_dict_iter_type);
    Py_INCREF(self);

    /* XXX: Maybe initialize `max_pos' to index file's `used' member? */
    iter->em_dict = self;
    iter->pos = 0;
    iter->max_pos = ((em_dict_index_hdr_t *)self->index)->mask + 1;
    iter->type = type;
    return (PyObject *)iter;
}

/* Get an iterator for key-value tuples. */
static PyObject *em_dict_items(em_dict_t *self)
{
    return em_dict_iterator_new(self, EM_DICT_ITER_ITEMS);
}

/* Get an iterator for keys. */
static PyObject *em_dict_keys(em_dict_t *self)
{
    return em_dict_iterator_new(self, EM_DICT_ITER_KEYS);
}

/* Get an iterator for values. */
static PyObject *em_dict_values(em_dict_t *self)
{
    return em_dict_iterator_new(self, EM_DICT_ITER_VALUES);
}



/* Called via `tp_init()'. */
static int em_dict_init(em_dict_t *self, PyObject *args, PyObject *kwargs)
{
    int ret = -1;

    if(self->is_open)
    {
        PyErr_SetString(PyExc_RuntimeError, "EMDict already open");
        goto _err;
    }

    if(em_dict_open_common(self, args) != 0)
        goto _err;

    ret = 0;

_err:
    return ret;
}


/* Called via `tp_dealloc()'. */
static void em_dict_dealloc(em_dict_t *self)
{
    PyObject *r = em_dict_close(self);
    Py_DECREF(r);
    PyObject_Del(self);
}



static PyMappingMethods em_dict_mapping_proto =
{
    (lenfunc)em_dict_len,
    (binaryfunc)em_dict_getitem,
    (objobjargproc)em_dict_setitem
};

static PyMethodDef em_dict_methods[] =
{
    M_NOARGS("open", em_dict_open),
    M_NOARGS("items", em_dict_items),
    M_NOARGS("keys", em_dict_keys),
    M_NOARGS("values", em_dict_values),
    M_NOARGS("close", em_dict_close),
    M_NULL
};

static PyMemberDef em_dict_members[] =
{
    {NULL, 0, 0, 0, NULL}
};

static PyTypeObject em_dict_type;


/* Initialization of `pyrsistence.EMDict' type should go here. */
static void initialize_em_dict_type(PyTypeObject *type)
{
    PyObject type_base =
    {
        PyObject_HEAD_INIT(NULL)
    };

    *(PyObject *)type = type_base;
    type->tp_name = "pyrsistence.EMDict";
    type->tp_basicsize = sizeof(em_dict_t);
    type->tp_dealloc = (destructor)em_dict_dealloc;
    type->tp_as_mapping = &em_dict_mapping_proto;
    type->tp_flags = Py_TPFLAGS_DEFAULT;
    type->tp_doc = "External memory dictionary implementation.";
    type->tp_methods = em_dict_methods;
    type->tp_members = em_dict_members;
    type->tp_init = (initproc)em_dict_init;
    type->tp_new = PyType_GenericNew;
}


void register_em_dict_object(PyObject *module)
{
    initialize_em_dict_type(&em_dict_type);
    if(PyType_Ready(&em_dict_type) == 0)
    {
        Py_INCREF(&em_dict_type);
        PyModule_AddObject(module, "EMDict", (PyObject *)&em_dict_type);
    }

    initialize_em_dict_iter_type(&em_dict_iter_type);
    if(PyType_Ready(&em_dict_iter_type) == 0)
    {
        Py_INCREF(&em_dict_iter_type);
        PyModule_AddObject(module, "_EMDictIter", (PyObject *)&em_dict_iter_type);
    }
};

