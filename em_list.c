/* pyrsistence - A Python extension for External Memory Data Structures (EMDs)
 * huku <huku@grhack.net>
 *
 * em_list.c - External memory list implementation.
 */
#include "includes.h"
#include "common.h"
#include "util.h"
#include "mapped_file.h"
#include "em_list.h"


#define EM_LIST_E2S(x) \
    (sizeof(em_list_index_hdr_t) + (x) * sizeof(em_list_index_ent_t))

#define EM_LIST_S2E(x) \
    (((x) - sizeof(em_list_index_hdr_t)) / sizeof(em_list_index_ent_t))



/* Functions for handling index entries in "index.bin". */

/* Gets the "index.bin" entry at index `i'. */
static int em_list_get_entry(mapped_file_t *mf, em_list_index_ent_t *ent,
        size_t i)
{
    size_t size = sizeof(em_list_index_ent_t);
    int ret = -1;

    if(mapped_file_seek(mf, EM_LIST_E2S(i), SEEK_SET) != 0)
        goto _err;

    if(mapped_file_read(mf, ent, size) != (ssize_t)size)
        goto _err;

    ret = 0;

_err:
    return ret;
}


/* Sets the "index.bin" entry at index `i'. */
static int em_list_set_entry(mapped_file_t *mf, em_list_index_ent_t *ent,
        size_t i)
{
    size_t size = sizeof(em_list_index_ent_t);
    int ret = -1;

    if(mapped_file_seek(mf, EM_LIST_E2S(i), SEEK_SET) != 0)
        goto _err;

    if(mapped_file_write(mf, ent, size) != (ssize_t)size)
        goto _err;

    ret = 0;

_err:
    return ret;
}



/* Mapping protocol implementation. */

/* Callback for Python's `len()'. */
static Py_ssize_t em_list_len(em_list_t *self)
{
    return ((em_list_index_hdr_t *)(self->index->address))->used;
}


static PyObject *em_list_getitem_internal(em_list_t *self, Py_ssize_t index)
{
    em_list_index_hdr_t *index_hdr;
    em_list_index_ent_t ent;
    size_t value_pos;

    PyObject *r = NULL;

    index_hdr = self->index->address;
    if(index < 0 || (size_t)index >= index_hdr->used)
    {
        PyErr_SetString(PyExc_IndexError, "Invalid index");
        goto _err;
    }

    if(em_list_get_entry(self->index, &ent, (size_t)index) != 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to read index entry");
        goto _err;
    }

    value_pos = ent.value_pos;
    if(value_pos != 0)
    {
        if((r = mapped_file_unmarshal_object(self->values, value_pos)) == NULL)
            PyErr_SetString(PyExc_RuntimeError, "Failed to unmarshal value object");
    }
    else
    {
        Py_INCREF(Py_None);
        r = Py_None;
    }

_err:
    return r;
}


/* Retrieve item from external memory list.
 *
 * XXX: Support slice objects and negative indices?
 */
static PyObject *em_list_getitem(em_list_t *self, PyObject *key)
{
    Py_ssize_t index;

    PyObject *r = NULL;

    /* Python 3 supports only long integers. */
#if PY_MAJOR_VERSION < 3
    if(PyInt_CheckExact(key))
    {
        index = PyInt_AsSsize_t(key);
        r = em_list_getitem_internal(self, index);
    }
    else if(PyLong_CheckExact(key))
#else
    if(PyLong_CheckExact(key))
#endif
    {
        index = PyLong_AsSsize_t(key);
        r = em_list_getitem_internal(self, index);
    }
    else
        PyErr_SetString(PyExc_TypeError, "Invalid key object type");

    return r;
}


/* Does the hard work of actually inserting an item in the external memory list. */
static int em_list_setitem_internal(em_list_t *self, Py_ssize_t index,
        PyObject *value)
{
    em_list_index_ent_t ent;
    ssize_t value_pos;

    int ret = -1;


    if(em_list_get_entry(self->index, &ent, (size_t)index) != 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to read index entry");
        goto _err;
    }

    if(ent.value_pos != 0)
        mapped_file_free_chunk(self->values, ent.value_pos);

    if((value_pos = mapped_file_marshal_object(self->values, value)) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to marshal value object");
        goto _err;
    }

    ent.value_pos = (size_t)value_pos;
    if(em_list_set_entry(self->index, &ent, (size_t)index) != 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to write index entry");
        goto _err;
    }

    ret = 0;

_err:
    return ret;
}


/* Sanitize index and insert item in external memory list. */
static int em_list_setitem_safe(em_list_t *self, Py_ssize_t index, PyObject *value)
{
    em_list_index_hdr_t *index_hdr;

    int ret = -1;

    index_hdr = self->index->address;
    if(index < 0)
        index += index_hdr->used;

    if(index < 0 || (size_t)index >= index_hdr->used)
    {
        PyErr_SetString(PyExc_IndexError, "List index out of range");
        goto _err;
    }

    ret = em_list_setitem_internal(self, index, value);

_err:
    return ret;
}


/* Insert item in external memory list.
 *
 * XXX: Support slice objects and negative indices?
 */
static int em_list_setitem(em_list_t *self, PyObject *key, PyObject *value)
{
    Py_ssize_t index;

    int ret = -1;

    /* Python 3 supports only long integers. */
#if PY_MAJOR_VERSION < 3
    if(PyInt_CheckExact(key))
    {
        index = PyInt_AsSsize_t(key);
        ret = em_list_setitem_safe(self, index, value);
    }
    else if(PyLong_CheckExact(key))
#else
    if(PyLong_CheckExact(key))
#endif
    {
        index = PyLong_AsSsize_t(key);
        ret = em_list_setitem_safe(self, index, value);
    }
    else
        PyErr_SetString(PyExc_TypeError, "Invalid index type");

    return ret;
}



static int em_list_resize(em_list_t *self)
{
    size_t capacity, new_capacity, new_size;
    char *filename;
    mapped_file_t *mf;
    em_list_index_hdr_t *index_hdr, *new_index_hdr;


    /* Get a reference to the current index file header. */
    index_hdr = (em_list_index_hdr_t *)self->index->address;

    /* Compute new capacity and size in bytes and check for overflows. */
    capacity = index_hdr->capacity;

    if(capacity == 0)
        new_capacity = 1;
    else
        new_capacity = capacity << 1;
    new_size = EM_LIST_E2S(new_capacity);

    if(new_capacity < capacity || new_size < new_capacity)
    {
        PyErr_SetString(PyExc_RuntimeError, "Integer overflow while resizing EMList");
        goto _err1;
    }


    msgf("EMList: Resizing");

    filename = path_combine(self->dirname, "index.bin.1");
    if((mf = mapped_file_create(filename, new_size)) == NULL)
        goto _err1;

    /* Copy old entries to the new external memory list. */
    memcpy(mf->address, self->index->address, EM_LIST_E2S(capacity));

    msgf("EMList: Resize successful");


    filename = path_combine(self->dirname, "index.bin.0");
    if(mapped_file_rename(self->index, filename) != 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Cannot rename old EMList index file");
        goto _err2;
    }

    filename = path_combine(self->dirname, "index.bin");
    if(mapped_file_rename(mf, filename) != 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Cannot rename new EMList index file");
        goto _err2;
    }

    /* Populate new index file header. */
    new_index_hdr = (em_list_index_hdr_t *)mf->address;
    new_index_hdr->magic = MAGIC;
    new_index_hdr->used = index_hdr->used;
    new_index_hdr->capacity = new_capacity;

    mapped_file_unlink(self->index);
    mapped_file_close(self->index);

    self->index = mf;
    return 0;

_err2:
    mapped_file_unlink(mf);
    mapped_file_close(mf);

_err1:
    return -1;
}


/* Append item in external memory list. */
static PyObject *em_list_append(em_list_t *self, PyObject *args)
{
    em_list_index_hdr_t *index;
    size_t used;
    PyObject *value;

    PyObject *r = NULL;


    if(PyArg_ParseTuple(args, "O", &value) == 0)
        goto _err;

    index = self->index->address;
    used = index->used;

    /* Resize memory mapped index if needed. */
    if(used >= index->capacity)
    {
        if(em_list_resize(self) != 0)
            goto _err;

        /* Pointer to memory mapped index file has probably been modified. */
        index = self->index->address;
    }

    /* Add item in external memory list, but don't increase reference count! */
    if(em_list_setitem_internal(self, used, value) != 0)
        goto _err;

    used += 1;
    index->used = used;

    r = Py_None;
    Py_INCREF(r);

_err:
    return r;
}



/* External memory list iterator interface. */

/* Iterator's `__iter__()' method. */
static PyObject *em_list_iter_iter(em_list_iter_t *self)
{
    Py_INCREF(self);
    return (PyObject *)self;
}


/* Iterator's `next()' method. */
static PyObject *em_list_iter_iternext(em_list_iter_t *self)
{
    em_list_t *em_list = self->em_list;
    size_t pos = self->pos;

    PyObject *r = NULL;

    if(pos < self->maxpos)
    {
        r = em_list_getitem_internal(em_list, pos);
        pos += 1;
        self->pos = pos;
    }
    else
        PyErr_SetNone(PyExc_StopIteration);

    return r;
}

static void em_list_iter_dealloc(em_list_iter_t *self)
{
    Py_DECREF(self->em_list);
    PyObject_Del(self);
}

static PyTypeObject em_list_iter_type_base =
{
    PyVarObject_HEAD_INIT(NULL, 0)
};

static PyTypeObject em_list_iter_type;


/* Initialization of `pyrsistence._EMListIter' type should go here. */
static void initialize_em_list_iter_type(PyTypeObject *type)
{
    *type = em_list_iter_type_base;
    type->tp_name = "pyrsistence._EMListIter";
    type->tp_basicsize = sizeof(em_list_iter_t);
    type->tp_dealloc = (destructor)em_list_iter_dealloc;
#if PY_MAJOR_VERSION >= 3
    type->tp_flags = Py_TPFLAGS_DEFAULT;
#else
    type->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER;
#endif
    type->tp_doc = "Internal EMList iterator object.";
    type->tp_iter = (getiterfunc)em_list_iter_iter;
    type->tp_iternext = (iternextfunc)em_list_iter_iternext;
}



/* This is the `tp_iter()' method of `EMList' object. */
static PyObject *em_list_iter(em_list_t *self)
{
    em_list_index_hdr_t *index = self->index->address;

    em_list_iter_t *iter;

    if((iter = PyObject_New(em_list_iter_t, &em_list_iter_type)) != NULL)
    {
        PyObject_Init((PyObject *)iter, &em_list_iter_type);
        Py_INCREF(self);

        iter->em_list = self;
        iter->pos = 0;
        iter->maxpos = index->used;
    }
    else
        PyErr_SetString(PyExc_RuntimeError, "Failed to initialize iterator");

    return (PyObject *)iter;
}



/* Standard interface to `open()' and `close()'. */

/* Create a new external memory list. */
static int em_list_create(em_list_t *self)
{
    mapped_file_t *mf;
    em_list_index_hdr_t index_hdr;
    em_list_values_hdr_t values_hdr;
    size_t size;
    char *filename;

    const char *dirname = self->dirname;

    /* Compute initial external memory list index size. */
    size = EM_LIST_E2S(0);

    /* Create directory to hold external memory list files. */
    if(mk_dir(dirname) != 0)
        goto _err1;

    /* Create "index.bin" and write file header. */
    filename = path_combine(dirname, "index.bin");
    if((mf = mapped_file_create(filename, size)) == NULL)
        goto _err2;

    index_hdr.magic = MAGIC;
    index_hdr.used = 0;
    index_hdr.capacity = 0;
    mapped_file_write(mf, &index_hdr, sizeof(em_list_index_hdr_t));

    self->index = mf;

    /* Create "values.bin" and write file header. */
    filename = path_combine(dirname, "values.bin");
    if((mf = mapped_file_create(filename, size)) == NULL)
        goto _err3;

    values_hdr.magic = MAGIC;
    mapped_file_write(mf, &values_hdr, sizeof(em_list_values_hdr_t));

    self->values = mf;
    return 0;

_err3:
    mapped_file_unlink(self->index);
    mapped_file_close(self->index);

_err2:
    rm_dir(dirname);

_err1:
    PyErr_SetString(PyExc_RuntimeError, "Cannot open EMList");
    return -1;
}


/* Open existing external memory list. */
static int em_list_open_existing(em_list_t *self)
{
    mapped_file_t *mf;
    em_list_index_hdr_t *index_hdr;
    em_list_values_hdr_t *values_hdr;
    size_t pos;
    char *filename;

    const char *dirname = self->dirname;


    /* Open and verify "index.bin". */
    filename = path_combine(dirname, "index.bin");
    if((mf = mapped_file_open(filename)) == NULL)
        goto _err1;

    self->index = mf;
    index_hdr = mf->address;

    if(index_hdr->magic != MAGIC)
        goto _err2;

    /* Open and verify "values.bin". */
    filename = path_combine(dirname, "values.bin");
    if((mf = mapped_file_open(filename)) == NULL)
        goto _err2;

    self->values = mf;
    values_hdr = mf->address;

    pos = mapped_file_get_eof(mf);
    if(values_hdr->magic != MAGIC || mapped_file_seek(mf, pos, SEEK_SET) != 0)
        goto _err3;

    return 0;

_err3:
    mapped_file_close(self->values);

_err2:
    mapped_file_close(self->index);

_err1:
    PyErr_SetString(PyExc_RuntimeError, "Cannot open EMList");
    return -1;
}


/* Called by `em_list_open()' and `em_list_init()'. */
static int em_list_open_common(em_list_t *self, PyObject *args)
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
        ret = em_list_open_existing(self);
    else
        ret = em_list_create(self);

    if(ret >= 0)
        self->is_open = 1;

_err:
    return ret;
}


/* Open external memory list. */
static PyObject *em_list_open(em_list_t *self, PyObject *args)
{
    PyObject *r = NULL;

    if(self->is_open)
    {
        PyErr_SetString(PyExc_RuntimeError, "EMList already open");
        goto _err;
    }

    if(em_list_open_common(self, args) != 0)
        goto _err;

    Py_INCREF(Py_True);
    r = Py_True;

_err:
    return r;
}


/* Synchronize and close an external memory list. */
static PyObject *em_list_close(em_list_t *self)
{
    mapped_file_t *index = self->index;
    mapped_file_t *values = self->values;

    if(self->is_open)
    {
        /* Sync and close "index.bin". */
        mapped_file_sync(index, 0, index->size);
        mapped_file_close(index);

        /* Sync and close "values.bin". */
        mapped_file_sync(values, 0, values->size);
        mapped_file_truncate(values, mapped_file_get_eof(values));
        mapped_file_close(values);

        PyMem_FREE(self->dirname);
        self->is_open = 0;
    }

    Py_RETURN_NONE;
};



/* Called via `tp_init()'. */
static int em_list_init(em_list_t *self, PyObject *args, PyObject *kwargs)
{
    int ret = -1;

    if(self->is_open)
    {
        PyErr_SetString(PyExc_RuntimeError, "EMList already open");
        goto _err;
    }

    if(em_list_open_common(self, args) != 0)
        goto _err;

    ret = 0;

_err:
    return ret;
}


/* Called via `tp_dealloc()'. */
static void em_list_dealloc(em_list_t *self)
{
    PyObject *r = em_list_close(self);
    Py_DECREF(r);
    PyObject_Del(self);
}



static PyMappingMethods mapping_proto =
{
    (lenfunc)em_list_len,
    (binaryfunc)em_list_getitem,
    (objobjargproc)em_list_setitem
};

static PyMethodDef methods[] =
{
    M_VARARGS("open", em_list_open),
    M_VARARGS("append", em_list_append),
    M_NOARGS("close", em_list_close),
    M_NULL
};

static PyMemberDef members[] =
{
    {NULL, 0, 0, 0, NULL}
};

static PyTypeObject em_list_type_base =
{
    PyVarObject_HEAD_INIT(NULL, 0)
};

static PyTypeObject em_list_type;


/* Initialization of `pyrsistence.EMList' type should go here. */
static void initialize_em_list_type(PyTypeObject *type)
{
    *type = em_list_type_base;
    type->tp_name = "pyrsistence.EMList";
    type->tp_basicsize = sizeof(em_list_t);
    type->tp_dealloc = (destructor)em_list_dealloc;
    type->tp_as_mapping = &mapping_proto;
    type->tp_flags = Py_TPFLAGS_DEFAULT;
    type->tp_doc = "External memory list implementation.";
    type->tp_iter = (getiterfunc)em_list_iter;
    type->tp_methods = methods;
    type->tp_members = members;
    type->tp_init = (initproc)em_list_init;
    type->tp_new = PyType_GenericNew;
}


void register_em_list_object(PyObject *module)
{
    initialize_em_list_type(&em_list_type);
    if(PyType_Ready(&em_list_type) == 0)
    {
        Py_INCREF(&em_list_type);
        PyModule_AddObject(module, "EMList", (PyObject *)&em_list_type);
    }

    initialize_em_list_iter_type(&em_list_iter_type);
    if(PyType_Ready(&em_list_iter_type) == 0)
    {
        Py_INCREF(&em_list_iter_type);
        PyModule_AddObject(module, "_EMListIter", (PyObject *)&em_list_iter_type);
    }
};

